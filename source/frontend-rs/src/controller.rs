// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::{BTreeMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Mutex;
use std::time::{Duration, Instant};

use serde::Serialize;

use crate::discovery::{DiscoveryOptions, DiscoveryRunner, PluginDatabase};
use crate::ffi::host::{CarlaHost, EngineEvent, PluginInfo, RuntimeDriverDeviceInfo};
use crate::ffi::types::*;
use crate::ffi::utils::{CarlaUtils, DiscoveredPlugin};
use crate::ffi::{dirs_home, lib_filename, resolve_carla_paths};
use crate::history::{EditHistory, HistoryAction, MergeKey};
use crate::logging;
use crate::model::category::{
    FRANCA_CATEGORY_PROPERTY, FRANCA_PLUGIN_CATEGORIES, category_hex, category_key, category_label,
    parse_category_key,
};
use crate::model::meters::{ClipWarningState, PeakMeterState, monotonic_secs};
use crate::patchbay::PatchbayState;
use crate::patchbay::arrange::NodeKey;
use crate::project::{PluginSnapshot, is_project_file, is_supported_file};
use crate::settings::{
    FrancaSettings, default_clap_paths, default_dssi_paths, default_jsfx_paths,
    default_ladspa_paths, default_lv2_paths, default_sf2_paths, default_sfz_paths,
    default_vst2_paths, default_vst3_paths,
};

pub static SIG_QUIT_REQUESTED: AtomicBool = AtomicBool::new(false);
pub static SIG_SAVE_REQUESTED: AtomicBool = AtomicBool::new(false);
pub static INITIAL_PROJECT: Mutex<Option<PathBuf>> = Mutex::new(None);

pub struct RackSlot {
    pub plugin_id: u32,
    pub info: PluginInfo,
    pub active: bool,
    pub category: i32,
    pub custom_ui_visible: bool,
    pub custom_ui_failed: bool,
    pub unavailable_error: Option<String>,
    pub in_channels: usize,
    pub out_channels: usize,
    pub peak_in: PeakMeterState,
    pub peak_out: PeakMeterState,
    pub clip_warning: ClipWarningState,
    pub active_notes: HashSet<u8>,
}

impl RackSlot {
    pub fn from_host(host: &CarlaHost, plugin_id: u32) -> Self {
        let info = host.get_plugin_info(plugin_id);
        let active = host.get_internal_parameter_value(plugin_id, PARAMETER_ACTIVE) >= 0.5;
        let saved_cat = host.get_custom_data_value(
            plugin_id,
            CUSTOM_DATA_TYPE_PROPERTY,
            FRANCA_CATEGORY_PROPERTY,
        );
        let category = parse_category_key(&saved_cat).unwrap_or(info.category);
        let audio = host.get_audio_port_count_info(plugin_id);
        let in_ch = (audio.ins as usize).min(2);
        let out_ch = (audio.outs as usize).min(2);
        Self {
            plugin_id,
            info,
            active,
            category,
            custom_ui_visible: false,
            custom_ui_failed: false,
            unavailable_error: None,
            in_channels: in_ch,
            out_channels: out_ch,
            peak_in: PeakMeterState::new(in_ch),
            peak_out: PeakMeterState::new(out_ch),
            clip_warning: ClipWarningState::new(),
            active_notes: HashSet::new(),
        }
    }

    pub fn reload_info(&mut self, host: &CarlaHost) {
        self.info = host.get_plugin_info(self.plugin_id);
        self.active = host.get_internal_parameter_value(self.plugin_id, PARAMETER_ACTIVE) >= 0.5;
        let audio = host.get_audio_port_count_info(self.plugin_id);
        self.in_channels = (audio.ins as usize).min(2);
        self.out_channels = (audio.outs as usize).min(2);
        self.peak_in.set_channels(self.in_channels);
        self.peak_out.set_channels(self.out_channels);
    }

    pub fn set_category(&mut self, host: &CarlaHost, category: i32, persist: bool) {
        self.category = category;
        if persist {
            host.set_custom_data(
                self.plugin_id,
                CUSTOM_DATA_TYPE_PROPERTY,
                FRANCA_CATEGORY_PROPERTY,
                category_key(category),
            );
        }
    }

    pub fn update_peaks(&mut self, host: &CarlaHost, now_secs: f64) {
        if !self.active || self.unavailable_error.is_some() {
            self.peak_in.clear();
            self.peak_out.clear();
            self.clip_warning.clear();
            return;
        }
        if self.in_channels > 0 {
            let l = host.get_input_peak_value(self.plugin_id, true);
            self.peak_in.update_channel(0, l, now_secs);
            if self.in_channels > 1 {
                let r = host.get_input_peak_value(self.plugin_id, false);
                self.peak_in.update_channel(1, r, now_secs);
            }
        }
        if self.out_channels > 0 {
            let l = host.get_output_peak_value(self.plugin_id, true);
            self.peak_out.update_channel(0, l, now_secs);
            if self.out_channels > 1 {
                let r = host.get_output_peak_value(self.plugin_id, false);
                self.peak_out.update_channel(1, r, now_secs);
            }
        }
        let max_peak = self.peak_in.max_raw().max(self.peak_out.max_raw());
        self.clip_warning.record_peak(max_peak, now_secs);
    }
}

#[derive(Clone, Debug, Default)]
pub struct PendingUiSignals {
    pub alerts: Vec<(String, String)>,
    pub focus_patchbay: bool,
    pub quit_requested: bool,
    pub rack_dirty: bool,
    pub patchbay_dirty: bool,
    pub plugin_edit_dirty_ids: Vec<u32>,
    pub logs_dirty: bool,
    pub files_dirty: bool,
    pub discovery_dirty: bool,
}

pub struct FrancaCore {
    pub host: Option<CarlaHost>,
    pub utils: Option<CarlaUtils>,
    pub bin_dir: PathBuf,
    pub res_dir: PathBuf,
    pub settings: FrancaSettings,
    pub db: PluginDatabase,
    pub supported_exts: Vec<String>,
    pub discovery_runner: Option<DiscoveryRunner>,

    pub epoch: Instant,
    pub last_slow_poll: Instant,
    pub engine_running: bool,
    pub running_driver_name: String,
    pub process_mode: i32,
    pub transport_mode: i32,
    pub buffer_size: u32,
    pub sample_rate: f64,
    pub runtime_info: CarlaRuntimeEngineInfo,
    pub runtime_dev_info: RuntimeDriverDeviceInfo,
    pub transport_info: CarlaTransportInfo,
    pub transport_link: bool,
    pub project_filename: Option<PathBuf>,
    pub is_project_loading: bool,
    pub removing_all_plugins: bool,
    pub external_patchbay: bool,
    pub needs_patchbay_refresh: bool,
    pub nsm_active: bool,
    pub cancelable_action_msg: String,

    pub slots: Vec<RackSlot>,
    pub history: EditHistory,
    pub history_plugin_count: usize,
    pub patchbay: PatchbayState,
    pub selected_patchbay_plugins: Vec<u32>,
    pub canvas_peak_in: PeakMeterState,
    pub canvas_peak_out: PeakMeterState,
    pub canvas_active_notes: HashSet<u8>,

    pub current_dir: PathBuf,
    pub selected_folder_idx: usize,
    pub last_log_count: usize,
    pub pending: PendingUiSignals,
}

impl FrancaCore {
    pub fn new() -> Self {
        let (bin_dir, res_dir) = resolve_carla_paths();
        let settings = FrancaSettings::load();
        let mut db = PluginDatabase::load();

        let standalone_path = bin_dir.join(lib_filename("carla_standalone2"));
        let host = match CarlaHost::load(&standalone_path) {
            Ok(h) => Some(h),
            Err(e) => {
                logging::log_error(&e);
                None
            }
        };

        let utils_path = bin_dir.join(lib_filename("carla_utils"));
        let utils = match CarlaUtils::load(&utils_path) {
            Ok(u) => Some(u),
            Err(e) => {
                logging::log_warn(&format!("Could not load carla_utils: {e}"));
                None
            }
        };

        let supported_exts = utils
            .as_ref()
            .map(|u| u.get_supported_file_extensions())
            .unwrap_or_default();
        if let Some(ref u) = utils {
            db.ensure_cached_builtins(u, &settings);
        }

        let mut patchbay =
            PatchbayState::new(settings.canvas_width as f32, settings.canvas_height as f32);
        patchbay.use_bezier = settings.canvas_use_bezier;
        patchbay.auto_hide_groups = settings.canvas_auto_hide_groups;

        let nsm_active = if std::env::var("NSM_URL").is_ok() {
            host.as_ref()
                .is_some_and(|h| h.nsm_init(std::process::id() as u64, "franca"))
        } else {
            false
        };

        let transport_link = settings.transport_extra.contains(":link:");
        let initial_external_patchbay = settings.process_mode != ENGINE_PROCESS_MODE_PATCHBAY;

        let mut core = Self {
            host,
            utils,
            bin_dir,
            res_dir,
            process_mode: settings.process_mode,
            transport_mode: settings.transport_mode,
            settings,
            db,
            supported_exts,
            discovery_runner: None,
            epoch: Instant::now(),
            last_slow_poll: Instant::now(),
            engine_running: false,
            running_driver_name: String::new(),
            buffer_size: 512,
            sample_rate: 44100.0,
            runtime_info: CarlaRuntimeEngineInfo::default(),
            runtime_dev_info: RuntimeDriverDeviceInfo::default(),
            transport_info: CarlaTransportInfo::default(),
            transport_link,
            project_filename: None,
            is_project_loading: false,
            removing_all_plugins: false,
            external_patchbay: initial_external_patchbay,
            needs_patchbay_refresh: false,
            nsm_active,
            cancelable_action_msg: String::new(),
            slots: Vec::new(),
            history: EditHistory::new(),
            history_plugin_count: 0,
            patchbay,
            selected_patchbay_plugins: Vec::new(),
            canvas_peak_in: PeakMeterState::new(2),
            canvas_peak_out: PeakMeterState::new(2),
            canvas_active_notes: HashSet::new(),
            current_dir: dirs_home(),
            selected_folder_idx: 0,
            last_log_count: 0,
            pending: PendingUiSignals {
                rack_dirty: true,
                patchbay_dirty: true,
                logs_dirty: true,
                files_dirty: true,
                discovery_dirty: true,
                ..Default::default()
            },
        };

        if !nsm_active {
            core.start_engine(true);
            let init_proj = INITIAL_PROJECT.lock().ok().and_then(|mut g| g.take());
            if let Some(proj) = init_proj {
                core.load_project_or_file(&proj);
            }
        }
        core
    }

    pub fn window_title(&self) -> String {
        match self
            .project_filename
            .as_ref()
            .and_then(|p| p.file_name())
            .and_then(|n| n.to_str())
        {
            Some(name) => format!("Franca — {name}"),
            None => "Franca".to_string(),
        }
    }

    pub fn start_engine(&mut self, silent: bool) {
        let Some(ref host) = self.host else {
            if !silent {
                self.pending.alerts.push((
                    "Backend Error".into(),
                    "libcarla_standalone2 could not be loaded.".into(),
                ));
            }
            return;
        };
        if host.is_engine_running() {
            return;
        }
        self.settings.apply_to_host(host, &self.bin_dir, &self.res_dir);
        let driver = self.settings.audio_driver.clone();
        let client_name = std::env::var("CARLA_CLIENT_NAME").unwrap_or_else(|_| "Franca".into());
        logging::log_ui(&format!("Starting audio engine ({driver})..."));
        if !host.engine_init(&driver, &client_name) {
            let err = host.get_last_error();
            logging::log_error(&format!("Failed to start engine ({driver}): {err}"));
            if !silent {
                self.pending.alerts.push((
                    "Engine Error".into(),
                    format!("Failed to start audio engine:\n{err}"),
                ));
            }
            return;
        }
        self.drain_engine_events();
        self.refresh_runtime_state();
        self.refresh_patchbay();
        self.pending.rack_dirty = true;
    }

    pub fn stop_engine(&mut self) {
        let Some(ref host) = self.host else { return };
        if !host.is_engine_running() {
            return;
        }
        logging::log_ui("Stopping audio engine...");
        let _ = host.set_engine_about_to_close();
        let _ = host.engine_close();
        self.drain_engine_events();
        self.engine_running = false;
        self.slots.clear();
        self.selected_patchbay_plugins.clear();
        self.patchbay.clear();
        self.history.clear();
        self.history_plugin_count = 0;
        self.pending.rack_dirty = true;
        self.pending.patchbay_dirty = true;
    }

    pub fn shutdown_clean(&mut self) {
        self.engine_running = false;
        self.nsm_active = false;
        if let (Some(mut runner), Some(utils)) = (self.discovery_runner.take(), self.utils.as_ref()) {
            runner.cancel(utils);
        }
        let _ = self.settings.save();
        if let Some(ref host) = self.host {
            host.finish_cancelable_ui();
            if host.is_engine_running() {
                let _ = host.set_engine_about_to_close();
                let _ = host.engine_close();
            }
        }
        self.slots.clear();
        self.patchbay.clear();
        logging::log_shutdown();
    }

    pub fn refresh_runtime_state(&mut self) {
        let Some(ref host) = self.host else { return };
        self.engine_running = host.is_engine_running();
        if self.engine_running {
            self.buffer_size = host.get_buffer_size();
            self.sample_rate = host.get_sample_rate();
            self.runtime_info = host.get_runtime_engine_info();
            self.runtime_dev_info = host.get_runtime_engine_driver_device_info();
            self.transport_info = host.get_transport_info();
            if self.running_driver_name.is_empty() {
                self.running_driver_name = self.settings.audio_driver.clone();
            }
        }
    }

    pub fn poll_tick(&mut self) {
        if SIG_SAVE_REQUESTED.swap(false, Ordering::SeqCst) {
            if let Some(path) = self.project_filename.clone() {
                self.save_project_to(&path);
            }
        }
        if SIG_QUIT_REQUESTED.swap(false, Ordering::SeqCst) {
            self.pending.quit_requested = true;
        }

        if self.engine_running || self.nsm_active {
            if let Some(ref host) = self.host {
                if host.is_engine_running() {
                    host.engine_idle();
                }
            }
        }
        self.drain_engine_events();

        // Step active plugin discovery
        if self.discovery_runner.is_some() {
            let mut finished = false;
            let mut cancelled = false;
            if let (Some(runner), Some(utils)) = (self.discovery_runner.as_mut(), self.utils.as_ref()) {
                for _ in 0..8 {
                    if runner.step(utils) {
                        break;
                    }
                }
                finished = runner.finished;
                cancelled = runner.cancelled;
            }
            self.pending.discovery_dirty = true;
            if finished {
                if !cancelled {
                    if let Some(ref runner) = self.discovery_runner {
                        runner.apply_to_database(&mut self.db);
                    }
                }
                self.discovery_runner = None;
            }
        }

        let now_secs = monotonic_secs(self.epoch);
        if self.engine_running {
            if let Some(ref host) = self.host {
                for slot in &mut self.slots {
                    slot.update_peaks(host, now_secs);
                }
                if let Some(&first_pid) = self.selected_patchbay_plugins.first() {
                    self.canvas_peak_in
                        .update_channel(0, host.get_input_peak_value(first_pid, true), now_secs);
                    self.canvas_peak_in
                        .update_channel(1, host.get_input_peak_value(first_pid, false), now_secs);
                    self.canvas_peak_out
                        .update_channel(0, host.get_output_peak_value(first_pid, true), now_secs);
                    self.canvas_peak_out
                        .update_channel(1, host.get_output_peak_value(first_pid, false), now_secs);
                }
                self.transport_info = host.get_transport_info();
                if self.last_slow_poll.elapsed() >= Duration::from_millis(250) {
                    self.last_slow_poll = Instant::now();
                    self.runtime_info = host.get_runtime_engine_info();
                }
            }
        }

        let log_count = logging::ui_lines_snapshot().len();
        if log_count != self.last_log_count {
            self.last_log_count = log_count;
            self.pending.logs_dirty = true;
        }
    }

    pub fn drain_engine_events(&mut self) {
        let events = self
            .host
            .as_ref()
            .map(|h| h.drain_events())
            .unwrap_or_default();
        for ev in events {
            self.handle_engine_event(ev);
        }
        if self.needs_patchbay_refresh {
            self.needs_patchbay_refresh = false;
            self.refresh_patchbay();
        }
    }

    fn handle_engine_event(&mut self, ev: EngineEvent) {
        match ev.action {
            ENGINE_CALLBACK_DEBUG => {
                if !ev.value_str.is_empty() {
                    logging::log_info(&ev.value_str);
                }
            }
            ENGINE_CALLBACK_INFO => {
                logging::log_ui(&ev.value_str);
                self.pending
                    .alerts
                    .push(("Information".into(), ev.value_str));
            }
            ENGINE_CALLBACK_ERROR => {
                logging::log_error(&ev.value_str);
                self.pending
                    .alerts
                    .push(("Engine Error".into(), ev.value_str));
            }
            ENGINE_CALLBACK_ENGINE_STARTED => {
                self.engine_running = true;
                self.process_mode = ev.value1;
                self.transport_mode = ev.value2;
                self.buffer_size = ev.value3.max(0) as u32;
                self.sample_rate = ev.valuef as f64;
                // Only Patchbay process mode has an internal plugin routing graph;
                // Continuous Rack / Single / Multiple Client modes always use the external patchbay.
                self.external_patchbay = self.process_mode != ENGINE_PROCESS_MODE_PATCHBAY;
                self.running_driver_name = if !ev.value_str.is_empty() {
                    ev.value_str
                } else {
                    self.settings.audio_driver.clone()
                };
                if let Some(ref host) = self.host {
                    self.runtime_dev_info = host.get_runtime_engine_driver_device_info();
                }
                self.history.clear();
                self.history.set_blocked(false);
                self.history_plugin_count = self.slots.len();
                logging::log_ui(&format!(
                    "Engine started: {} (mode={}, buffer={}, rate={:.0} Hz)",
                    self.running_driver_name,
                    process_mode_label(self.process_mode),
                    self.buffer_size,
                    self.sample_rate
                ));
            }
            ENGINE_CALLBACK_ENGINE_STOPPED => {
                self.engine_running = false;
                self.slots.clear();
                self.patchbay.clear();
                self.history.clear();
                self.history.set_blocked(false);
                self.history_plugin_count = 0;
                self.pending.rack_dirty = true;
                self.pending.patchbay_dirty = true;
                logging::log_ui("Engine stopped");
            }
            ENGINE_CALLBACK_PROCESS_MODE_CHANGED => self.process_mode = ev.value1,
            ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED => self.transport_mode = ev.value1,
            ENGINE_CALLBACK_BUFFER_SIZE_CHANGED => {
                self.buffer_size = ev.value1.max(0) as u32;
                if let Some(ref host) = self.host {
                    self.runtime_dev_info = host.get_runtime_engine_driver_device_info();
                }
            }
            ENGINE_CALLBACK_SAMPLE_RATE_CHANGED => {
                self.sample_rate = ev.valuef as f64;
                if let Some(ref host) = self.host {
                    self.runtime_dev_info = host.get_runtime_engine_driver_device_info();
                }
            }
            ENGINE_CALLBACK_CANCELABLE_ACTION => {
                let starting = ev.value1 != 0;
                self.history.set_blocked(starting);
                self.cancelable_action_msg = if starting {
                    if ev.value_str.is_empty() {
                        "Processing...".into()
                    } else {
                        ev.value_str
                    }
                } else {
                    String::new()
                };
            }
            ENGINE_CALLBACK_PROJECT_LOAD_FINISHED => {
                self.is_project_loading = false;
                self.history.clear();
                self.history.set_blocked(false);
                self.history_plugin_count = self.slots.len();
                self.needs_patchbay_refresh = true;
                self.pending.rack_dirty = true;
                self.pending.patchbay_dirty = true;
                logging::log_ui("Project load finished");
            }
            ENGINE_CALLBACK_PLUGIN_ADDED => {
                let pid = ev.plugin_id;
                let before = self.history_plugin_count;
                if let Some(ref host) = self.host {
                    let slot = RackSlot::from_host(host, pid);
                    let pname = slot.info.name.clone();
                    if (pid as usize) < self.slots.len() {
                        self.slots[pid as usize] = slot;
                    } else {
                        self.slots.push(slot);
                    }
                    self.history_plugin_count = self.slots.len();
                    logging::log_ui(&format!("Plugin added: {pname} (id={pid})"));
                    if !self.history.busy && !self.history.blocked && !self.is_project_loading {
                        if self.history_plugin_count == before + 1 && self.can_restore_plugin(pid) {
                            self.history.record(
                                "Add Effect",
                                HistoryAction::AddPlugin {
                                    plugin_id: pid,
                                    snapshot: None,
                                },
                                None,
                            );
                        } else {
                            self.history.clear();
                        }
                    }
                    self.pending.rack_dirty = true;
                }
            }
            ENGINE_CALLBACK_PLUGIN_REMOVED => {
                let pid = ev.plugin_id as usize;
                self.patchbay.handle_plugin_removed(ev.plugin_id);
                if pid < self.slots.len() {
                    let removed = self.slots.remove(pid);
                    logging::log_ui(&format!("Plugin removed: {}", removed.info.name));
                    for (i, slot) in self.slots.iter_mut().enumerate().skip(pid) {
                        slot.plugin_id = i as u32;
                    }
                }
                self.history_plugin_count = self.slots.len();
                if !self.history.busy {
                    self.history.clear();
                }
                if self.slots.is_empty() && self.removing_all_plugins {
                    self.removing_all_plugins = false;
                    self.is_project_loading = false;
                }
                self.pending.rack_dirty = true;
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PLUGIN_RENAMED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.info.name = ev.value_str;
                    self.pending.rack_dirty = true;
                    self.pending.plugin_edit_dirty_ids.push(ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_PLUGIN_UNAVAILABLE => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    logging::log_error(&format!("{}: {}", slot.info.name, ev.value_str));
                    slot.unavailable_error = Some(ev.value_str);
                    self.pending.rack_dirty = true;
                }
            }
            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    if ev.value1 == PARAMETER_ACTIVE {
                        slot.active = ev.valuef >= 0.5;
                        if !slot.active {
                            slot.clip_warning.clear();
                        }
                        self.pending.rack_dirty = true;
                    }
                    self.pending.plugin_edit_dirty_ids.push(ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED
            | ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED
            | ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED
            | ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED
            | ENGINE_CALLBACK_PROGRAM_CHANGED
            | ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED
            | ENGINE_CALLBACK_RELOAD_PARAMETERS
            | ENGINE_CALLBACK_RELOAD_PROGRAMS => {
                self.pending.plugin_edit_dirty_ids.push(ev.plugin_id);
            }
            ENGINE_CALLBACK_OPTION_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    let bit = ev.value1 as u32;
                    if ev.value2 != 0 {
                        slot.info.options_enabled |= bit;
                    } else {
                        slot.info.options_enabled &= !bit;
                    }
                    self.pending.plugin_edit_dirty_ids.push(ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_UI_STATE_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.custom_ui_visible = ev.value1 == 1;
                    slot.custom_ui_failed = ev.value1 == -1;
                    if ev.value1 == -1 {
                        logging::log_ui(&format!("Editor unavailable for {}", slot.info.name));
                    }
                    self.pending.rack_dirty = true;
                }
            }
            ENGINE_CALLBACK_NOTE_ON => {
                let note = ev.value2.clamp(0, 127) as u8;
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.active_notes.insert(note);
                }
                if self.selected_patchbay_plugins.contains(&ev.plugin_id) {
                    self.canvas_active_notes.insert(note);
                }
            }
            ENGINE_CALLBACK_NOTE_OFF => {
                let note = ev.value2.clamp(0, 127) as u8;
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.active_notes.remove(&note);
                }
                if self.selected_patchbay_plugins.contains(&ev.plugin_id) {
                    self.canvas_active_notes.remove(&note);
                }
            }
            ENGINE_CALLBACK_RELOAD_INFO | ENGINE_CALLBACK_UPDATE | ENGINE_CALLBACK_RELOAD_ALL => {
                if let (Some(host), Some(slot)) =
                    (self.host.as_ref(), self.slots.get_mut(ev.plugin_id as usize))
                {
                    slot.reload_info(host);
                    self.pending.rack_dirty = true;
                    self.pending.plugin_edit_dirty_ids.push(ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED => {
                self.patchbay
                    .add_group(ev.plugin_id, ev.value2, ev.value1, ev.value_str);
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED => {
                self.patchbay.remove_group(ev.plugin_id);
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED => {
                self.patchbay.rename_group(ev.plugin_id, ev.value_str);
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED => {
                self.patchbay
                    .set_group_data(ev.plugin_id, ev.value2, ev.value1);
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED => {
                self.patchbay.set_group_position(
                    ev.plugin_id,
                    ev.value1,
                    ev.value2,
                    ev.value3,
                    ev.valuef as i32,
                );
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_PORT_ADDED => {
                self.patchbay.add_port(
                    ev.plugin_id,
                    ev.value1 as u32,
                    ev.value2 as u32,
                    ev.value_str,
                );
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED => {
                self.patchbay.remove_port(ev.plugin_id, ev.value1 as u32);
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED => {
                self.patchbay.update_port(
                    ev.plugin_id,
                    ev.value1 as u32,
                    ev.value2 as u32,
                    ev.value_str,
                );
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED => {
                let parts: Vec<u32> = ev
                    .value_str
                    .split(':')
                    .filter_map(|s| s.parse::<u32>().ok())
                    .collect();
                if parts.len() == 4 {
                    self.patchbay.add_connection(
                        ev.plugin_id,
                        parts[0],
                        parts[1],
                        parts[2],
                        parts[3],
                    );
                    self.pending.patchbay_dirty = true;
                }
            }
            ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED => {
                self.patchbay.remove_connection(ev.plugin_id);
                self.pending.patchbay_dirty = true;
            }
            ENGINE_CALLBACK_NSM => self.handle_nsm_event(ev.value1, ev.value_str),
            ENGINE_CALLBACK_QUIT => self.pending.quit_requested = true,
            _ => {}
        }
    }

    fn handle_nsm_event(&mut self, opcode: i32, value_str: String) {
        match opcode {
            NSM_CALLBACK_INIT => {
                if let Some(ref host) = self.host {
                    host.nsm_ready(NSM_CALLBACK_INIT);
                }
            }
            NSM_CALLBACK_OPEN => {
                let parts: Vec<&str> = value_str.split(':').collect();
                if let Some(path) = parts.first() {
                    let proj_dir = PathBuf::from(path);
                    let _ = fs::create_dir_all(&proj_dir);
                    let proj_file = proj_dir.join("project.carxp");
                    if !self.engine_running {
                        self.start_engine(true);
                    }
                    if proj_file.exists() {
                        self.load_project(&proj_file);
                    }
                    self.project_filename = Some(proj_file);
                }
                if let Some(ref host) = self.host {
                    host.nsm_report_result(NSM_CALLBACK_OPEN, true);
                }
            }
            NSM_CALLBACK_SAVE => {
                let ok = self
                    .project_filename
                    .clone()
                    .is_some_and(|p| self.save_project_to(&p));
                if let Some(ref host) = self.host {
                    host.nsm_report_result(NSM_CALLBACK_SAVE, ok);
                }
            }
            _ => {}
        }
    }

    fn can_restore_plugin(&self, plugin_id: u32) -> bool {
        self.process_mode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK
            && self
                .slots
                .get(plugin_id as usize)
                .is_some_and(|s| s.info.ptype != PLUGIN_JACK)
    }

    // --- File operations ---

    pub fn file_new(&mut self) {
        if !self.engine_running {
            return;
        }
        self.history.clear();
        if let Some(ref host) = self.host {
            if !self.slots.is_empty() {
                self.removing_all_plugins = true;
                let _ = host.remove_all_plugins();
            }
            host.clear_project_filename();
        }
        self.project_filename = None;
        self.drain_engine_events();
        self.pending.rack_dirty = true;
        self.pending.patchbay_dirty = true;
        logging::log_ui("New project created");
    }

    pub fn file_open_dialog(&mut self) {
        let mut dlg = rfd::FileDialog::new()
            .set_title("Open Franca Project")
            .add_filter("Carla/Franca Projects", &["carxp"]);
        if Path::new(&self.settings.project_folder).exists() {
            dlg = dlg.set_directory(&self.settings.project_folder);
        }
        if let Some(path) = dlg.pick_file() {
            self.load_project(&path);
        }
    }

    pub fn load_project(&mut self, path: &Path) {
        if !self.engine_running {
            return;
        }
        self.history.clear();
        self.is_project_loading = true;
        let path_str = path.to_string_lossy().into_owned();
        let res = self.host.as_ref().map(|h| {
            let ok = h.load_project(&path_str);
            h.finish_cancelable_ui();
            (ok, h.get_last_error())
        });
        if let Some((ok, err)) = res {
            if !ok {
                self.is_project_loading = false;
                logging::log_error(&format!("Failed to load project {path_str}: {err}"));
                self.pending
                    .alerts
                    .push(("Load Project Failed".into(), format!("Could not load project:\n{err}")));
            } else {
                self.project_filename = Some(path.to_path_buf());
                logging::log_ui(&format!("Loaded project: {path_str}"));
            }
        }
        self.drain_engine_events();
        self.is_project_loading = false;
        self.history.set_blocked(false);
        self.pending.rack_dirty = true;
        self.pending.patchbay_dirty = true;
    }

    pub fn load_project_or_file(&mut self, path: &Path) {
        if !self.engine_running {
            return;
        }
        if is_project_file(path) {
            self.load_project(path);
        } else {
            let s = path.to_string_lossy().into_owned();
            if let Some(ref host) = self.host {
                let ok = host.load_file(&s);
                host.finish_cancelable_ui();
                if !ok {
                    let err = host.get_last_error();
                    self.pending
                        .alerts
                        .push(("Load File Failed".into(), format!("Could not load file:\n{err}")));
                } else {
                    logging::log_ui(&format!("Loaded file: {s}"));
                }
            }
            self.drain_engine_events();
        }
    }

    pub fn file_save(&mut self) {
        if let Some(path) = self.project_filename.clone() {
            self.save_project_to(&path);
        } else {
            self.file_save_as_dialog();
        }
    }

    pub fn file_save_as_dialog(&mut self) {
        let mut dlg = rfd::FileDialog::new()
            .set_title("Save Franca Project")
            .add_filter("Carla/Franca Projects", &["carxp"])
            .set_file_name("project.carxp");
        if Path::new(&self.settings.project_folder).exists() {
            dlg = dlg.set_directory(&self.settings.project_folder);
        }
        if let Some(mut path) = dlg.save_file() {
            if path.extension().is_none() {
                path.set_extension("carxp");
            }
            self.save_project_to(&path);
        }
    }

    pub fn save_project_to(&mut self, path: &Path) -> bool {
        if !self.engine_running {
            return false;
        }
        let s = path.to_string_lossy().into_owned();
        if let Some(ref host) = self.host {
            if !host.save_project(&s) {
                let err = host.get_last_error();
                logging::log_error(&format!("Save project failed: {err}"));
                self.pending
                    .alerts
                    .push(("Save Project Failed".into(), format!("Could not save project:\n{err}")));
                return false;
            }
            self.project_filename = Some(path.to_path_buf());
            logging::log_ui(&format!("Saved project: {s}"));
            return true;
        }
        false
    }

    // --- Plugin actions with EditHistory ---

    fn move_plugin_raw(&mut self, source: u32, target: u32) -> u32 {
        let count = self.slots.len() as u32;
        if source == target || source >= count || target >= count {
            return source;
        }
        let Some(ref host) = self.host else { return source };
        let mut cur = source;
        let step: i32 = if target > source { 1 } else { -1 };
        while cur != target {
            let next = (cur as i32 + step) as u32;
            if !host.switch_plugins(cur, next) {
                let err = host.get_last_error();
                self.pending
                    .alerts
                    .push(("Move Failed".into(), format!("Could not move plugin:\n{err}")));
                break;
            }
            cur = next;
        }
        if cur != source {
            let item = self.slots.remove(source as usize);
            self.slots.insert(cur as usize, item);
            for (i, slot) in self.slots.iter_mut().enumerate() {
                slot.plugin_id = i as u32;
            }
            self.refresh_patchbay();
            self.pending.rack_dirty = true;
        }
        cur
    }

    pub fn move_plugin(&mut self, source: u32, target: u32) {
        let moved = self.move_plugin_raw(source, target);
        if moved != source {
            self.history.record(
                "Move Effect",
                HistoryAction::MovePlugin {
                    from: source,
                    to: moved,
                },
                None,
            );
        }
    }

    pub fn rename_plugin(&mut self, plugin_id: u32, new_name: &str) {
        let Some(before) = self.slots.get(plugin_id as usize).map(|s| s.info.name.clone()) else {
            return;
        };
        let ok = self
            .host
            .as_ref()
            .is_some_and(|h| h.rename_plugin(plugin_id, new_name));
        if !ok {
            let err = self.host.as_ref().map(|h| h.get_last_error()).unwrap_or_default();
            self.pending
                .alerts
                .push(("Rename Failed".into(), format!("Could not rename plugin:\n{err}")));
            return;
        }
        self.drain_engine_events();
        let after = self
            .host
            .as_ref()
            .map(|h| h.get_plugin_info(plugin_id).name)
            .unwrap_or_else(|| new_name.to_string());
        if let Some(slot) = self.slots.get_mut(plugin_id as usize) {
            slot.info.name = after.clone();
        }
        self.pending.rack_dirty = true;
        self.pending.plugin_edit_dirty_ids.push(plugin_id);
        if before != after {
            self.history.record(
                "Rename Effect",
                HistoryAction::RenamePlugin {
                    plugin_id,
                    before,
                    after,
                },
                None,
            );
        }
    }

    pub fn set_plugin_active(&mut self, plugin_id: u32, active: bool) {
        let Some(ref host) = self.host else { return };
        let before = host.get_internal_parameter_value(plugin_id, PARAMETER_ACTIVE) >= 0.5;
        host.set_active(plugin_id, active);
        let after = host.get_internal_parameter_value(plugin_id, PARAMETER_ACTIVE) >= 0.5;
        if let Some(slot) = self.slots.get_mut(plugin_id as usize) {
            slot.active = after;
            if !after {
                slot.clip_warning.clear();
            }
        }
        self.pending.rack_dirty = true;
        self.pending.plugin_edit_dirty_ids.push(plugin_id);
        if before != after {
            self.history.record(
                if after { "Enable Effect" } else { "Disable Effect" },
                HistoryAction::SetActive {
                    plugin_id,
                    before,
                    after,
                },
                None,
            );
        }
    }

    pub fn set_plugin_internal_param(&mut self, plugin_id: u32, param_id: i32, value: f32) {
        let Some(ref host) = self.host else { return };
        let before = host.get_internal_parameter_value(plugin_id, param_id);
        match param_id {
            PARAMETER_DRYWET => host.set_drywet(plugin_id, value),
            PARAMETER_VOLUME => host.set_volume(plugin_id, value),
            PARAMETER_BALANCE_LEFT => host.set_balance_left(plugin_id, value),
            PARAMETER_BALANCE_RIGHT => host.set_balance_right(plugin_id, value),
            PARAMETER_PANNING => host.set_panning(plugin_id, value),
            _ => return,
        }
        let after = host.get_internal_parameter_value(plugin_id, param_id);
        self.pending.plugin_edit_dirty_ids.push(plugin_id);
        if (before - after).abs() > 1e-7 {
            self.history.record(
                "Change Parameter",
                HistoryAction::SetInternalParam {
                    plugin_id,
                    param_id,
                    before,
                    after,
                },
                Some(MergeKey::Param(plugin_id, param_id)),
            );
        }
    }

    pub fn set_plugin_param(&mut self, plugin_id: u32, param_id: u32, value: f32) {
        let Some(ref host) = self.host else { return };
        let before = host.get_current_parameter_value(plugin_id, param_id);
        host.set_parameter_value(plugin_id, param_id, value);
        let after = host.get_current_parameter_value(plugin_id, param_id);
        self.pending.plugin_edit_dirty_ids.push(plugin_id);
        if (before - after).abs() > 1e-7 {
            self.history.record(
                "Change Parameter",
                HistoryAction::SetParam {
                    plugin_id,
                    param_id,
                    before,
                    after,
                },
                Some(MergeKey::Param(plugin_id, param_id as i32)),
            );
        }
    }

    pub fn change_plugin_params_batch(
        &mut self,
        plugin_id: u32,
        label: &str,
        op: impl FnOnce(&CarlaHost, u32),
    ) {
        let Some(ref host) = self.host else { return };
        let count = host.get_parameter_count(plugin_id);
        let mut inputs = Vec::new();
        for i in 0..count {
            if host.get_parameter_data(plugin_id, i).ptype == PARAMETER_INPUT {
                inputs.push((i, host.get_current_parameter_value(plugin_id, i)));
            }
        }
        op(host, plugin_id);
        let mut before = Vec::new();
        let mut after = Vec::new();
        for (i, bv) in inputs {
            let av = host.get_current_parameter_value(plugin_id, i);
            if (bv - av).abs() > 1e-7 {
                before.push((i, bv));
                after.push((i, av));
            }
        }
        self.pending.plugin_edit_dirty_ids.push(plugin_id);
        if !before.is_empty() {
            self.history.record(
                label,
                HistoryAction::SetParamsBatch {
                    plugin_id,
                    before,
                    after,
                },
                None,
            );
        }
    }

    pub fn set_plugin_category(&mut self, plugin_id: u32, category: i32) {
        let Some(ref host) = self.host else { return };
        let Some(slot) = self.slots.get_mut(plugin_id as usize) else {
            return;
        };
        let before = slot.category;
        if before == category {
            return;
        }
        slot.set_category(host, category, true);
        self.pending.rack_dirty = true;
        self.history.record(
            "Set Category",
            HistoryAction::SetCategory {
                plugin_id,
                before,
                after: category,
            },
            None,
        );
    }

    pub fn remove_plugin(&mut self, plugin_id: u32) {
        if self.can_restore_plugin(plugin_id) {
            self.history.busy = true;
            let snap = self
                .host
                .as_ref()
                .ok_or_else(|| "Host unavailable".to_string())
                .and_then(|h| PluginSnapshot::capture(h, plugin_id));
            let ok = if snap.is_ok() {
                let res = self.host.as_ref().is_some_and(|h| h.remove_plugin(plugin_id));
                self.drain_engine_events();
                res
            } else {
                false
            };
            self.history.busy = false;
            if ok {
                self.history.record(
                    "Remove Effect",
                    HistoryAction::RemovePlugin {
                        plugin_id,
                        snapshot: snap.ok(),
                    },
                    None,
                );
                return;
            }
        }
        if let Some(ref host) = self.host {
            if !host.remove_plugin(plugin_id) {
                let err = host.get_last_error();
                self.pending
                    .alerts
                    .push(("Remove Failed".into(), format!("Could not remove plugin:\n{err}")));
            }
        }
        self.drain_engine_events();
    }

    pub fn remove_all_plugins(&mut self) {
        self.history.clear();
        self.removing_all_plugins = true;
        if let Some(ref host) = self.host {
            let _ = host.remove_all_plugins();
        }
        self.drain_engine_events();
    }

    pub fn add_discovered_plugin(&mut self, p: &DiscoveredPlugin, replace_id: Option<u32>) {
        if !self.engine_running {
            self.pending.alerts.push((
                "Engine Stopped".into(),
                "Start the audio engine before adding plugins.".into(),
            ));
            return;
        }
        if let Some(ref host) = self.host {
            if let Some(rid) = replace_id {
                let _ = host.replace_plugin(rid);
            }
            let name_opt = if p.name.is_empty() {
                None
            } else {
                Some(p.name.as_str())
            };
            let (filename, label) = if p.ptype == PLUGIN_LV2 {
                crate::ffi::utils::normalize_lv2_bundle_and_uri(&p.filename, &p.label)
            } else {
                (p.filename.clone(), p.label.clone())
            };
            let ok = host.add_plugin(
                p.build,
                p.ptype,
                &filename,
                name_opt,
                &label,
                p.unique_id as i64,
                PLUGIN_OPTIONS_NULL,
            );
            host.finish_cancelable_ui();
            if !ok {
                let err = host.get_last_error();
                logging::log_error(&format!("Failed to load plugin {}: {err}", p.name));
                self.pending.alerts.push((
                    "Failed to Load Plugin".into(),
                    format!("Could not load '{}':\n{err}", p.name),
                ));
            }
        }
        self.drain_engine_events();
    }

    // --- Patchbay actions ---

    pub fn switch_patchbay_view(&mut self, external: bool) {
        if self.external_patchbay == external && !self.patchbay.groups.is_empty() {
            return;
        }
        self.external_patchbay = external;
        self.patchbay.clear();
        if self.engine_running {
            if let Some(ref host) = self.host {
                let _ = host.patchbay_refresh(external);
            }
            self.drain_engine_events();
        }
        self.pending.patchbay_dirty = true;
    }

    pub fn refresh_patchbay(&mut self) {
        if !self.engine_running {
            return;
        }
        self.patchbay.clear();
        if let Some(ref host) = self.host {
            let _ = host.patchbay_refresh(self.external_patchbay);
        }
        self.drain_engine_events();
        self.pending.patchbay_dirty = true;
    }

    pub fn connect_patchbay_ports(&mut self, g_out: u32, p_out: u32, g_in: u32, p_in: u32) {
        let ep_out = self.patchbay.endpoint_for(g_out, p_out);
        let ep_in = self.patchbay.endpoint_for(g_in, p_in);
        let ok = self.host.as_ref().is_some_and(|h| {
            h.patchbay_connect(self.external_patchbay, g_out, p_out, g_in, p_in)
        });
        if !ok {
            return;
        }
        self.drain_engine_events();
        if let (Some(output), Some(input)) = (ep_out, ep_in) {
            self.history.record(
                "Connect Ports",
                HistoryAction::PatchbayLink {
                    external: self.external_patchbay,
                    output,
                    input,
                    connected: true,
                },
                None,
            );
        }
    }

    pub fn disconnect_patchbay_ports(&mut self, connection_id: u32) {
        let eps = self
            .patchbay
            .connections
            .iter()
            .find(|c| c.connection_id == connection_id)
            .and_then(|c| {
                Some((
                    self.patchbay.endpoint_for(c.group_out, c.port_out)?,
                    self.patchbay.endpoint_for(c.group_in, c.port_in)?,
                ))
            });
        let ok = self
            .host
            .as_ref()
            .is_some_and(|h| h.patchbay_disconnect(self.external_patchbay, connection_id));
        if !ok {
            return;
        }
        self.drain_engine_events();
        if let Some((output, input)) = eps {
            self.history.record(
                "Disconnect Ports",
                HistoryAction::PatchbayLink {
                    external: self.external_patchbay,
                    output,
                    input,
                    connected: false,
                },
                None,
            );
        }
    }

    pub fn arrange_patchbay(&mut self) {
        let before = self.patchbay.capture_positions();
        let updates = self.patchbay.arrange_nodes();
        if let Some(ref host) = self.host {
            for (gid, x1, y1, x2, y2) in updates {
                let _ = host.patchbay_set_group_pos(self.external_patchbay, gid, x1, y1, x2, y2);
            }
        }
        let after = self.patchbay.capture_positions();
        if before != after {
            self.history.record(
                "Arrange Patchbay",
                HistoryAction::PatchbayArrange {
                    external: self.external_patchbay,
                    before,
                    after,
                },
                None,
            );
        }
        self.pending.patchbay_dirty = true;
    }

    pub fn commit_group_position(&mut self, group_id: u32) {
        if !self.engine_running || self.is_project_loading {
            return;
        }
        if let (Some(host), Some(g)) = (self.host.as_ref(), self.patchbay.groups.get(&group_id)) {
            let (x2, y2) = if g.split {
                (g.pos2.x.round() as i32, g.pos2.y.round() as i32)
            } else {
                (0, 0)
            };
            let _ = host.patchbay_set_group_pos(
                self.external_patchbay,
                group_id,
                g.pos1.x.round() as i32,
                g.pos1.y.round() as i32,
                x2,
                y2,
            );
        }
    }

    // --- Undo / Redo ---

    pub fn perform_undo(&mut self) {
        if !self.history.can_undo() {
            return;
        }
        self.history.busy = true;
        let err = self.apply_history_step(true);
        self.history.busy = false;
        if let Some(msg) = err {
            self.history.revert_undo_index();
            self.pending.alerts.push(("Undo Failed".into(), msg));
        }
    }

    pub fn perform_redo(&mut self) {
        if !self.history.can_redo() {
            return;
        }
        self.history.busy = true;
        let err = self.apply_history_step(false);
        self.history.busy = false;
        if let Some(msg) = err {
            self.history.revert_redo_index();
            self.pending.alerts.push(("Redo Failed".into(), msg));
        }
    }

    fn restore_plugin_snapshot(&mut self, plugin_id: u32, snap: &PluginSnapshot) -> Result<(), String> {
        let prev_count = self.slots.len() as u32;
        let path_str = snap.carxp_path().to_string_lossy().into_owned();
        let ok = self.host.as_ref().is_some_and(|h| h.load_file(&path_str));
        if !ok {
            let err = self.host.as_ref().map(|h| h.get_last_error()).unwrap_or_default();
            self.drain_engine_events();
            if self.slots.len() as u32 > prev_count {
                if let Some(ref host) = self.host {
                    let _ = host.remove_plugin(prev_count);
                }
                self.drain_engine_events();
            }
            return Err(err);
        }
        self.drain_engine_events();
        if self.slots.len() as u32 != prev_count + 1 {
            return Err("The effect could not be restored.".into());
        }
        let moved = self.move_plugin_raw(prev_count, plugin_id);
        if moved != plugin_id {
            if let Some(ref host) = self.host {
                let _ = host.remove_plugin(moved);
            }
            self.drain_engine_events();
            return Err("Could not restore the effect to its place in the rack.".into());
        }
        Ok(())
    }

    fn apply_history_step(&mut self, is_undo: bool) -> Option<String> {
        let mut entry_action = {
            let entry = if is_undo {
                self.history.take_undo()?
            } else {
                self.history.take_redo()?
            };
            std::mem::replace(
                &mut entry.action,
                HistoryAction::SetCategory {
                    plugin_id: 0,
                    before: 0,
                    after: 0,
                },
            )
        };

        let result: Result<(), String> = match &mut entry_action {
            HistoryAction::MovePlugin { from, to } => {
                let (src, dst) = if is_undo { (*to, *from) } else { (*from, *to) };
                if self.move_plugin_raw(src, dst) == dst {
                    Ok(())
                } else {
                    Err("The effect could not be moved.".into())
                }
            }
            HistoryAction::RenamePlugin {
                plugin_id,
                before,
                after,
            } => {
                let target = if is_undo { before.as_str() } else { after.as_str() };
                if self.host.as_ref().is_some_and(|h| h.rename_plugin(*plugin_id, target)) {
                    self.drain_engine_events();
                    if let Some(s) = self.slots.get_mut(*plugin_id as usize) {
                        s.info.name = target.to_string();
                    }
                    self.pending.rack_dirty = true;
                    Ok(())
                } else {
                    Err("The effect could not be renamed.".into())
                }
            }
            HistoryAction::SetActive {
                plugin_id,
                before,
                after,
            } => {
                let val = if is_undo { *before } else { *after };
                if let (Some(host), Some(s)) = (self.host.as_ref(), self.slots.get_mut(*plugin_id as usize)) {
                    host.set_active(*plugin_id, val);
                    s.active = val;
                    self.pending.rack_dirty = true;
                    self.pending.plugin_edit_dirty_ids.push(*plugin_id);
                    Ok(())
                } else {
                    Err("The effect is no longer in the rack.".into())
                }
            }
            HistoryAction::SetInternalParam {
                plugin_id,
                param_id,
                before,
                after,
            } => {
                let val = if is_undo { *before } else { *after };
                if let Some(host) = self.host.as_ref() {
                    if (*plugin_id as usize) >= self.slots.len() {
                        Err("The effect is no longer in the rack.".into())
                    } else {
                        match *param_id {
                            PARAMETER_DRYWET => host.set_drywet(*plugin_id, val),
                            PARAMETER_VOLUME => host.set_volume(*plugin_id, val),
                            PARAMETER_BALANCE_LEFT => host.set_balance_left(*plugin_id, val),
                            PARAMETER_BALANCE_RIGHT => host.set_balance_right(*plugin_id, val),
                            PARAMETER_PANNING => host.set_panning(*plugin_id, val),
                            _ => {}
                        }
                        self.pending.plugin_edit_dirty_ids.push(*plugin_id);
                        Ok(())
                    }
                } else {
                    Err("Host unavailable".into())
                }
            }
            HistoryAction::SetParam {
                plugin_id,
                param_id,
                before,
                after,
            } => {
                let val = if is_undo { *before } else { *after };
                if let Some(host) = self.host.as_ref() {
                    if (*plugin_id as usize) >= self.slots.len() {
                        Err("The effect is no longer in the rack.".into())
                    } else {
                        host.set_parameter_value(*plugin_id, *param_id, val);
                        self.pending.plugin_edit_dirty_ids.push(*plugin_id);
                        Ok(())
                    }
                } else {
                    Err("Host unavailable".into())
                }
            }
            HistoryAction::SetParamsBatch {
                plugin_id,
                before,
                after,
            } => {
                let vals = if is_undo { before } else { after };
                if let Some(host) = self.host.as_ref() {
                    if (*plugin_id as usize) >= self.slots.len() {
                        Err("The effect is no longer in the rack.".into())
                    } else {
                        for &(pid, v) in vals.iter() {
                            host.set_parameter_value(*plugin_id, pid, v);
                        }
                        self.pending.plugin_edit_dirty_ids.push(*plugin_id);
                        Ok(())
                    }
                } else {
                    Err("Host unavailable".into())
                }
            }
            HistoryAction::SetCategory {
                plugin_id,
                before,
                after,
            } => {
                let val = if is_undo { *before } else { *after };
                if let (Some(host), Some(s)) = (self.host.as_ref(), self.slots.get_mut(*plugin_id as usize)) {
                    s.set_category(host, val, true);
                    self.pending.rack_dirty = true;
                    Ok(())
                } else {
                    Err("The effect is no longer in the rack.".into())
                }
            }
            HistoryAction::AddPlugin {
                plugin_id,
                snapshot,
            } => {
                if is_undo {
                    let cap = self
                        .host
                        .as_ref()
                        .ok_or_else(|| "Host unavailable".to_string())
                        .and_then(|h| PluginSnapshot::capture(h, *plugin_id));
                    match cap {
                        Ok(s) => {
                            *snapshot = Some(s);
                            if self.host.as_ref().is_some_and(|h| h.remove_plugin(*plugin_id)) {
                                self.drain_engine_events();
                                Ok(())
                            } else {
                                Err("The effect could not be removed.".into())
                            }
                        }
                        Err(e) => Err(e),
                    }
                } else if let Some(ref snap) = *snapshot {
                    self.restore_plugin_snapshot(*plugin_id, snap)
                } else {
                    Err("Missing snapshot to restore plugin.".into())
                }
            }
            HistoryAction::RemovePlugin {
                plugin_id,
                snapshot,
            } => {
                if is_undo {
                    if let Some(ref snap) = *snapshot {
                        self.restore_plugin_snapshot(*plugin_id, snap)
                    } else {
                        Err("Missing snapshot to restore plugin.".into())
                    }
                } else {
                    let cap = self
                        .host
                        .as_ref()
                        .ok_or_else(|| "Host unavailable".to_string())
                        .and_then(|h| PluginSnapshot::capture(h, *plugin_id));
                    match cap {
                        Ok(s) => {
                            *snapshot = Some(s);
                            if self.host.as_ref().is_some_and(|h| h.remove_plugin(*plugin_id)) {
                                self.drain_engine_events();
                                Ok(())
                            } else {
                                Err("The effect could not be removed.".into())
                            }
                        }
                        Err(e) => Err(e),
                    }
                }
            }
            HistoryAction::PatchbayLink {
                external,
                output,
                input,
                connected,
            } => {
                let want_connect = if is_undo { !*connected } else { *connected };
                let prev_ext = self.external_patchbay;
                if *external != prev_ext {
                    self.switch_patchbay_view(*external);
                }
                let mut ids_out = self.patchbay.lookup_endpoint(output);
                let mut ids_in = self.patchbay.lookup_endpoint(input);
                if ids_out.is_none() || ids_in.is_none() {
                    self.refresh_patchbay();
                    ids_out = self.patchbay.lookup_endpoint(output);
                    ids_in = self.patchbay.lookup_endpoint(input);
                }
                let res = match (ids_out, ids_in) {
                    (Some((go, po)), Some((gi, pi))) => {
                        let existing = self.patchbay.find_connection(go, po, gi, pi);
                        if want_connect && existing.is_none() {
                            if self.host.as_ref().is_some_and(|h| h.patchbay_connect(*external, go, po, gi, pi)) {
                                self.drain_engine_events();
                                Ok(())
                            } else {
                                Err("The ports could not be connected.".into())
                            }
                        } else if !want_connect {
                            if let Some(cid) = existing {
                                if self.host.as_ref().is_some_and(|h| h.patchbay_disconnect(*external, cid)) {
                                    self.drain_engine_events();
                                    Ok(())
                                } else {
                                    Err("The ports could not be disconnected.".into())
                                }
                            } else {
                                Ok(())
                            }
                        } else {
                            Ok(())
                        }
                    }
                    _ => Err("One of the patchbay ports is unavailable.".into()),
                };
                if *external != prev_ext {
                    self.switch_patchbay_view(prev_ext);
                }
                res
            }
            HistoryAction::PatchbayArrange {
                external,
                before,
                after,
            } => {
                let prev_ext = self.external_patchbay;
                if *external != prev_ext {
                    self.switch_patchbay_view(*external);
                }
                let target = if is_undo { before } else { after };
                let updates = self.patchbay.apply_positions(target);
                if let Some(ref host) = self.host {
                    for (gid, x1, y1, x2, y2) in updates {
                        let _ = host.patchbay_set_group_pos(*external, gid, x1, y1, x2, y2);
                    }
                }
                self.pending.patchbay_dirty = true;
                if *external != prev_ext {
                    self.switch_patchbay_view(prev_ext);
                }
                Ok(())
            }
        };

        if is_undo {
            if let Some(e) = self.history.take_redo() {
                e.action = entry_action;
                self.history.revert_redo_index();
            }
        } else if let Some(e) = self.history.take_undo() {
            e.action = entry_action;
            self.history.revert_undo_index();
        }

        result.err()
    }

    // --- JSON View-Model Serializers for QML ---

    pub fn rack_json(&self) -> String {
        #[derive(Serialize)]
        struct SlotDto<'a> {
            plugin_id: u32,
            name: &'a str,
            format: &'a str,
            active: bool,
            category: i32,
            category_color: &'a str,
            has_custom_ui: bool,
            custom_ui_visible: bool,
            custom_ui_failed: bool,
            unavailable_error: Option<&'a str>,
            in_channels: usize,
            out_channels: usize,
        }
        let items: Vec<SlotDto<'_>> = self
            .slots
            .iter()
            .map(|s| SlotDto {
                plugin_id: s.plugin_id,
                name: &s.info.name,
                format: plugin_type_label(s.info.ptype),
                active: s.active,
                category: s.category,
                category_color: category_hex(s.category),
                has_custom_ui: (s.info.hints & PLUGIN_HAS_CUSTOM_UI) != 0,
                custom_ui_visible: s.custom_ui_visible,
                custom_ui_failed: s.custom_ui_failed,
                unavailable_error: s.unavailable_error.as_deref(),
                in_channels: s.in_channels,
                out_channels: s.out_channels,
            })
            .collect();
        serde_json::to_string(&items).unwrap_or_else(|_| "[]".into())
    }

    pub fn categories_json() -> String {
        #[derive(Serialize)]
        struct CatDto {
            id: i32,
            key: &'static str,
            label: &'static str,
            color: &'static str,
        }
        let list: Vec<CatDto> = FRANCA_PLUGIN_CATEGORIES
            .iter()
            .map(|c| CatDto {
                id: c.id,
                key: c.key,
                label: c.label,
                color: c.hex_color,
            })
            .collect();
        serde_json::to_string(&list).unwrap_or_else(|_| "[]".into())
    }

    pub fn meters_json(&self) -> String {
        let now = monotonic_secs(self.epoch);
        #[derive(Serialize)]
        struct MeterItem {
            in_l: f32,
            in_r: f32,
            in_clip_l: bool,
            in_clip_r: bool,
            out_l: f32,
            out_r: f32,
            out_clip_l: bool,
            out_clip_r: bool,
            clip_warning: bool,
        }
        #[derive(Serialize)]
        struct MetersPayload {
            slots: Vec<MeterItem>,
            canvas: MeterItem,
            canvas_notes: Vec<u8>,
        }
        let slots: Vec<MeterItem> = self
            .slots
            .iter()
            .map(|s| MeterItem {
                in_l: s.peak_in.smoothed[0],
                in_r: s.peak_in.smoothed[1],
                in_clip_l: s.peak_in.is_channel_clipping(0, now),
                in_clip_r: s.peak_in.is_channel_clipping(1, now),
                out_l: s.peak_out.smoothed[0],
                out_r: s.peak_out.smoothed[1],
                out_clip_l: s.peak_out.is_channel_clipping(0, now),
                out_clip_r: s.peak_out.is_channel_clipping(1, now),
                clip_warning: s.clip_warning.is_active(),
            })
            .collect();
        let canvas = MeterItem {
            in_l: self.canvas_peak_in.smoothed[0],
            in_r: self.canvas_peak_in.smoothed[1],
            in_clip_l: self.canvas_peak_in.is_channel_clipping(0, now),
            in_clip_r: self.canvas_peak_in.is_channel_clipping(1, now),
            out_l: self.canvas_peak_out.smoothed[0],
            out_r: self.canvas_peak_out.smoothed[1],
            out_clip_l: self.canvas_peak_out.is_channel_clipping(0, now),
            out_clip_r: self.canvas_peak_out.is_channel_clipping(1, now),
            clip_warning: false,
        };
        let payload = MetersPayload {
            slots,
            canvas,
            canvas_notes: self.canvas_active_notes.iter().copied().collect(),
        };
        serde_json::to_string(&payload).unwrap_or_else(|_| "{}".into())
    }

    pub fn patchbay_json(&self) -> String {
        serde_json::to_string(&self.patchbay.build_scene_dto()).unwrap_or_else(|_| "{}".into())
    }

    pub fn plugin_edit_json(&self, plugin_id: u32) -> String {
        let Some(ref host) = self.host else {
            return "{}".into();
        };
        let Some(slot) = self.slots.get(plugin_id as usize) else {
            return "{}".into();
        };

        #[derive(Serialize)]
        struct SpDto {
            value: f32,
            label: String,
        }
        #[derive(Serialize)]
        struct ParamDto {
            index: u32,
            name: String,
            unit: String,
            comment: String,
            group_name: String,
            is_output: bool,
            is_read_only: bool,
            is_boolean: bool,
            is_integer: bool,
            is_logarithmic: bool,
            is_automatable: bool,
            value: f32,
            def: f32,
            min: f32,
            max: f32,
            custom_text: String,
            midi_channel: u8,
            mapped_control_index: i16,
            mapped_minimum: f32,
            mapped_maximum: f32,
            scale_points: Vec<SpDto>,
        }
        #[derive(Serialize)]
        struct OptDto {
            bit: u32,
            label: &'static str,
            enabled: bool,
        }
        #[derive(Serialize)]
        struct EditDto {
            plugin_id: u32,
            name: String,
            format: String,
            label: String,
            maker: String,
            copyright: String,
            filename: String,
            unique_id: i64,
            active: bool,
            can_drywet: bool,
            can_volume: bool,
            can_balance: bool,
            can_panning: bool,
            drywet: f32,
            volume: f32,
            balance_left: f32,
            balance_right: f32,
            panning: f32,
            ctrl_channel: i8,
            audio_ins: u32,
            audio_outs: u32,
            midi_ins: u32,
            midi_outs: u32,
            param_ins: u32,
            param_outs: u32,
            latency: u32,
            options: Vec<OptDto>,
            programs: Vec<String>,
            current_program: i32,
            midi_programs: Vec<String>,
            current_midi_program: i32,
            params: Vec<ParamDto>,
            active_notes: Vec<u8>,
        }

        let count = host.get_parameter_count(plugin_id).min(512);
        let mut params = Vec::new();
        for i in 0..count {
            let data = host.get_parameter_data(plugin_id, i);
            if (data.hints & PARAMETER_IS_ENABLED) == 0 {
                continue;
            }
            let info = host.get_parameter_info(plugin_id, i);
            let ranges = host.get_parameter_ranges(plugin_id, i);
            let value = host.get_current_parameter_value(plugin_id, i);
            let mut scale_points = Vec::with_capacity(info.scale_point_count as usize);
            for sp in 0..info.scale_point_count {
                let s = host.get_parameter_scalepoint_info(plugin_id, i, sp);
                scale_points.push(SpDto {
                    value: s.value,
                    label: s.label,
                });
            }
            let custom_text = if (data.hints & PARAMETER_USES_CUSTOM_TEXT) != 0 {
                host.get_parameter_text(plugin_id, i)
            } else {
                String::new()
            };
            params.push(ParamDto {
                index: i,
                name: info.name,
                unit: info.unit,
                comment: info.comment,
                group_name: if info.group_name.is_empty() {
                    "Parameters".into()
                } else {
                    info.group_name
                },
                is_output: data.ptype == PARAMETER_OUTPUT,
                is_read_only: (data.hints & PARAMETER_IS_READ_ONLY) != 0
                    || data.ptype == PARAMETER_OUTPUT,
                is_boolean: (data.hints & PARAMETER_IS_BOOLEAN) != 0,
                is_integer: (data.hints & PARAMETER_IS_INTEGER) != 0,
                is_logarithmic: (data.hints & PARAMETER_IS_LOGARITHMIC) != 0,
                is_automatable: (data.hints & PARAMETER_IS_AUTOMATABLE) != 0,
                value,
                def: ranges.def,
                min: ranges.min,
                max: if ranges.max > ranges.min {
                    ranges.max
                } else {
                    ranges.min + 1.0
                },
                custom_text,
                midi_channel: data.midi_channel,
                mapped_control_index: data.mapped_control_index,
                mapped_minimum: data.mapped_minimum,
                mapped_maximum: data.mapped_maximum,
                scale_points,
            });
        }

        let option_defs: &[(u32, &str)] = &[
            (PLUGIN_OPTION_FIXED_BUFFERS, "Use fixed buffers"),
            (PLUGIN_OPTION_FORCE_STEREO, "Force stereo"),
            (PLUGIN_OPTION_MAP_PROGRAM_CHANGES, "Map program changes"),
            (PLUGIN_OPTION_USE_CHUNKS, "Use chunks"),
            (PLUGIN_OPTION_SEND_CONTROL_CHANGES, "Send control changes"),
            (PLUGIN_OPTION_SEND_CHANNEL_PRESSURE, "Send channel pressure"),
            (PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH, "Send note aftertouch"),
            (PLUGIN_OPTION_SEND_PITCHBEND, "Send pitchbend"),
            (PLUGIN_OPTION_SEND_ALL_SOUND_OFF, "Send all sound off"),
            (PLUGIN_OPTION_SEND_PROGRAM_CHANGES, "Send program changes"),
            (PLUGIN_OPTION_SKIP_SENDING_NOTES, "Skip sending notes"),
        ];
        let options: Vec<OptDto> = option_defs
            .iter()
            .filter(|(bit, _)| (slot.info.options_available & *bit) != 0)
            .map(|&(bit, label)| OptDto {
                bit,
                label,
                enabled: (slot.info.options_enabled & bit) != 0,
            })
            .collect();

        let pcount = host.get_program_count(plugin_id);
        let programs: Vec<String> = (0..pcount)
            .map(|i| format!("{i:03} - {}", host.get_program_name(plugin_id, i)))
            .collect();
        let mcount = host.get_midi_program_count(plugin_id);
        let midi_programs: Vec<String> = (0..mcount)
            .map(|i| {
                let m = host.get_midi_program_data(plugin_id, i);
                format!("{:03}:{:03} - {}", m.bank, m.program, m.name)
            })
            .collect();

        let audio = host.get_audio_port_count_info(plugin_id);
        let midi = host.get_midi_port_count_info(plugin_id);
        let pc = host.get_parameter_count_info(plugin_id);

        let dto = EditDto {
            plugin_id,
            name: slot.info.name.clone(),
            format: plugin_type_label(slot.info.ptype).to_string(),
            label: slot.info.label.clone(),
            maker: slot.info.maker.clone(),
            copyright: slot.info.copyright.clone(),
            filename: slot.info.filename.clone(),
            unique_id: slot.info.unique_id,
            active: slot.active,
            can_drywet: (slot.info.hints & PLUGIN_CAN_DRYWET) != 0,
            can_volume: (slot.info.hints & PLUGIN_CAN_VOLUME) != 0,
            can_balance: (slot.info.hints & PLUGIN_CAN_BALANCE) != 0,
            can_panning: (slot.info.hints & PLUGIN_CAN_PANNING) != 0,
            drywet: host.get_internal_parameter_value(plugin_id, PARAMETER_DRYWET),
            volume: host.get_internal_parameter_value(plugin_id, PARAMETER_VOLUME),
            balance_left: host.get_internal_parameter_value(plugin_id, PARAMETER_BALANCE_LEFT),
            balance_right: host.get_internal_parameter_value(plugin_id, PARAMETER_BALANCE_RIGHT),
            panning: host.get_internal_parameter_value(plugin_id, PARAMETER_PANNING),
            ctrl_channel: host.get_internal_parameter_value(plugin_id, PARAMETER_CTRL_CHANNEL) as i8,
            audio_ins: audio.ins,
            audio_outs: audio.outs,
            midi_ins: midi.ins,
            midi_outs: midi.outs,
            param_ins: pc.ins,
            param_outs: pc.outs,
            latency: host.get_plugin_latency(plugin_id),
            options,
            programs,
            current_program: host.get_current_program_index(plugin_id),
            midi_programs,
            current_midi_program: host.get_current_midi_program_index(plugin_id),
            params,
            active_notes: slot.active_notes.iter().copied().collect(),
        };
        serde_json::to_string(&dto).unwrap_or_else(|_| "{}".into())
    }

    pub fn query_plugins_json(
        &self,
        search: &str,
        favorites_only: bool,
        category_filter: i32, // -1 = all
        formats_mask: u32,
        types_mask: u32,
        sort_col: i32,
        sort_asc: bool,
    ) -> String {
        let q = search.trim().to_ascii_lowercase();
        let mut filtered: Vec<&DiscoveredPlugin> = self
            .db
            .plugins
            .iter()
            .filter(|p| {
                let key = p.unique_key();
                if favorites_only && !self.settings.favorite_plugins.contains(&key) {
                    return false;
                }
                let fbit = match p.ptype {
                    PLUGIN_INTERNAL => 1 << 0,
                    PLUGIN_LADSPA => 1 << 1,
                    PLUGIN_DSSI => 1 << 2,
                    PLUGIN_LV2 => 1 << 3,
                    PLUGIN_VST2 => 1 << 4,
                    PLUGIN_VST3 => 1 << 5,
                    PLUGIN_CLAP => 1 << 6,
                    PLUGIN_SF2 => 1 << 7,
                    PLUGIN_SFZ => 1 << 8,
                    PLUGIN_JSFX => 1 << 9,
                    _ => 0xFFFF,
                };
                if (formats_mask & fbit) == 0 {
                    return false;
                }
                if category_filter >= 0 && p.category != category_filter {
                    return false;
                }
                let tbit = if p.is_synth() {
                    1 << 1
                } else if p.is_midi_only() {
                    1 << 2
                } else if p.is_effect() {
                    1 << 0
                } else {
                    1 << 3
                };
                if (types_mask & tbit) == 0 {
                    return false;
                }
                if !q.is_empty() {
                    let hay = format!(
                        "{} {} {} {}",
                        p.name.to_ascii_lowercase(),
                        p.maker.to_ascii_lowercase(),
                        p.label.to_ascii_lowercase(),
                        plugin_type_label(p.ptype).to_ascii_lowercase()
                    );
                    for term in q.split_whitespace() {
                        if !hay.contains(term) {
                            return false;
                        }
                    }
                }
                true
            })
            .collect();

        filtered.sort_by(|a, b| {
            let ord = match sort_col {
                1 => plugin_type_label(a.ptype).cmp(plugin_type_label(b.ptype)),
                2 => category_label(a.category).cmp(category_label(b.category)),
                3 => a.maker.to_lowercase().cmp(&b.maker.to_lowercase()),
                _ => a.name.to_lowercase().cmp(&b.name.to_lowercase()),
            };
            if sort_asc { ord } else { ord.reverse() }
        });

        #[derive(Serialize)]
        struct ItemDto {
            key: String,
            favorite: bool,
            name: String,
            format: String,
            build: String,
            category: String,
            maker: String,
            label: String,
            filename: String,
            audio_ins: u32,
            audio_outs: u32,
            midi_ins: u32,
            midi_outs: u32,
            param_ins: u32,
            param_outs: u32,
        }
        #[derive(Serialize)]
        struct RespDto {
            total_count: usize,
            scanning: bool,
            scan_progress: f32,
            scan_status: String,
            scan_found: usize,
            items: Vec<ItemDto>,
        }
        let items: Vec<ItemDto> = filtered
            .into_iter()
            .map(|p| {
                let key = p.unique_key();
                ItemDto {
                    favorite: self.settings.favorite_plugins.contains(&key),
                    key,
                    name: p.name.clone(),
                    format: plugin_type_label(p.ptype).to_string(),
                    build: binary_type_label(p.build).to_string(),
                    category: category_label(p.category).to_string(),
                    maker: p.maker.clone(),
                    label: p.label.clone(),
                    filename: p.filename.clone(),
                    audio_ins: p.audio_ins,
                    audio_outs: p.audio_outs,
                    midi_ins: p.midi_ins,
                    midi_outs: p.midi_outs,
                    param_ins: p.parameter_ins,
                    param_outs: p.parameter_outs,
                }
            })
            .collect();

        let (scanning, scan_progress, scan_status, scan_found) =
            if let Some(ref r) = self.discovery_runner {
                (true, r.progress(), r.status_text(), r.discovered_count)
            } else {
                (false, 0.0, String::new(), 0)
            };

        let resp = RespDto {
            total_count: self.db.plugins.len(),
            scanning,
            scan_progress,
            scan_status,
            scan_found,
            items,
        };
        serde_json::to_string(&resp).unwrap_or_else(|_| "{}".into())
    }

    pub fn start_discovery(&mut self, formats_mask: u32, check_invalid: bool) {
        let Some(ref u) = self.utils else {
            self.pending.alerts.push((
                "Discovery Unavailable".into(),
                "libcarla_utils could not be loaded.".into(),
            ));
            return;
        };
        let opts = DiscoveryOptions {
            check_internal: (formats_mask & (1 << 0)) != 0,
            check_ladspa: (formats_mask & (1 << 1)) != 0,
            check_dssi: (formats_mask & (1 << 2)) != 0,
            check_lv2: (formats_mask & (1 << 3)) != 0,
            check_vst2: (formats_mask & (1 << 4)) != 0,
            check_vst3: (formats_mask & (1 << 5)) != 0,
            check_clap: (formats_mask & (1 << 6)) != 0,
            check_sf2: (formats_mask & (1 << 7)) != 0,
            check_sfz: (formats_mask & (1 << 8)) != 0,
            check_jsfx: (formats_mask & (1 << 9)) != 0,
            check_invalid,
        };
        let runner = DiscoveryRunner::start(
            u,
            &self.bin_dir,
            &self.settings,
            &self.db.file_cache,
            &opts,
        );
        self.discovery_runner = Some(runner);
        self.pending.discovery_dirty = true;
        logging::log_ui("Started plugin discovery scan...");
    }

    pub fn files_json(&self) -> String {
        #[derive(Serialize)]
        struct EntryDto {
            name: String,
            path: String,
            is_dir: bool,
        }
        #[derive(Serialize)]
        struct FilesDto {
            folders: Vec<String>,
            selected_folder_idx: usize,
            current_dir: String,
            can_go_up: bool,
            entries: Vec<EntryDto>,
        }
        let mut folders = vec!["Home".to_string()];
        for f in &self.settings.disk_folders {
            let label = Path::new(f)
                .file_name()
                .map(|n| n.to_string_lossy().into_owned())
                .unwrap_or_else(|| f.clone());
            folders.push(label);
        }

        let mut dirs = Vec::new();
        let mut files = Vec::new();
        if let Ok(read) = fs::read_dir(&self.current_dir) {
            for entry in read.flatten() {
                let p = entry.path();
                let Some(name) = p.file_name().and_then(|n| n.to_str()).map(str::to_string) else {
                    continue;
                };
                if name.starts_with('.') {
                    continue;
                }
                if p.is_dir() {
                    dirs.push(EntryDto {
                        name,
                        path: p.to_string_lossy().into_owned(),
                        is_dir: true,
                    });
                } else if is_supported_file(&p, &self.supported_exts) {
                    files.push(EntryDto {
                        name,
                        path: p.to_string_lossy().into_owned(),
                        is_dir: false,
                    });
                }
            }
        }
        dirs.sort_by(|a, b| a.name.to_lowercase().cmp(&b.name.to_lowercase()));
        files.sort_by(|a, b| a.name.to_lowercase().cmp(&b.name.to_lowercase()));
        dirs.extend(files);

        let dto = FilesDto {
            folders,
            selected_folder_idx: self.selected_folder_idx,
            current_dir: self.current_dir.to_string_lossy().into_owned(),
            can_go_up: self.current_dir.parent().is_some(),
            entries: dirs,
        };
        serde_json::to_string(&dto).unwrap_or_else(|_| "{}".into())
    }

    pub fn settings_draft_json(&self) -> String {
        #[derive(Serialize)]
        struct SettingsPayload {
            settings: FrancaSettings,
            drivers: Vec<String>,
            osc_tcp_url: String,
            osc_udp_url: String,
        }
        let mut drivers = Vec::new();
        let (osc_tcp_url, osc_udp_url) = if let Some(ref host) = self.host {
            let count = host.get_engine_driver_count();
            for i in 0..count {
                drivers.push(host.get_engine_driver_name(i));
            }
            if host.is_engine_running() {
                (host.get_host_osc_url_tcp(), host.get_host_osc_url_udp())
            } else {
                (String::new(), String::new())
            }
        } else {
            (String::new(), String::new())
        };
        let payload = SettingsPayload {
            settings: self.settings.clone(),
            drivers,
            osc_tcp_url,
            osc_udp_url,
        };
        serde_json::to_string(&payload).unwrap_or_else(|_| "{}".into())
    }

    pub fn query_driver_devices_json(&self, driver_name: &str, device_name: &str) -> String {
        #[derive(Serialize)]
        struct DevPayload {
            devices: Vec<String>,
            buffer_sizes: Vec<u32>,
            sample_rates: Vec<u32>,
            can_triple_buffer: bool,
            has_control_panel: bool,
        }
        let Some(ref host) = self.host else {
            return "{}".into();
        };
        let count = host.get_engine_driver_count();
        let mut drv_idx = None;
        for i in 0..count {
            if host.get_engine_driver_name(i) == driver_name {
                drv_idx = Some(i);
                break;
            }
        }
        let Some(idx) = drv_idx else {
            return "{}".into();
        };
        let devices = host.get_engine_driver_device_names(idx);
        let qdev = if device_name.is_empty() {
            devices.first().cloned().unwrap_or_default()
        } else {
            device_name.to_string()
        };
        let info = host.get_engine_driver_device_info(idx, &qdev);
        let buffer_sizes = if info.buffer_sizes.is_empty() {
            vec![64, 128, 256, 512, 1024, 2048]
        } else {
            info.buffer_sizes
        };
        let sample_rates = if info.sample_rates.is_empty() {
            vec![44100, 48000, 88200, 96000]
        } else {
            info.sample_rates.into_iter().map(|r| r as u32).collect()
        };
        let payload = DevPayload {
            devices,
            buffer_sizes,
            sample_rates,
            can_triple_buffer: (info.hints & ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER) != 0,
            has_control_panel: (info.hints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL) != 0,
        };
        serde_json::to_string(&payload).unwrap_or_else(|_| "{}".into())
    }

    pub fn default_paths_for_category(cat: &str) -> Vec<String> {
        match cat {
            "ladspa" => default_ladspa_paths(),
            "dssi" => default_dssi_paths(),
            "lv2" => default_lv2_paths(),
            "vst2" => default_vst2_paths(),
            "vst3" => default_vst3_paths(),
            "clap" => default_clap_paths(),
            "sf2" => default_sf2_paths(),
            "sfz" => default_sfz_paths(),
            "jsfx" => default_jsfx_paths(),
            _ => Vec::new(),
        }
    }

    pub fn effective_buffer_sizes(&self) -> Vec<u32> {
        let mut sizes = self.runtime_dev_info.buffer_sizes.clone();
        if sizes.is_empty()
            && (self.runtime_dev_info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE) != 0
        {
            sizes.extend_from_slice(&[16, 32, 64, 128, 256, 512, 1024, 2048, 4096]);
        }
        if self.buffer_size > 0 && !sizes.contains(&self.buffer_size) {
            sizes.push(self.buffer_size);
        }
        sizes.sort_unstable();
        sizes.dedup();
        sizes
    }

    pub fn effective_sample_rates(&self) -> Vec<u32> {
        let mut rates: Vec<u32> = self
            .runtime_dev_info
            .sample_rates
            .iter()
            .map(|&r| r as u32)
            .collect();
        if rates.is_empty()
            && (self.runtime_dev_info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE) != 0
        {
            rates.extend_from_slice(&[22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000]);
        }
        let cur = self.sample_rate as u32;
        if cur > 0 && !rates.contains(&cur) {
            rates.push(cur);
        }
        rates.sort_unstable();
        rates.dedup();
        rates
    }

    pub fn runtime_driver_json(&self) -> String {
        #[derive(Serialize)]
        struct RtDto {
            name: String,
            buffer_size: u32,
            buffer_sizes: Vec<u32>,
            sample_rate: u32,
            sample_rates: Vec<u32>,
            variable_buffer: bool,
            variable_rate: bool,
            has_control_panel: bool,
        }
        let info = &self.runtime_dev_info;
        let dto = RtDto {
            name: info.name.clone(),
            buffer_size: self.buffer_size,
            buffer_sizes: self.effective_buffer_sizes(),
            sample_rate: self.sample_rate as u32,
            sample_rates: self.effective_sample_rates(),
            variable_buffer: (info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE) != 0,
            variable_rate: (info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE) != 0,
            has_control_panel: (info.hints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL) != 0,
        };
        serde_json::to_string(&dto).unwrap_or_else(|_| "{}".into())
    }

    pub fn about_json(&self) -> String {
        #[derive(Serialize)]
        struct AboutDto {
            version: &'static str,
            features: Vec<String>,
            license: String,
        }
        let (features, license) = if let Some(ref u) = self.utils {
            (u.get_supported_features(), u.get_complete_license_text())
        } else {
            (Vec::new(), String::new())
        };
        let dto = AboutDto {
            version: CARLA_VERSION_STRING,
            features,
            license,
        };
        serde_json::to_string(&dto).unwrap_or_else(|_| "{}".into())
    }

    pub fn apply_selected_boxes_csv(&mut self, csv: &str) {
        self.patchbay.selected_boxes.clear();
        for tok in csv.split(',').filter(|s| !s.is_empty()) {
            if let Some((g, p)) = tok.split_once(':') {
                if let (Ok(gid), Ok(sp)) = (g.parse::<u32>(), p.parse::<u8>()) {
                    self.patchbay.selected_boxes.insert(NodeKey {
                        group_id: gid,
                        split_part: sp,
                    });
                }
            }
        }
        self.selected_patchbay_plugins = self.patchbay.selected_plugin_ids();
        if self.selected_patchbay_plugins.is_empty() {
            self.canvas_peak_in.clear();
            self.canvas_peak_out.clear();
            self.canvas_active_notes.clear();
        }
        self.pending.patchbay_dirty = true;
    }

    pub fn build_param_groups_map(&self) -> BTreeMap<String, Vec<u32>> {
        BTreeMap::new()
    }
}
