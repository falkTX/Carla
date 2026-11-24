// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::{Duration, Instant};

use eframe::egui::{self, Color32, Vec2};

use crate::discovery::{DiscoveryOptions, DiscoveryRunner, PluginDatabase};
use crate::ffi::host::{CarlaHost, EngineEvent, RuntimeDriverDeviceInfo};
use crate::ffi::types::*;
use crate::ffi::utils::{CarlaUtils, DiscoveredPlugin};
use crate::history::{EditHistory, HistoryAction, MergeKey};
use crate::logging;
use crate::patchbay::widget::show_patchbay_canvas;
use crate::patchbay::{PatchbayAction, PatchbayState};
use crate::project::{PluginSnapshot, is_project_file, is_supported_file};
use crate::settings::FrancaSettings;
use crate::ui::add_plugin::{AddPluginAction, AddPluginDialogState, show_add_plugin_dialog};
use crate::ui::dialogs::{
    AboutDialogState, AddJackAppDialogState, AlertState, ConfirmKind, RenameDialogState,
    show_about_dialog, show_add_jack_app_dialog, show_alert_modal, show_cancelable_action_modal,
    show_confirm_modal, show_rename_dialog,
};
use crate::ui::logs_tab::{LogsTabState, show_logs_tab};
use crate::ui::plugin_edit::{PluginEditAction, show_plugin_edit_window};
use crate::ui::rack::{RackAction, RackDragState, RackPluginSlot, show_rack_view};
use crate::ui::settings_ui::{
    RuntimeDriverDialogState, SettingsDialogState, show_runtime_driver_dialog, show_settings_dialog,
};
use crate::ui::side_panel::{SidePanelAction, SidePanelState, show_side_panel};
use crate::widgets::keyboard::{NoteEvent, PianoKeyboardState};
use crate::widgets::meters::{MeterColor, PeakMeterState, monotonic_secs};

pub static SIG_QUIT_REQUESTED: AtomicBool = AtomicBool::new(false);
pub static SIG_SAVE_REQUESTED: AtomicBool = AtomicBool::new(false);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MainTab {
    Rack,
    Patchbay,
    Logs,
}

pub struct FrancaApp {
    pub host: CarlaHost,
    pub utils: Option<CarlaUtils>,
    pub bin_dir: PathBuf,
    pub res_dir: PathBuf,
    pub settings: FrancaSettings,
    pub db: PluginDatabase,
    pub supported_exts: Vec<String>,

    // Runtime state
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
    pub project_filename: Option<PathBuf>,
    pub is_project_loading: bool,
    pub removing_all_plugins: bool,
    pub external_patchbay: bool,
    pub nsm_active: bool,
    pub quit_confirmed: bool,
    pub last_window_title: String,
    pub cancelable_action_msg: Option<String>,
    pub pending_open_project: Option<PathBuf>,

    // Tabs & Rack
    pub active_tab: MainTab,
    pub slots: Vec<RackPluginSlot>,
    pub selected_rack_plugin: Option<u32>,
    pub selected_patchbay_plugins: Vec<u32>,
    pub rack_drag: RackDragState,
    pub history: EditHistory,
    pub history_plugin_count: usize,

    // Patchbay & Side Widgets
    pub patchbay: PatchbayState,
    pub canvas_peak_in: PeakMeterState,
    pub canvas_peak_out: PeakMeterState,
    pub canvas_keyboard: PianoKeyboardState,
    pub side_panel: SidePanelState,
    pub logs_tab: LogsTabState,

    // Dialogs
    pub add_plugin_dlg: AddPluginDialogState,
    pub settings_dlg: SettingsDialogState,
    pub runtime_driver_dlg: RuntimeDriverDialogState,
    pub rename_dlg: RenameDialogState,
    pub jack_app_dlg: AddJackAppDialogState,
    pub about_dlg: AboutDialogState,
    pub alert: AlertState,
    pub confirm: Option<ConfirmKind>,
}

impl FrancaApp {
    pub fn new(
        host: CarlaHost,
        utils: Option<CarlaUtils>,
        bin_dir: PathBuf,
        res_dir: PathBuf,
        initial_project: Option<PathBuf>,
    ) -> Self {
        let settings = FrancaSettings::load();
        let mut db = PluginDatabase::load();
        let supported_exts = utils
            .as_ref()
            .map(|u| u.get_supported_file_extensions())
            .unwrap_or_default();
        if let Some(ref u) = utils {
            db.ensure_cached_builtins(u, &settings);
        }

        let mut patchbay = PatchbayState::new(
            settings.canvas_width as f32,
            settings.canvas_height as f32,
        );
        patchbay.use_bezier = settings.canvas_use_bezier;
        patchbay.auto_hide_groups = settings.canvas_auto_hide_groups;

        let nsm_active = if std::env::var("NSM_URL").is_ok() {
            host.nsm_init(std::process::id() as u64, "franca")
        } else {
            false
        };

        let mut app = Self {
            host,
            utils,
            bin_dir,
            res_dir,
            process_mode: settings.process_mode,
            transport_mode: settings.transport_mode,
            settings,
            db,
            supported_exts,
            epoch: Instant::now(),
            last_slow_poll: Instant::now(),
            engine_running: false,
            running_driver_name: String::new(),
            buffer_size: 512,
            sample_rate: 44100.0,
            runtime_info: CarlaRuntimeEngineInfo::default(),
            runtime_dev_info: RuntimeDriverDeviceInfo::default(),
            transport_info: CarlaTransportInfo::default(),
            project_filename: None,
            is_project_loading: false,
            removing_all_plugins: false,
            external_patchbay: false,
            nsm_active,
            quit_confirmed: false,
            last_window_title: "Franca".into(),
            cancelable_action_msg: None,
            pending_open_project: initial_project,
            active_tab: MainTab::Rack,
            slots: Vec::new(),
            selected_rack_plugin: None,
            selected_patchbay_plugins: Vec::new(),
            rack_drag: RackDragState::default(),
            history: EditHistory::new(),
            history_plugin_count: 0,
            patchbay,
            canvas_peak_in: PeakMeterState::new(2, MeterColor::Green),
            canvas_peak_out: PeakMeterState::new(2, MeterColor::Blue),
            canvas_keyboard: PianoKeyboardState::default(),
            side_panel: SidePanelState::default(),
            logs_tab: LogsTabState::default(),
            add_plugin_dlg: AddPluginDialogState::default(),
            settings_dlg: SettingsDialogState::default(),
            runtime_driver_dlg: RuntimeDriverDialogState::default(),
            rename_dlg: RenameDialogState::default(),
            jack_app_dlg: AddJackAppDialogState::default(),
            about_dlg: AboutDialogState::default(),
            alert: AlertState::default(),
            confirm: None,
        };

        if !nsm_active {
            app.start_engine(true);
            if let Some(proj) = app.pending_open_project.take() {
                app.load_project_or_file(&proj);
            }
        }

        app
    }

    pub fn apply_theme(&self, ctx: &egui::Context) {
        if self.settings.dark_theme {
            ctx.set_visuals(egui::Visuals::dark());
        } else {
            ctx.set_visuals(egui::Visuals::light());
        }
    }

    pub fn start_engine(&mut self, silent_on_first_fail: bool) {
        if self.host.is_engine_running() {
            return;
        }
        self.settings
            .apply_to_host(&self.host, &self.bin_dir, &self.res_dir);
        let driver = self.settings.audio_driver.clone();
        let client_name = std::env::var("CARLA_CLIENT_NAME").unwrap_or_else(|_| "Franca".into());
        logging::log_ui(&format!("Starting audio engine ({driver})..."));
        if !self.host.engine_init(&driver, &client_name) {
            let err = self.host.get_last_error();
            logging::log_error(&format!("Failed to start engine ({driver}): {err}"));
            if !silent_on_first_fail {
                self.alert
                    .show("Engine Error", format!("Failed to start audio engine:\n{err}"));
            }
            return;
        }
        self.drain_engine_events();
        self.refresh_runtime_state();
        let _ = self.host.patchbay_refresh(self.external_patchbay);
        self.drain_engine_events();
    }

    pub fn stop_engine(&mut self) {
        if !self.host.is_engine_running() {
            return;
        }
        logging::log_ui("Stopping audio engine...");
        let _ = self.host.set_engine_about_to_close();
        let _ = self.host.engine_close();
        self.drain_engine_events();
        self.engine_running = false;
        self.slots.clear();
        self.selected_rack_plugin = None;
        self.selected_patchbay_plugins.clear();
        self.patchbay.clear();
        self.history.clear();
        self.history_plugin_count = 0;
    }

    pub fn refresh_runtime_state(&mut self) {
        self.engine_running = self.host.is_engine_running();
        if self.engine_running {
            self.buffer_size = self.host.get_buffer_size();
            self.sample_rate = self.host.get_sample_rate();
            self.runtime_info = self.host.get_runtime_engine_info();
            self.runtime_dev_info = self.host.get_runtime_engine_driver_device_info();
            self.transport_info = self.host.get_transport_info();
            if self.running_driver_name.is_empty() {
                self.running_driver_name = self.settings.audio_driver.clone();
            }
        }
    }

    pub fn drain_engine_events(&mut self) {
        let events = self.host.drain_events();
        for ev in events {
            self.handle_engine_event(ev);
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
                self.alert.show("Information", ev.value_str);
            }
            ENGINE_CALLBACK_ERROR => {
                logging::log_error(&ev.value_str);
                self.alert.show("Engine Error", ev.value_str);
            }
            ENGINE_CALLBACK_ENGINE_STARTED => {
                self.engine_running = true;
                self.process_mode = ev.value1;
                self.transport_mode = ev.value2;
                self.buffer_size = ev.value3.max(0) as u32;
                self.sample_rate = ev.valuef as f64;
                if !ev.value_str.is_empty() {
                    self.running_driver_name = ev.value_str;
                } else {
                    self.running_driver_name = self.settings.audio_driver.clone();
                }
                self.runtime_dev_info = self.host.get_runtime_engine_driver_device_info();
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
                logging::log_ui("Engine stopped");
            }
            ENGINE_CALLBACK_PROCESS_MODE_CHANGED => {
                self.process_mode = ev.value1;
            }
            ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED => {
                self.transport_mode = ev.value1;
            }
            ENGINE_CALLBACK_BUFFER_SIZE_CHANGED => {
                self.buffer_size = ev.value1.max(0) as u32;
                self.runtime_dev_info = self.host.get_runtime_engine_driver_device_info();
            }
            ENGINE_CALLBACK_SAMPLE_RATE_CHANGED => {
                self.sample_rate = ev.valuef as f64;
                self.runtime_dev_info = self.host.get_runtime_engine_driver_device_info();
            }
            ENGINE_CALLBACK_CANCELABLE_ACTION => {
                let starting = ev.value1 != 0;
                self.history.set_blocked(starting);
                if starting {
                    self.cancelable_action_msg = Some(if ev.value_str.is_empty() {
                        "Processing...".into()
                    } else {
                        ev.value_str
                    });
                } else {
                    self.cancelable_action_msg = None;
                }
            }
            ENGINE_CALLBACK_PROJECT_LOAD_FINISHED => {
                self.is_project_loading = false;
                self.history.clear();
                self.history.set_blocked(false);
                self.history_plugin_count = self.slots.len();
                let _ = self.host.patchbay_refresh(self.external_patchbay);
                logging::log_ui("Project load finished");
            }
            ENGINE_CALLBACK_PLUGIN_ADDED => {
                let pid = ev.plugin_id;
                let before = self.history_plugin_count;
                let slot = RackPluginSlot::from_host(&self.host, pid);
                let pname = slot.info.name.clone();
                if (pid as usize) < self.slots.len() {
                    self.slots[pid as usize] = slot;
                } else {
                    self.slots.push(slot);
                }
                self.history_plugin_count = self.slots.len();
                logging::log_ui(&format!("Plugin added: {pname} (id={pid})"));

                if !self.history.busy
                    && !self.history.blocked
                    && !self.is_project_loading
                {
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
                if self.selected_rack_plugin == Some(ev.plugin_id) {
                    self.selected_rack_plugin = None;
                } else if let Some(sel) = self.selected_rack_plugin {
                    if sel > ev.plugin_id {
                        self.selected_rack_plugin = Some(sel - 1);
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
            }
            ENGINE_CALLBACK_PLUGIN_RENAMED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.info.name = ev.value_str;
                }
            }
            ENGINE_CALLBACK_PLUGIN_UNAVAILABLE => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    logging::log_error(&format!("{}: {}", slot.info.name, ev.value_str));
                    slot.unavailable_error = Some(ev.value_str);
                }
            }
            ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    if ev.value1 == PARAMETER_ACTIVE {
                        slot.active = ev.valuef >= 0.5;
                        if !slot.active {
                            slot.clip_warning.clear();
                        }
                    } else {
                        slot.edit_state.set_parameter_value_cached(
                            ev.value1,
                            ev.valuef,
                            &self.host,
                            ev.plugin_id,
                        );
                    }
                }
            }
            ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    if let Some(p) = slot.edit_state.params.get_mut(ev.value1 as usize) {
                        p.ranges.def = ev.valuef;
                    }
                }
            }
            ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    if let Some(p) = slot.edit_state.params.get_mut(ev.value1 as usize) {
                        p.data.mapped_control_index = ev.value2 as i16;
                    }
                }
            }
            ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    if let Some(p) = slot.edit_state.params.get_mut(ev.value1 as usize) {
                        p.data.midi_channel = ev.value2 as u8;
                    }
                }
            }
            ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    if let Some(p) = slot.edit_state.params.get_mut(ev.value1 as usize) {
                        p.data = self.host.get_parameter_data(ev.plugin_id, ev.value1 as u32);
                    }
                }
            }
            ENGINE_CALLBACK_OPTION_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    let bit = ev.value1 as u32;
                    if ev.value2 != 0 {
                        slot.info.options_enabled |= bit;
                    } else {
                        slot.info.options_enabled &= !bit;
                    }
                }
            }
            ENGINE_CALLBACK_PROGRAM_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.edit_state.current_program = ev.value1;
                    slot.edit_state.reload_parameters(&self.host, ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.edit_state.current_midi_program = ev.value1;
                    slot.edit_state.reload_parameters(&self.host, ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_UI_STATE_CHANGED => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.custom_ui_visible = ev.value1 == 1;
                    slot.custom_ui_failed = ev.value1 == -1;
                    if ev.value1 == -1 {
                        logging::log_ui(&format!("Editor unavailable for {}", slot.info.name));
                    }
                }
            }
            ENGINE_CALLBACK_NOTE_ON => {
                let note = ev.value2.clamp(0, 127) as u8;
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.edit_state.keyboard.note_on_external(note);
                }
                if self.selected_patchbay_plugins.contains(&ev.plugin_id) {
                    self.canvas_keyboard.note_on_external(note);
                }
            }
            ENGINE_CALLBACK_NOTE_OFF => {
                let note = ev.value2.clamp(0, 127) as u8;
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.edit_state.keyboard.note_off_external(note);
                }
                if self.selected_patchbay_plugins.contains(&ev.plugin_id) {
                    self.canvas_keyboard.note_off_external(note);
                }
            }
            ENGINE_CALLBACK_RELOAD_INFO | ENGINE_CALLBACK_UPDATE => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.reload_info(&self.host);
                }
            }
            ENGINE_CALLBACK_RELOAD_PARAMETERS => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.edit_state.reload_parameters(&self.host, ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_RELOAD_PROGRAMS => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.edit_state.reload_programs(&self.host, ev.plugin_id);
                }
            }
            ENGINE_CALLBACK_RELOAD_ALL => {
                if let Some(slot) = self.slots.get_mut(ev.plugin_id as usize) {
                    slot.reload_info(&self.host);
                    slot.edit_state.reload_all(&self.host, ev.plugin_id);
                }
            }
            // Patchbay callbacks
            ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED => {
                self.patchbay
                    .add_group(ev.plugin_id, ev.value2, ev.value1, ev.value_str);
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED => {
                self.patchbay.remove_group(ev.plugin_id);
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED => {
                self.patchbay.rename_group(ev.plugin_id, ev.value_str);
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED => {
                self.patchbay
                    .set_group_data(ev.plugin_id, ev.value2, ev.value1);
            }
            ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED => {
                self.patchbay.set_group_position(
                    ev.plugin_id,
                    ev.value1,
                    ev.value2,
                    ev.value3,
                    ev.valuef as i32,
                );
            }
            ENGINE_CALLBACK_PATCHBAY_PORT_ADDED => {
                self.patchbay.add_port(
                    ev.plugin_id,
                    ev.value1 as u32,
                    ev.value2 as u32,
                    ev.value_str,
                );
            }
            ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED => {
                self.patchbay.remove_port(ev.plugin_id, ev.value1 as u32);
            }
            ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED => {
                self.patchbay.update_port(
                    ev.plugin_id,
                    ev.value1 as u32,
                    ev.value2 as u32,
                    ev.value_str,
                );
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
                }
            }
            ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED => {
                self.patchbay.remove_connection(ev.plugin_id);
            }
            ENGINE_CALLBACK_NSM => {
                self.handle_nsm_event(ev.value1, ev.value_str);
            }
            ENGINE_CALLBACK_QUIT => {
                self.quit_confirmed = true;
            }
            _ => {}
        }
    }

    fn handle_nsm_event(&mut self, opcode: i32, value_str: String) {
        match opcode {
            NSM_CALLBACK_INIT => {
                self.host.nsm_ready(NSM_CALLBACK_INIT);
            }
            NSM_CALLBACK_OPEN => {
                let parts: Vec<&str> = value_str.split(':').collect();
                if let Some(path) = parts.first() {
                    let proj_dir = PathBuf::from(path);
                    let _ = std::fs::create_dir_all(&proj_dir);
                    let proj_file = proj_dir.join("project.carxp");
                    if !self.engine_running {
                        self.start_engine(true);
                    }
                    if proj_file.exists() {
                        self.load_project(&proj_file);
                    }
                    self.project_filename = Some(proj_file);
                }
                self.host.nsm_report_result(NSM_CALLBACK_OPEN, true);
            }
            NSM_CALLBACK_SAVE => {
                let ok = if let Some(ref path) = self.project_filename.clone() {
                    self.save_project_to(path)
                } else {
                    false
                };
                self.host.nsm_report_result(NSM_CALLBACK_SAVE, ok);
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

    // --- File & Project operations ---

    pub fn action_file_new(&mut self) {
        if !self.engine_running {
            return;
        }
        self.history.clear();
        if !self.slots.is_empty() {
            self.removing_all_plugins = true;
            let _ = self.host.remove_all_plugins();
        }
        self.host.clear_project_filename();
        self.project_filename = None;
        self.drain_engine_events();
        logging::log_ui("New project created");
    }

    pub fn action_file_open(&mut self) {
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
        if !self.host.load_project(&path_str) {
            self.is_project_loading = false;
            let err = self.host.get_last_error();
            logging::log_error(&format!("Failed to load project {path_str}: {err}"));
            self.alert
                .show("Load Project Failed", format!("Could not load project:\n{err}"));
        } else {
            self.project_filename = Some(path.to_path_buf());
            logging::log_ui(&format!("Loaded project: {path_str}"));
        }
        self.drain_engine_events();
        self.is_project_loading = false;
        self.history.set_blocked(false);
    }

    pub fn load_project_or_file(&mut self, path: &Path) {
        if !self.engine_running {
            return;
        }
        if is_project_file(path) {
            self.load_project(path);
        } else {
            let s = path.to_string_lossy().into_owned();
            if !self.host.load_file(&s) {
                let err = self.host.get_last_error();
                self.alert
                    .show("Load File Failed", format!("Could not load file:\n{err}"));
            } else {
                logging::log_ui(&format!("Loaded file: {s}"));
            }
            self.drain_engine_events();
        }
    }

    pub fn action_file_save(&mut self) {
        if let Some(path) = self.project_filename.clone() {
            self.save_project_to(&path);
        } else {
            self.action_file_save_as();
        }
    }

    pub fn action_file_save_as(&mut self) {
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
        if !self.host.save_project(&s) {
            let err = self.host.get_last_error();
            logging::log_error(&format!("Save project failed: {err}"));
            self.alert
                .show("Save Project Failed", format!("Could not save project:\n{err}"));
            false
        } else {
            self.project_filename = Some(path.to_path_buf());
            logging::log_ui(&format!("Saved project: {s}"));
            true
        }
    }

    // --- Plugin actions with full EditHistory ---

    fn move_plugin_raw(&mut self, source: u32, target: u32) -> u32 {
        let count = self.slots.len() as u32;
        if source == target || source >= count || target >= count {
            return source;
        }
        let mut cur = source;
        let step: i32 = if target > source { 1 } else { -1 };
        while cur != target {
            let next = (cur as i32 + step) as u32;
            if !self.host.switch_plugins(cur, next) {
                let err = self.host.get_last_error();
                self.alert
                    .show("Move Failed", format!("Could not move plugin:\n{err}"));
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
            if self.selected_rack_plugin == Some(source) {
                self.selected_rack_plugin = Some(cur);
            }
            let _ = self.host.patchbay_refresh(self.external_patchbay);
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
        if !self.host.rename_plugin(plugin_id, new_name) {
            let err = self.host.get_last_error();
            self.alert
                .show("Rename Failed", format!("Could not rename plugin:\n{err}"));
            return;
        }
        self.drain_engine_events();
        let after = self.host.get_plugin_info(plugin_id).name;
        if let Some(slot) = self.slots.get_mut(plugin_id as usize) {
            slot.info.name = after.clone();
        }
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
        let before = self.host.get_internal_parameter_value(plugin_id, PARAMETER_ACTIVE) >= 0.5;
        self.host.set_active(plugin_id, active);
        let after = self.host.get_internal_parameter_value(plugin_id, PARAMETER_ACTIVE) >= 0.5;
        if let Some(slot) = self.slots.get_mut(plugin_id as usize) {
            slot.active = after;
            if !after {
                slot.clip_warning.clear();
            }
        }
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
        let before = self.host.get_internal_parameter_value(plugin_id, param_id);
        match param_id {
            PARAMETER_DRYWET => self.host.set_drywet(plugin_id, value),
            PARAMETER_VOLUME => self.host.set_volume(plugin_id, value),
            PARAMETER_BALANCE_LEFT => self.host.set_balance_left(plugin_id, value),
            PARAMETER_BALANCE_RIGHT => self.host.set_balance_right(plugin_id, value),
            PARAMETER_PANNING => self.host.set_panning(plugin_id, value),
            _ => return,
        }
        let after = self.host.get_internal_parameter_value(plugin_id, param_id);
        if let Some(slot) = self.slots.get_mut(plugin_id as usize) {
            slot.edit_state
                .set_parameter_value_cached(param_id, after, &self.host, plugin_id);
        }
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
        let before = self.host.get_current_parameter_value(plugin_id, param_id);
        self.host.set_parameter_value(plugin_id, param_id, value);
        let after = self.host.get_current_parameter_value(plugin_id, param_id);
        if let Some(slot) = self.slots.get_mut(plugin_id as usize) {
            slot.edit_state
                .set_parameter_value_cached(param_id as i32, after, &self.host, plugin_id);
        }
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
        let count = self.host.get_parameter_count(plugin_id);
        let mut inputs = Vec::new();
        for i in 0..count {
            if self.host.get_parameter_data(plugin_id, i).ptype == PARAMETER_INPUT {
                inputs.push((i, self.host.get_current_parameter_value(plugin_id, i)));
            }
        }
        op(&self.host, plugin_id);
        let mut before = Vec::new();
        let mut after = Vec::new();
        for (i, bv) in inputs {
            let av = self.host.get_current_parameter_value(plugin_id, i);
            if (bv - av).abs() > 1e-7 {
                before.push((i, bv));
                after.push((i, av));
            }
        }
        if let Some(slot) = self.slots.get_mut(plugin_id as usize) {
            slot.edit_state.reload_parameters(&self.host, plugin_id);
        }
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
        let Some(slot) = self.slots.get_mut(plugin_id as usize) else {
            return;
        };
        let before = slot.category;
        if before == category {
            return;
        }
        slot.set_category(&self.host, category, true);
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
            let snap = PluginSnapshot::capture(&self.host, plugin_id);
            let ok = if snap.is_ok() {
                let res = self.host.remove_plugin(plugin_id);
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
        if !self.host.remove_plugin(plugin_id) {
            let err = self.host.get_last_error();
            self.alert
                .show("Remove Failed", format!("Could not remove plugin:\n{err}"));
        }
        self.drain_engine_events();
    }

    pub fn add_discovered_plugin(&mut self, p: &DiscoveredPlugin, replace_id: Option<u32>) {
        if !self.engine_running {
            self.alert
                .show("Engine Stopped", "Start the audio engine before adding plugins.");
            return;
        }
        if let Some(rid) = replace_id {
            let _ = self.host.replace_plugin(rid);
        }
        let name_opt = if p.name.is_empty() {
            None
        } else {
            Some(p.name.as_str())
        };
        if !self.host.add_plugin(
            p.build,
            p.ptype,
            &p.filename,
            name_opt,
            &p.label,
            p.unique_id as i64,
            PLUGIN_OPTIONS_NULL,
        ) {
            let err = self.host.get_last_error();
            logging::log_error(&format!("Failed to load plugin {}: {err}", p.name));
            self.alert
                .show("Failed to Load Plugin", format!("Could not load '{}':\n{err}", p.name));
        }
        self.drain_engine_events();
    }

    // --- Patchbay operations with EditHistory ---

    pub fn switch_patchbay_view(&mut self, external: bool) {
        if self.external_patchbay == external && !self.patchbay.groups.is_empty() {
            return;
        }
        self.external_patchbay = external;
        self.patchbay.clear();
        self.patchbay.needs_initial_fit = true;
        if self.engine_running {
            let _ = self.host.patchbay_refresh(external);
            self.drain_engine_events();
        }
    }

    pub fn refresh_patchbay(&mut self) {
        if !self.engine_running {
            return;
        }
        self.patchbay.clear();
        let _ = self.host.patchbay_refresh(self.external_patchbay);
        self.drain_engine_events();
    }

    pub fn connect_patchbay_ports(&mut self, g_out: u32, p_out: u32, g_in: u32, p_in: u32) {
        let ep_out = self.patchbay.endpoint_for(g_out, p_out);
        let ep_in = self.patchbay.endpoint_for(g_in, p_in);
        if !self
            .host
            .patchbay_connect(self.external_patchbay, g_out, p_out, g_in, p_in)
        {
            logging::log_warn(&format!(
                "Patchbay connect failed: {}",
                self.host.get_last_error()
            ));
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
        if !self
            .host
            .patchbay_disconnect(self.external_patchbay, connection_id)
        {
            logging::log_warn(&format!(
                "Patchbay disconnect failed: {}",
                self.host.get_last_error()
            ));
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
        let pos_actions = self.patchbay.arrange_nodes();
        for act in pos_actions {
            if let PatchbayAction::GroupPosition(gid, x1, y1, x2, y2) = act {
                let _ = self
                    .host
                    .patchbay_set_group_pos(self.external_patchbay, gid, x1, y1, x2, y2);
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
    }

    pub fn save_patchbay_image(&mut self, scale: f32) {
        if let Some(mut path) = rfd::FileDialog::new()
            .set_title("Save Patchbay Image")
            .add_filter("PNG Image", &["png"])
            .set_file_name("patchbay.png")
            .save_file()
        {
            if path.extension().is_none() {
                path.set_extension("png");
            }
            if let Err(e) = self.patchbay.save_png(&path, scale) {
                self.alert
                    .show("Save Image Failed", format!("Could not save PNG:\n{e}"));
            } else {
                logging::log_ui(&format!("Saved patchbay image to {}", path.display()));
            }
        }
    }

    pub fn copy_patchbay_image_to_clipboard(&mut self) {
        let (w, h, rgba) = self.patchbay.render_to_rgba(2.0);
        match arboard::Clipboard::new() {
            Ok(mut cb) => {
                let img = arboard::ImageData {
                    width: w as usize,
                    height: h as usize,
                    bytes: std::borrow::Cow::Owned(rgba),
                };
                if let Err(e) = cb.set_image(img) {
                    self.alert
                        .show("Clipboard Error", format!("Could not copy image:\n{e}"));
                } else {
                    logging::log_ui("Copied patchbay image to clipboard");
                }
            }
            Err(e) => {
                self.alert
                    .show("Clipboard Error", format!("Clipboard unavailable:\n{e}"));
            }
        }
    }

    // --- Undo / Redo Execution ---

    pub fn perform_undo(&mut self) {
        if !self.history.can_undo() {
            return;
        }
        self.history.busy = true;
        // Take temporary ownership by running on the entry
        let err = self.apply_history_step(true);
        self.history.busy = false;
        if let Some(msg) = err {
            self.history.revert_undo_index();
            self.alert.show("Undo Failed", msg);
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
            self.alert.show("Redo Failed", msg);
        }
    }

    fn restore_plugin_snapshot(&mut self, plugin_id: u32, snap: &PluginSnapshot) -> Result<(), String> {
        let prev_count = self.slots.len() as u32;
        let path_str = snap.carxp_path().to_string_lossy().into_owned();
        if !self.host.load_file(&path_str) {
            self.drain_engine_events();
            if self.slots.len() as u32 > prev_count {
                let _ = self.host.remove_plugin(prev_count);
                self.drain_engine_events();
            }
            return Err(self.host.get_last_error());
        }
        self.drain_engine_events();
        if self.slots.len() as u32 != prev_count + 1 {
            return Err("The effect could not be restored.".into());
        }
        let moved = self.move_plugin_raw(prev_count, plugin_id);
        if moved != plugin_id {
            let _ = self.host.remove_plugin(moved);
            self.drain_engine_events();
            return Err("Could not restore the effect to its place in the rack.".into());
        }
        Ok(())
    }

    fn apply_history_step(&mut self, is_undo: bool) -> Option<String> {
        // Pull action out briefly to avoid borrowing self.history while mutating self
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
                if self.host.rename_plugin(*plugin_id, target) {
                    self.drain_engine_events();
                    if let Some(s) = self.slots.get_mut(*plugin_id as usize) {
                        s.info.name = target.to_string();
                    }
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
                if self.slots.get(*plugin_id as usize).is_none() {
                    Err("The effect is no longer in the rack.".into())
                } else {
                    self.host.set_active(*plugin_id, val);
                    if let Some(s) = self.slots.get_mut(*plugin_id as usize) {
                        s.active = val;
                    }
                    Ok(())
                }
            }
            HistoryAction::SetInternalParam {
                plugin_id,
                param_id,
                before,
                after,
            } => {
                let val = if is_undo { *before } else { *after };
                if self.slots.get(*plugin_id as usize).is_none() {
                    Err("The effect is no longer in the rack.".into())
                } else {
                    match *param_id {
                        PARAMETER_DRYWET => self.host.set_drywet(*plugin_id, val),
                        PARAMETER_VOLUME => self.host.set_volume(*plugin_id, val),
                        PARAMETER_BALANCE_LEFT => self.host.set_balance_left(*plugin_id, val),
                        PARAMETER_BALANCE_RIGHT => self.host.set_balance_right(*plugin_id, val),
                        PARAMETER_PANNING => self.host.set_panning(*plugin_id, val),
                        _ => {}
                    }
                    if let Some(s) = self.slots.get_mut(*plugin_id as usize) {
                        s.edit_state
                            .set_parameter_value_cached(*param_id, val, &self.host, *plugin_id);
                    }
                    Ok(())
                }
            }
            HistoryAction::SetParam {
                plugin_id,
                param_id,
                before,
                after,
            } => {
                let val = if is_undo { *before } else { *after };
                if self.slots.get(*plugin_id as usize).is_none() {
                    Err("The effect is no longer in the rack.".into())
                } else {
                    self.host.set_parameter_value(*plugin_id, *param_id, val);
                    if let Some(s) = self.slots.get_mut(*plugin_id as usize) {
                        s.edit_state.set_parameter_value_cached(
                            *param_id as i32,
                            val,
                            &self.host,
                            *plugin_id,
                        );
                    }
                    Ok(())
                }
            }
            HistoryAction::SetParamsBatch {
                plugin_id,
                before,
                after,
            } => {
                let vals = if is_undo { before } else { after };
                if self.slots.get(*plugin_id as usize).is_none() {
                    Err("The effect is no longer in the rack.".into())
                } else {
                    for &(pid, v) in vals.iter() {
                        self.host.set_parameter_value(*plugin_id, pid, v);
                    }
                    if let Some(s) = self.slots.get_mut(*plugin_id as usize) {
                        s.edit_state.reload_parameters(&self.host, *plugin_id);
                    }
                    Ok(())
                }
            }
            HistoryAction::SetCategory {
                plugin_id,
                before,
                after,
            } => {
                let val = if is_undo { *before } else { *after };
                if let Some(s) = self.slots.get_mut(*plugin_id as usize) {
                    s.set_category(&self.host, val, true);
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
                    match PluginSnapshot::capture(&self.host, *plugin_id) {
                        Ok(s) => {
                            *snapshot = Some(s);
                            if self.host.remove_plugin(*plugin_id) {
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
                    match PluginSnapshot::capture(&self.host, *plugin_id) {
                        Ok(s) => {
                            *snapshot = Some(s);
                            if self.host.remove_plugin(*plugin_id) {
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
                            if self.host.patchbay_connect(*external, go, po, gi, pi) {
                                self.drain_engine_events();
                                Ok(())
                            } else {
                                Err("The ports could not be connected.".into())
                            }
                        } else if !want_connect {
                            if let Some(cid) = existing {
                                if self.host.patchbay_disconnect(*external, cid) {
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
                let acts = self.patchbay.apply_positions(target);
                for a in acts {
                    if let PatchbayAction::GroupPosition(gid, x1, y1, x2, y2) = a {
                        let _ = self.host.patchbay_set_group_pos(*external, gid, x1, y1, x2, y2);
                    }
                }
                if *external != prev_ext {
                    self.switch_patchbay_view(prev_ext);
                }
                Ok(())
            }
        };

        // Put the action back in its slot
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

    // --- Keyboard Shortcuts ---

    fn handle_shortcuts(&mut self, ctx: &egui::Context) {
        if ctx.egui_wants_keyboard_input() {
            return;
        }
        let mut actions_flags = (false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false);
        ctx.input_mut(|i| {
            let ctrl = i.modifiers.ctrl;
            let shift = i.modifiers.shift;
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::N) {
                actions_flags.0 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::O) {
                actions_flags.1 = true;
            }
            if ctrl && shift && i.consume_key(egui::Modifiers::CTRL | egui::Modifiers::SHIFT, egui::Key::S) {
                actions_flags.2 = true;
            } else if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::S) {
                actions_flags.3 = true;
            }
            if ctrl && shift && i.consume_key(egui::Modifiers::CTRL | egui::Modifiers::SHIFT, egui::Key::Z) {
                actions_flags.5 = true;
            } else if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::Z) {
                actions_flags.4 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::Y) {
                actions_flags.5 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::G) {
                actions_flags.6 = true;
            }
            if (ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::R))
                || i.consume_key(egui::Modifiers::NONE, egui::Key::F5)
            {
                actions_flags.7 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::P) {
                actions_flags.8 = true;
            }
            if ctrl && (i.consume_key(egui::Modifiers::CTRL, egui::Key::Plus) || i.consume_key(egui::Modifiers::CTRL, egui::Key::Equals)) {
                actions_flags.9 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::Minus) {
                actions_flags.10 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::Num1) {
                actions_flags.11 = true;
            }
            if i.consume_key(egui::Modifiers::NONE, egui::Key::Home) {
                actions_flags.12 = true;
            }
            if i.consume_key(egui::Modifiers::NONE, egui::Key::Delete) {
                actions_flags.13 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::Q) {
                actions_flags.14 = true;
            }
            if ctrl && i.consume_key(egui::Modifiers::CTRL, egui::Key::A) {
                actions_flags.15 = true;
            }
        });

        if actions_flags.0 {
            self.action_file_new();
        }
        if actions_flags.1 {
            self.action_file_open();
        }
        if actions_flags.2 {
            self.action_file_save_as();
        }
        if actions_flags.3 {
            self.action_file_save();
        }
        if actions_flags.4 {
            self.perform_undo();
        }
        if actions_flags.5 {
            self.perform_redo();
        }
        if actions_flags.6 && self.active_tab == MainTab::Patchbay {
            self.arrange_patchbay();
        }
        if actions_flags.7 && self.active_tab == MainTab::Patchbay {
            self.refresh_patchbay();
        }
        if actions_flags.8 && self.engine_running {
            self.add_plugin_dlg.open_for_add();
        }
        if actions_flags.9 && self.active_tab == MainTab::Patchbay {
            self.patchbay.zoom_in();
        }
        if actions_flags.10 && self.active_tab == MainTab::Patchbay {
            self.patchbay.zoom_out();
        }
        if actions_flags.11 && self.active_tab == MainTab::Patchbay {
            self.patchbay.zoom_reset();
        }
        if actions_flags.12 && self.active_tab == MainTab::Patchbay {
            self.patchbay.zoom_fit();
        }
        if actions_flags.13 && self.active_tab == MainTab::Rack {
            if let Some(pid) = self.selected_rack_plugin {
                self.remove_plugin(pid);
            }
        }
        if actions_flags.14 {
            if self.settings.confirm_exit && !self.slots.is_empty() {
                self.confirm = Some(ConfirmKind::QuitWithPlugins);
            } else {
                self.quit_confirmed = true;
                ctx.send_viewport_cmd(egui::ViewportCommand::Close);
            }
        }
        if actions_flags.15 && self.active_tab == MainTab::Patchbay {
            let geoms = self.patchbay.compute_geometries();
            self.patchbay.selected_boxes = geoms.into_iter().map(|g| g.key).collect();
        }
    }

    // --- Menu Bar ---

    fn show_menu_bar(&mut self, ctx: &egui::Context, ui: &mut egui::Ui) {
        egui::MenuBar::new().ui(ui, |ui| {
            // File menu
            ui.menu_button("File", |ui| {
                if ui
                    .add_enabled(self.engine_running && !self.nsm_active, egui::Button::new("New  (Ctrl+N)"))
                    .clicked()
                {
                    self.action_file_new();
                    ui.close();
                }
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Open...  (Ctrl+O)"))
                    .clicked()
                {
                    self.action_file_open();
                    ui.close();
                }
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Save  (Ctrl+S)"))
                    .clicked()
                {
                    self.action_file_save();
                    ui.close();
                }
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Save As...  (Ctrl+Shift+S)"))
                    .clicked()
                {
                    self.action_file_save_as();
                    ui.close();
                }
                ui.separator();
                if ui.button("Quit  (Ctrl+Q)").clicked() {
                    if self.settings.confirm_exit && !self.slots.is_empty() {
                        self.confirm = Some(ConfirmKind::QuitWithPlugins);
                    } else {
                        self.quit_confirmed = true;
                        ctx.send_viewport_cmd(egui::ViewportCommand::Close);
                    }
                    ui.close();
                }
            });

            // Edit menu
            ui.menu_button("Edit", |ui| {
                let undo_lbl = format!("{}  (Ctrl+Z)", self.history.undo_label());
                if ui
                    .add_enabled(self.history.can_undo(), egui::Button::new(undo_lbl))
                    .clicked()
                {
                    self.perform_undo();
                    ui.close();
                }
                let redo_lbl = format!("{}  (Ctrl+Y)", self.history.redo_label());
                if ui
                    .add_enabled(self.history.can_redo(), egui::Button::new(redo_lbl))
                    .clicked()
                {
                    self.perform_redo();
                    ui.close();
                }
            });

            // Engine menu
            ui.menu_button("Engine", |ui| {
                if ui
                    .add_enabled(!self.engine_running, egui::Button::new("Start"))
                    .clicked()
                {
                    self.start_engine(false);
                    ui.close();
                }
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Stop"))
                    .clicked()
                {
                    self.stop_engine();
                    ui.close();
                }
                ui.separator();
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Panic (All Notes Off)"))
                    .clicked()
                {
                    for pid in 0..self.slots.len() as u32 {
                        for ch in 0..16u8 {
                            for note in 0..128u8 {
                                self.host.send_midi_note(pid, ch, note, 0);
                            }
                        }
                    }
                    self.canvas_keyboard.all_notes_off();
                    ui.close();
                }
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Clear Xruns"))
                    .clicked()
                {
                    self.host.clear_engine_xruns();
                    ui.close();
                }
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Driver Settings..."))
                    .clicked()
                {
                    self.runtime_driver_dlg.open_from_host(&self.host);
                    ui.close();
                }
            });

            // Plugin menu
            ui.menu_button("Plugin", |ui| {
                if ui
                    .add_enabled(self.engine_running, egui::Button::new("Add Plugin...  (Ctrl+P)"))
                    .clicked()
                {
                    self.add_plugin_dlg.open_for_add();
                    ui.close();
                }
                if self.settings.experimental && self.settings.exp_jack_apps {
                    if ui
                        .add_enabled(self.engine_running, egui::Button::new("Add JACK Application..."))
                        .clicked()
                    {
                        self.jack_app_dlg.open = true;
                        ui.close();
                    }
                }
                if ui
                    .add_enabled(!self.slots.is_empty(), egui::Button::new("Remove All..."))
                    .clicked()
                {
                    self.confirm = Some(ConfirmKind::RemoveAllPlugins);
                    ui.close();
                }
                ui.separator();
                ui.add_enabled_ui(!self.slots.is_empty(), |ui| {
                    ui.menu_button("Macros", |ui| {
                        if ui.button("Enable All").clicked() {
                            for i in 0..self.slots.len() as u32 {
                                self.set_plugin_active(i, true);
                            }
                            ui.close();
                        }
                        if ui.button("Disable All").clicked() {
                            for i in 0..self.slots.len() as u32 {
                                self.set_plugin_active(i, false);
                            }
                            ui.close();
                        }
                        ui.separator();
                        if ui.button("Volume 100%").clicked() {
                            for i in 0..self.slots.len() as u32 {
                                if (self.slots[i as usize].info.hints & PLUGIN_CAN_VOLUME) != 0 {
                                    self.set_plugin_internal_param(i, PARAMETER_VOLUME, 1.0);
                                }
                            }
                            ui.close();
                        }
                        if ui.button("Mute All").clicked() {
                            for i in 0..self.slots.len() as u32 {
                                if (self.slots[i as usize].info.hints & PLUGIN_CAN_VOLUME) != 0 {
                                    self.set_plugin_internal_param(i, PARAMETER_VOLUME, 0.0);
                                }
                            }
                            ui.close();
                        }
                        ui.separator();
                        if ui.button("Dry/Wet 100%").clicked() {
                            for i in 0..self.slots.len() as u32 {
                                if (self.slots[i as usize].info.hints & PLUGIN_CAN_DRYWET) != 0 {
                                    self.set_plugin_internal_param(i, PARAMETER_DRYWET, 1.0);
                                }
                            }
                            ui.close();
                        }
                        if ui.button("Bypass All (Dry/Wet 0%)").clicked() {
                            for i in 0..self.slots.len() as u32 {
                                if (self.slots[i as usize].info.hints & PLUGIN_CAN_DRYWET) != 0 {
                                    self.set_plugin_internal_param(i, PARAMETER_DRYWET, 0.0);
                                }
                            }
                            ui.close();
                        }
                        if ui.button("Center Balance / Pan").clicked() {
                            for i in 0..self.slots.len() as u32 {
                                let hints = self.slots[i as usize].info.hints;
                                if (hints & PLUGIN_CAN_BALANCE) != 0 {
                                    self.set_plugin_internal_param(i, PARAMETER_BALANCE_LEFT, -1.0);
                                    self.set_plugin_internal_param(i, PARAMETER_BALANCE_RIGHT, 1.0);
                                }
                                if (hints & PLUGIN_CAN_PANNING) != 0 {
                                    self.set_plugin_internal_param(i, PARAMETER_PANNING, 0.0);
                                }
                            }
                            ui.close();
                        }
                    });
                });
            });

            // Canvas menu
            ui.menu_button("Canvas", |ui| {
                ui.add_enabled_ui(self.engine_running, |ui| {
                    if ui
                        .selectable_label(!self.external_patchbay, "Plugin Routing (Internal)")
                        .clicked()
                    {
                        self.switch_patchbay_view(false);
                        ui.close();
                    }
                    if ui
                        .selectable_label(self.external_patchbay, "System Routing (External)")
                        .clicked()
                    {
                        self.switch_patchbay_view(true);
                        ui.close();
                    }
                    ui.separator();
                    if ui.button("Arrange  (Ctrl+G)").clicked() {
                        self.arrange_patchbay();
                        ui.close();
                    }
                    if ui.button("Refresh  (Ctrl+R)").clicked() {
                        self.refresh_patchbay();
                        ui.close();
                    }
                    ui.separator();
                    ui.menu_button("Zoom", |ui| {
                        if ui.button("Auto-Fit  (Home)").clicked() {
                            self.patchbay.zoom_fit();
                            ui.close();
                        }
                        if ui.button("Zoom In  (Ctrl++)").clicked() {
                            self.patchbay.zoom_in();
                            ui.close();
                        }
                        if ui.button("Zoom Out  (Ctrl+-)").clicked() {
                            self.patchbay.zoom_out();
                            ui.close();
                        }
                        if ui.button("Zoom 100%  (Ctrl+1)").clicked() {
                            self.patchbay.zoom_reset();
                            ui.close();
                        }
                    });
                    ui.menu_button("Save Image", |ui| {
                        if ui.button("Save PNG (1x)...").clicked() {
                            self.save_patchbay_image(1.0);
                            ui.close();
                        }
                        if ui.button("Save PNG (2x)...").clicked() {
                            self.save_patchbay_image(2.0);
                            ui.close();
                        }
                        if ui.button("Save PNG (4x)...").clicked() {
                            self.save_patchbay_image(4.0);
                            ui.close();
                        }
                        ui.separator();
                        if ui.button("Copy to Clipboard").clicked() {
                            self.copy_patchbay_image_to_clipboard();
                            ui.close();
                        }
                    });
                });
            });

            // Settings menu
            ui.menu_button("Settings", |ui| {
                if ui.checkbox(&mut self.settings.show_toolbar, "Show Main Toolbar").changed() {
                    let _ = self.settings.save();
                }
                if ui
                    .checkbox(
                        &mut self.settings.show_side_panel,
                        "Show Files, Transport and Minimap",
                    )
                    .changed()
                {
                    let _ = self.settings.save();
                }
                if ui
                    .checkbox(&mut self.settings.show_meters, "Show Patchbay Peak Meters")
                    .changed()
                {
                    let _ = self.settings.save();
                }
                if ui
                    .checkbox(&mut self.settings.show_keyboard, "Show Patchbay Keyboard")
                    .changed()
                {
                    let _ = self.settings.save();
                }
                ui.separator();
                if ui.button("Configure Franca...").clicked() {
                    self.settings_dlg.open_with(&self.settings, &self.host);
                    ui.close();
                }
            });

            // Help menu
            ui.menu_button("Help", |ui| {
                if ui.button("About Franca").clicked() {
                    if self.about_dlg.features.is_empty() {
                        if let Some(ref u) = self.utils {
                            self.about_dlg.features = u.get_supported_features();
                            self.about_dlg.license_text = u.get_complete_license_text();
                        }
                    }
                    self.about_dlg.open = true;
                    ui.close();
                }
            });
        });
    }

    // --- Main Toolbar ---

    fn show_main_toolbar(&mut self, ui: &mut egui::Ui) {
        ui.horizontal(|ui| {
            if ui
                .add_enabled(self.engine_running && !self.nsm_active, egui::Button::new("📄 New"))
                .on_hover_text("New project (Ctrl+N)")
                .clicked()
            {
                self.action_file_new();
            }
            if ui
                .add_enabled(self.engine_running, egui::Button::new("📂 Open"))
                .on_hover_text("Open project (Ctrl+O)")
                .clicked()
            {
                self.action_file_open();
            }
            if ui
                .add_enabled(self.engine_running, egui::Button::new("💾 Save"))
                .on_hover_text("Save project (Ctrl+S)")
                .clicked()
            {
                self.action_file_save();
            }

            ui.separator();

            if ui
                .add_enabled(self.history.can_undo(), egui::Button::new("↶ Undo"))
                .on_hover_text(self.history.undo_label())
                .clicked()
            {
                self.perform_undo();
            }
            if ui
                .add_enabled(self.history.can_redo(), egui::Button::new("↷ Redo"))
                .on_hover_text(self.history.redo_label())
                .clicked()
            {
                self.perform_redo();
            }

            ui.separator();

            if ui
                .add_enabled(self.engine_running, egui::Button::new("＋ Add Plugin"))
                .on_hover_text("Add Plugin (Ctrl+P)")
                .clicked()
            {
                self.add_plugin_dlg.open_for_add();
            }

            ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                if let Some(ref p) = self.project_filename {
                    if let Some(name) = p.file_name().and_then(|n| n.to_str()) {
                        ui.label(egui::RichText::new(name).small().weak());
                    }
                }
            });
        });
    }

    // --- Status Bar ---

    fn show_status_bar(&mut self, ui: &mut egui::Ui) {
        ui.horizontal(|ui| {
            // Engine status indicator
            let (dot_col, status_txt) = if self.engine_running {
                (
                    Color32::from_rgb(34, 197, 94),
                    format!("{} running", self.running_driver_name),
                )
            } else {
                (Color32::from_rgb(156, 163, 175), "Audio stopped".to_string())
            };
            ui.colored_label(dot_col, "●");
            ui.label(status_txt);

            ui.separator();

            let count = self.slots.len();
            ui.label(if count == 1 {
                "1 plugin".to_string()
            } else {
                format!("{count} plugins")
            });

            ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                // DSP % load bar
                let load_pct = self.runtime_info.load.clamp(0.0, 100.0);
                ui.add(
                    egui::ProgressBar::new(load_pct / 100.0)
                        .desired_width(92.0)
                        .text(format!("DSP {:.0}%", load_pct)),
                );

                ui.separator();

                // Clickable Xruns counter
                let xruns = self.runtime_info.xruns;
                let xrun_lbl = if xruns == 1 {
                    "1 xrun".to_string()
                } else {
                    format!("{xruns} xruns")
                };
                let xrun_btn = ui
                    .add_enabled(
                        self.engine_running,
                        egui::Button::new(if xruns > 0 {
                            egui::RichText::new(xrun_lbl).color(Color32::from_rgb(249, 115, 22))
                        } else {
                            egui::RichText::new(xrun_lbl)
                        })
                        .frame(false),
                    )
                    .on_hover_text("Click to clear xruns");
                if xrun_btn.clicked() {
                    self.host.clear_engine_xruns();
                    self.runtime_info.xruns = 0;
                }

                ui.separator();

                // Live Sample Rate selector
                let var_sr =
                    (self.runtime_dev_info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE) != 0;
                if self.engine_running && var_sr && !self.runtime_dev_info.sample_rates.is_empty() {
                    let mut cur_sr = self.sample_rate;
                    let rates = self.runtime_dev_info.sample_rates.clone();
                    egui::ComboBox::from_id_salt("status_sr_cb")
                        .selected_text(format!("{} Hz", cur_sr as u32))
                        .width(82.0)
                        .show_ui(ui, |ui| {
                            for sr in rates {
                                if ui
                                    .selectable_value(&mut cur_sr, sr, format!("{} Hz", sr as u32))
                                    .clicked()
                                {
                                    let _ = self
                                        .host
                                        .set_engine_buffer_size_and_sample_rate(self.buffer_size, sr);
                                }
                            }
                        });
                } else if self.engine_running {
                    ui.label(format!("{} Hz", self.sample_rate as u32));
                } else {
                    ui.label("— Hz");
                }
                ui.label(egui::RichText::new("Sample rate").weak().small());

                ui.separator();

                // Live Buffer Size selector
                let var_buf =
                    (self.runtime_dev_info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE) != 0;
                if self.engine_running && var_buf && !self.runtime_dev_info.buffer_sizes.is_empty() {
                    let mut cur_buf = self.buffer_size;
                    let sizes = self.runtime_dev_info.buffer_sizes.clone();
                    egui::ComboBox::from_id_salt("status_buf_cb")
                        .selected_text(format!("{cur_buf}"))
                        .width(66.0)
                        .show_ui(ui, |ui| {
                            for b in sizes {
                                if ui.selectable_value(&mut cur_buf, b, format!("{b}")).clicked() {
                                    let _ = self
                                        .host
                                        .set_engine_buffer_size_and_sample_rate(b, self.sample_rate);
                                }
                            }
                        });
                } else if self.engine_running {
                    ui.label(format!("{}", self.buffer_size));
                } else {
                    ui.label("—");
                }
                ui.label(egui::RichText::new("Buffer").weak().small());
            });
        });
    }

    // --- Dispatchers ---

    fn process_rack_actions(&mut self, actions: Vec<RackAction>) {
        for act in actions {
            match act {
                RackAction::Select(pid) => self.selected_rack_plugin = Some(pid),
                RackAction::SetActive(pid, on) => self.set_plugin_active(pid, on),
                RackAction::ToggleCustomUi(pid, show) => {
                    self.host.show_custom_ui(pid, show);
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        s.custom_ui_visible = show;
                    }
                }
                RackAction::ToggleEdit(pid) => {
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        s.edit_open = !s.edit_open;
                        if s.edit_open {
                            s.edit_state.reload_all(&self.host, pid);
                        }
                    }
                }
                RackAction::MovePlugin(from, to) => self.move_plugin(from, to),
                RackAction::SetCategory(pid, cat) => self.set_plugin_category(pid, cat),
                RackAction::ResetParameters(pid) => {
                    self.change_plugin_params_batch(pid, "Reset Parameters", |h, id| {
                        h.reset_parameters(id)
                    });
                }
                RackAction::RandomizeParameters(pid) => {
                    self.change_plugin_params_batch(pid, "Randomize Parameters", |h, id| {
                        h.randomize_parameters(id)
                    });
                }
                RackAction::ClonePlugin(pid) => {
                    if !self.host.clone_plugin(pid) {
                        self.alert
                            .show("Clone Failed", self.host.get_last_error());
                    }
                    self.drain_engine_events();
                }
                RackAction::RenamePlugin(pid) => {
                    if let Some(s) = self.slots.get(pid as usize) {
                        let name = s.info.name.clone();
                        self.rename_dlg.open_for(pid, &name);
                    }
                }
                RackAction::ReplacePlugin(pid) => {
                    self.add_plugin_dlg.open_for_replace(pid);
                }
                RackAction::RemovePlugin(pid) => self.remove_plugin(pid),
                RackAction::ExportLv2(pid) => {
                    if let Some(dir) = rfd::FileDialog::new()
                        .set_title("Export Plugin as LV2 Bundle")
                        .pick_folder()
                    {
                        if !self
                            .host
                            .export_plugin_lv2(pid, &dir.to_string_lossy())
                        {
                            self.alert
                                .show("LV2 Export Failed", self.host.get_last_error());
                        }
                    }
                }
                RackAction::FindInPatchbay(pid) => {
                    let name = self
                        .slots
                        .get(pid as usize)
                        .map(|s| s.info.name.clone())
                        .unwrap_or_default();
                    if self.external_patchbay {
                        self.switch_patchbay_view(false);
                    }
                    if self.patchbay.focus_plugin(pid, &name) {
                        self.active_tab = MainTab::Patchbay;
                    }
                }
                RackAction::AddPlugin => {
                    if self.engine_running {
                        self.add_plugin_dlg.open_for_add();
                    }
                }
            }
        }
    }

    fn process_plugin_edit_actions(&mut self, actions: Vec<PluginEditAction>) {
        for act in actions {
            match act {
                PluginEditAction::SetActive(pid, on) => self.set_plugin_active(pid, on),
                PluginEditAction::SetInternalParam(pid, param, val) => {
                    self.set_plugin_internal_param(pid, param, val);
                }
                PluginEditAction::SetCtrlChannel(pid, ch) => self.host.set_ctrl_channel(pid, ch),
                PluginEditAction::SetOption(pid, opt, on) => {
                    self.host.set_option(pid, opt, on);
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        if on {
                            s.info.options_enabled |= opt;
                        } else {
                            s.info.options_enabled &= !opt;
                        }
                    }
                }
                PluginEditAction::SetParam(pid, idx, val) => {
                    self.set_plugin_param(pid, idx, val);
                }
                PluginEditAction::SetParamMidiChannel(pid, idx, ch) => {
                    self.host.set_parameter_midi_channel(pid, idx, ch);
                }
                PluginEditAction::SetParamMappedControl(pid, idx, cc) => {
                    self.host.set_parameter_mapped_control_index(pid, idx, cc);
                }
                PluginEditAction::SetParamMappedRange(pid, idx, rmin, rmax) => {
                    self.host.set_parameter_mapped_range(pid, idx, rmin, rmax);
                }
                PluginEditAction::SetProgram(pid, prog) => {
                    self.host.set_program(pid, prog);
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        s.edit_state.reload_parameters(&self.host, pid);
                    }
                }
                PluginEditAction::SetMidiProgram(pid, mprog) => {
                    self.host.set_midi_program(pid, mprog);
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        s.edit_state.reload_parameters(&self.host, pid);
                    }
                }
                PluginEditAction::ResetParameters(pid) => {
                    self.change_plugin_params_batch(pid, "Reset Parameters", |h, id| {
                        h.reset_parameters(id)
                    });
                }
                PluginEditAction::RandomizeParameters(pid) => {
                    self.change_plugin_params_batch(pid, "Randomize Parameters", |h, id| {
                        h.randomize_parameters(id)
                    });
                }
                PluginEditAction::LoadState(pid) => {
                    if let Some(path) = rfd::FileDialog::new()
                        .set_title("Load Plugin State")
                        .add_filter("Carla Plugin State", &["carxs"])
                        .pick_file()
                    {
                        if self.host.load_plugin_state(pid, &path.to_string_lossy()) {
                            if let Some(s) = self.slots.get_mut(pid as usize) {
                                s.edit_state.reload_all(&self.host, pid);
                            }
                        } else {
                            self.alert
                                .show("Load State Failed", self.host.get_last_error());
                        }
                    }
                }
                PluginEditAction::SaveState(pid) => {
                    if let Some(mut path) = rfd::FileDialog::new()
                        .set_title("Save Plugin State")
                        .add_filter("Carla Plugin State", &["carxs"])
                        .set_file_name("preset.carxs")
                        .save_file()
                    {
                        if path.extension().is_none() {
                            path.set_extension("carxs");
                        }
                        if !self.host.save_plugin_state(pid, &path.to_string_lossy()) {
                            self.alert
                                .show("Save State Failed", self.host.get_last_error());
                        }
                    }
                }
                PluginEditAction::ReloadAll(pid) => {
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        s.reload_info(&self.host);
                        s.edit_state.reload_all(&self.host, pid);
                    }
                }
                PluginEditAction::SendNote(pid, ev) => match ev {
                    NoteEvent::On {
                        channel,
                        note,
                        velocity,
                    } => self.host.send_midi_note(pid, channel, note, velocity),
                    NoteEvent::Off { channel, note } => self.host.send_midi_note(pid, channel, note, 0),
                },
            }
        }
    }

    fn process_patchbay_actions(&mut self, actions: Vec<PatchbayAction>) {
        for act in actions {
            match act {
                PatchbayAction::Connect(go, po, gi, pi) => {
                    self.connect_patchbay_ports(go, po, gi, pi);
                }
                PatchbayAction::Disconnect(cid) => {
                    self.disconnect_patchbay_ports(cid);
                }
                PatchbayAction::GroupPosition(gid, x1, y1, x2, y2) => {
                    if self.engine_running && !self.is_project_loading {
                        let _ = self
                            .host
                            .patchbay_set_group_pos(self.external_patchbay, gid, x1, y1, x2, y2);
                    }
                }
                PatchbayAction::SelectPlugins(pids) => {
                    self.selected_patchbay_plugins = pids;
                    if self.selected_patchbay_plugins.is_empty() {
                        self.canvas_peak_in.clear();
                        self.canvas_peak_out.clear();
                    }
                }
                PatchbayAction::PluginEdit(pid) => {
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        s.edit_open = true;
                        s.edit_state.reload_all(&self.host, pid);
                    }
                }
                PatchbayAction::PluginCustomUi(pid) => {
                    if let Some(s) = self.slots.get_mut(pid as usize) {
                        let next = !s.custom_ui_visible;
                        self.host.show_custom_ui(pid, next);
                        s.custom_ui_visible = next;
                    }
                }
                PatchbayAction::PluginClone(pid) => {
                    let _ = self.host.clone_plugin(pid);
                    self.drain_engine_events();
                }
                PatchbayAction::PluginRename(pid) => {
                    if let Some(s) = self.slots.get(pid as usize) {
                        let name = s.info.name.clone();
                        self.rename_dlg.open_for(pid, &name);
                    }
                }
                PatchbayAction::PluginReplace(pid) => {
                    self.add_plugin_dlg.open_for_replace(pid);
                }
                PatchbayAction::PluginRemove(pid) => self.remove_plugin(pid),
                PatchbayAction::ArrangeRequested => self.arrange_patchbay(),
                PatchbayAction::RefreshRequested => self.refresh_patchbay(),
            }
        }
    }

    fn process_side_panel_actions(&mut self, actions: Vec<SidePanelAction>) {
        for act in actions {
            match act {
                SidePanelAction::LoadFile(p) => self.load_project_or_file(&p),
                SidePanelAction::AddDiskFolder(dir) => {
                    if !self.settings.disk_folders.contains(&dir) {
                        self.settings.disk_folders.push(dir);
                        let _ = self.settings.save();
                    }
                }
                SidePanelAction::RemoveDiskFolder(idx) => {
                    if idx < self.settings.disk_folders.len() {
                        self.settings.disk_folders.remove(idx);
                        let _ = self.settings.save();
                    }
                }
                SidePanelAction::TransportPlayPause => {
                    if self.transport_info.playing {
                        self.host.transport_pause();
                    } else {
                        self.host.transport_play();
                    }
                }
                SidePanelAction::TransportStop => {
                    self.host.transport_pause();
                    self.host.transport_relocate(0);
                }
                SidePanelAction::TransportRewind => {
                    let step = (self.sample_rate.max(44100.0) * 2.0) as u64;
                    self.host
                        .transport_relocate(self.transport_info.frame.saturating_sub(step));
                }
                SidePanelAction::TransportForward => {
                    let step = (self.sample_rate.max(44100.0) * 2.0) as u64;
                    self.host
                        .transport_relocate(self.transport_info.frame.saturating_add(step));
                }
                SidePanelAction::TransportSetBpm(bpm) => self.host.transport_bpm(bpm),
                SidePanelAction::TransportToggleJack(on) => {
                    let mode = if on {
                        ENGINE_TRANSPORT_MODE_JACK
                    } else {
                        ENGINE_TRANSPORT_MODE_INTERNAL
                    };
                    self.transport_mode = mode;
                    self.settings.transport_mode = mode;
                    self.host
                        .set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, mode, &self.settings.transport_extra);
                }
                SidePanelAction::TransportToggleLink(on) => {
                    self.settings.transport_extra = if on { ":link:".into() } else { String::new() };
                    self.host.set_engine_option(
                        ENGINE_OPTION_TRANSPORT_MODE,
                        self.transport_mode,
                        &self.settings.transport_extra,
                    );
                }
            }
        }
    }

    fn start_plugin_discovery(&mut self, opts: DiscoveryOptions) {
        if let Some(ref u) = self.utils {
            let runner = DiscoveryRunner::start(
                u,
                &self.bin_dir,
                &self.settings,
                &self.db.file_cache,
                &opts,
            );
            self.add_plugin_dlg.runner = Some(runner);
            logging::log_ui("Started plugin discovery scan...");
        } else {
            self.alert.show(
                "Discovery Unavailable",
                "libcarla_utils could not be loaded; plugin scanning is unavailable.",
            );
        }
    }
}

impl eframe::App for FrancaApp {
    fn ui(&mut self, ui: &mut egui::Ui, _frame: &mut eframe::Frame) {
        let ctx = ui.ctx().clone();
        self.apply_theme(&ctx);

        // Keep window title synced with active project filename
        let desired_title = match self
            .project_filename
            .as_ref()
            .and_then(|p| p.file_name())
            .and_then(|n| n.to_str())
        {
            Some(name) => format!("Franca — {name}"),
            None => "Franca".to_string(),
        };
        if desired_title != self.last_window_title {
            self.last_window_title = desired_title.clone();
            ctx.send_viewport_cmd(egui::ViewportCommand::Title(desired_title));
        }

        // Handle OS signals
        if SIG_SAVE_REQUESTED.swap(false, Ordering::SeqCst) {
            if let Some(ref path) = self.project_filename.clone() {
                self.save_project_to(path);
            }
        }
        if SIG_QUIT_REQUESTED.swap(false, Ordering::SeqCst) {
            self.quit_confirmed = true;
            ctx.send_viewport_cmd(egui::ViewportCommand::Close);
        }

        // Handle window close confirmation
        if ctx.input(|i| i.viewport().close_requested()) && !self.quit_confirmed {
            if self.settings.confirm_exit && !self.slots.is_empty() {
                ctx.send_viewport_cmd(egui::ViewportCommand::CancelClose);
                self.confirm = Some(ConfirmKind::QuitWithPlugins);
            } else {
                self.quit_confirmed = true;
            }
        }

        // Pump Carla engine idle + callbacks
        if self.engine_running || self.nsm_active {
            self.host.engine_idle();
        }
        self.drain_engine_events();

        let now_secs = monotonic_secs(self.epoch);

        // Fast poll: peak meters & transport
        if self.engine_running {
            for slot in &mut self.slots {
                slot.update_peaks(&self.host, now_secs);
            }
            if let Some(&first_pid) = self.selected_patchbay_plugins.first() {
                let il = self.host.get_input_peak_value(first_pid, true);
                let ir = self.host.get_input_peak_value(first_pid, false);
                let ol = self.host.get_output_peak_value(first_pid, true);
                let or = self.host.get_output_peak_value(first_pid, false);
                self.canvas_peak_in.update_channel(0, il, now_secs);
                self.canvas_peak_in.update_channel(1, ir, now_secs);
                self.canvas_peak_out.update_channel(0, ol, now_secs);
                self.canvas_peak_out.update_channel(1, or, now_secs);
            }
            self.transport_info = self.host.get_transport_info();

            if self.last_slow_poll.elapsed() >= Duration::from_millis(250) {
                self.last_slow_poll = Instant::now();
                self.runtime_info = self.host.get_runtime_engine_info();
                for slot in &mut self.slots {
                    if slot.edit_open {
                        slot.edit_state.refresh_live_values(&self.host, slot.plugin_id);
                    }
                }
            }
        }

        // Handle drag-and-drop files onto the window
        let dropped: Vec<PathBuf> = ctx.input(|i| {
            i.raw
                .dropped_files
                .iter()
                .map(|f| f.path().to_path_buf())
                .collect()
        });
        for path in dropped {
            if is_supported_file(&path, &self.supported_exts) {
                self.load_project_or_file(&path);
            }
        }

        self.handle_shortcuts(&ctx);

        // Top Menu Bar & Main Toolbar
        egui::Panel::top("franca_top_panel").resizable(false).show(ui, |ui| {
            self.show_menu_bar(&ctx, ui);
            if self.settings.show_toolbar {
                ui.separator();
                self.show_main_toolbar(ui);
            }
        });

        // Bottom Status Bar
        egui::Panel::bottom("franca_status_bar").resizable(false).show(ui, |ui| {
            self.show_status_bar(ui);
        });

        // Left Collapsible Side Panel (Files, Transport, Minimap)
        if self.settings.show_side_panel {
            egui::Panel::left("franca_side_panel")
                .resizable(true)
                .default_size(230.0)
                .min_size(180.0)
                .show(ui, |ui| {
                    let acts = show_side_panel(
                        ui,
                        &mut self.side_panel,
                        &self.settings.disk_folders,
                        &self.supported_exts,
                        self.engine_running,
                        &self.transport_info,
                        self.sample_rate,
                        self.transport_mode,
                        &mut self.patchbay,
                    );
                    self.process_side_panel_actions(acts);
                });
        }

        // Main Central Area (Tabs: Rack / Patchbay / Logs)
        egui::CentralPanel::default().show(ui, |ui| {
            // Tab Bar
            ui.horizontal(|ui| {
                ui.selectable_value(&mut self.active_tab, MainTab::Rack, "  Rack  ");
                ui.selectable_value(&mut self.active_tab, MainTab::Patchbay, "  Patchbay  ");
                if self.settings.show_logs {
                    ui.selectable_value(&mut self.active_tab, MainTab::Logs, "  Logs  ");
                } else if self.active_tab == MainTab::Logs {
                    self.active_tab = MainTab::Rack;
                }
            });
            ui.separator();

            match self.active_tab {
                MainTab::Rack => {
                    let rack_acts = show_rack_view(
                        ui,
                        &mut self.slots,
                        self.selected_rack_plugin,
                        &mut self.rack_drag,
                        now_secs,
                        self.settings.experimental && self.settings.exp_export_lv2,
                        self.engine_running,
                    );
                    self.process_rack_actions(rack_acts);
                }
                MainTab::Patchbay => {
                    // Patchbay Toolbar
                    ui.horizontal(|ui| {
                        let show_mode_toggle =
                            self.process_mode != ENGINE_PROCESS_MODE_SINGLE_CLIENT
                                && self.process_mode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
                        if show_mode_toggle {
                            if ui
                                .selectable_label(!self.external_patchbay, "Plugin Routing")
                                .clicked()
                            {
                                self.switch_patchbay_view(false);
                            }
                            if ui
                                .selectable_label(self.external_patchbay, "System Routing")
                                .clicked()
                            {
                                self.switch_patchbay_view(true);
                            }
                            ui.separator();
                        }

                        if ui
                            .add_enabled(self.engine_running, egui::Button::new("↻ Refresh"))
                            .on_hover_text("Refresh Patchbay (Ctrl+R)")
                            .clicked()
                        {
                            self.refresh_patchbay();
                        }
                        if ui
                            .add_enabled(self.engine_running, egui::Button::new("⊞ Arrange"))
                            .on_hover_text("Auto-arrange left-to-right grid (Ctrl+G)")
                            .clicked()
                        {
                            self.arrange_patchbay();
                        }

                        ui.separator();

                        if ui
                            .add_enabled(self.history.can_undo(), egui::Button::new("↶ Undo"))
                            .on_hover_text(self.history.undo_label())
                            .clicked()
                        {
                            self.perform_undo();
                        }
                        if ui
                            .add_enabled(self.history.can_redo(), egui::Button::new("↷ Redo"))
                            .on_hover_text(self.history.redo_label())
                            .clicked()
                        {
                            self.perform_redo();
                        }

                        ui.separator();

                        if ui
                            .button("⊡ Auto-Fit")
                            .on_hover_text("Fit all nodes in view (Home)")
                            .clicked()
                        {
                            self.patchbay.zoom_fit();
                        }
                        if ui
                            .button("−")
                            .on_hover_text("Zoom Out (Ctrl+-)")
                            .clicked()
                        {
                            self.patchbay.zoom_out();
                        }
                        ui.label(
                            egui::RichText::new(format!("{:.0}%", self.patchbay.zoom * 100.0))
                                .small()
                                .weak(),
                        );
                        if ui
                            .button("+")
                            .on_hover_text("Zoom In (Ctrl++)")
                            .clicked()
                        {
                            self.patchbay.zoom_in();
                        }
                    });
                    ui.separator();

                    // Optional bottom piano keyboard inside Patchbay
                    let kb_h = if self.settings.show_keyboard { 104.0 } else { 0.0 };
                    let meters_w = if self.settings.show_meters { 56.0 } else { 0.0 };
                    let canvas_h = (ui.available_height() - kb_h).max(160.0);

                    ui.horizontal(|ui| {
                        ui.allocate_ui(
                            Vec2::new((ui.available_width() - meters_w).max(200.0), canvas_h),
                            |ui| {
                                let pb_acts = show_patchbay_canvas(ui, &mut self.patchbay);
                                self.process_patchbay_actions(pb_acts);
                            },
                        );

                        if self.settings.show_meters {
                            ui.separator();
                            ui.vertical(|ui| {
                                ui.label(egui::RichText::new("In / Out").small().weak());
                                ui.horizontal(|ui| {
                                    self.canvas_peak_in.draw_vertical(
                                        ui,
                                        Vec2::new(18.0, (canvas_h - 24.0).max(80.0)),
                                        now_secs,
                                        "Selected plugin input peak",
                                    );
                                    self.canvas_peak_out.draw_vertical(
                                        ui,
                                        Vec2::new(18.0, (canvas_h - 24.0).max(80.0)),
                                        now_secs,
                                        "Selected plugin output peak",
                                    );
                                });
                            });
                        }
                    });

                    if self.settings.show_keyboard {
                        ui.separator();
                        let enabled = !self.selected_patchbay_plugins.is_empty();
                        let note_evs = self.canvas_keyboard.show(ui, enabled);
                        for ev in note_evs {
                            for &pid in &self.selected_patchbay_plugins {
                                match ev {
                                    NoteEvent::On {
                                        channel,
                                        note,
                                        velocity,
                                    } => self.host.send_midi_note(pid, channel, note, velocity),
                                    NoteEvent::Off { channel, note } => {
                                        self.host.send_midi_note(pid, channel, note, 0)
                                    }
                                }
                            }
                        }
                    }
                }
                MainTab::Logs => {
                    show_logs_tab(ui, &mut self.logs_tab);
                }
            }
        });

        // Floating Per-Plugin Parameter Editor Windows
        let mut all_edit_actions = Vec::new();
        for slot in &mut self.slots {
            if slot.edit_open {
                let mut open = slot.edit_open;
                let acts = show_plugin_edit_window(
                    &ctx,
                    slot.plugin_id,
                    &slot.info,
                    slot.active,
                    &mut open,
                    &mut slot.edit_state,
                );
                slot.edit_open = open;
                all_edit_actions.extend(acts);
            }
        }
        self.process_plugin_edit_actions(all_edit_actions);

        // Add / Replace Plugin Dialog
        let add_acts = show_add_plugin_dialog(
            &ctx,
            &mut self.add_plugin_dlg,
            &mut self.db,
            &self.settings.favorite_plugins,
            self.utils.as_ref(),
        );
        for act in add_acts {
            match act {
                AddPluginAction::AddPlugin(p) => self.add_discovered_plugin(&p, None),
                AddPluginAction::ReplacePlugin(rid, p) => self.add_discovered_plugin(&p, Some(rid)),
                AddPluginAction::ToggleFavorite(key) => {
                    if !self.settings.favorite_plugins.remove(&key) {
                        self.settings.favorite_plugins.insert(key);
                    }
                    let _ = self.settings.save();
                }
                AddPluginAction::StartRefresh(opts) => self.start_plugin_discovery(opts),
            }
        }

        // Settings Dialog
        if let Some(new_settings) = show_settings_dialog(&ctx, &mut self.settings_dlg, &self.host) {
            self.settings = new_settings;
            let _ = self.settings.save();
            self.settings
                .apply_to_host(&self.host, &self.bin_dir, &self.res_dir);
            self.patchbay.canvas_size = Vec2::new(
                self.settings.canvas_width as f32,
                self.settings.canvas_height as f32,
            );
            self.patchbay.use_bezier = self.settings.canvas_use_bezier;
            self.patchbay.auto_hide_groups = self.settings.canvas_auto_hide_groups;
            if self.engine_running {
                let _ = self.host.patchbay_refresh(self.external_patchbay);
            }
            logging::log_ui("Settings updated");
        }

        // Runtime Driver Settings Dialog
        if let Some((buf, sr)) =
            show_runtime_driver_dialog(&ctx, &mut self.runtime_driver_dlg, &self.host)
        {
            if !self.host.set_engine_buffer_size_and_sample_rate(buf, sr) {
                self.alert
                    .show("Driver Error", self.host.get_last_error());
            } else {
                self.refresh_runtime_state();
            }
        }

        // Rename Dialog
        if let Some((pid, new_name)) = show_rename_dialog(&ctx, &mut self.rename_dlg) {
            self.rename_plugin(pid, &new_name);
        }

        // Add JACK App Dialog
        if let Some(spec) = show_add_jack_app_dialog(&ctx, &mut self.jack_app_dlg) {
            if !self.host.add_plugin(
                BINARY_NATIVE,
                PLUGIN_JACK,
                &spec.command,
                Some(&spec.name),
                &spec.label_setup,
                0,
                PLUGIN_OPTIONS_NULL,
            ) {
                self.alert
                    .show("Failed to Add JACK App", self.host.get_last_error());
            }
            self.drain_engine_events();
        }

        // About Dialog
        show_about_dialog(&ctx, &mut self.about_dlg);

        // Cancelable Action Modal
        if show_cancelable_action_modal(&ctx, self.cancelable_action_msg.as_deref()) {
            self.host.cancel_engine_action();
        }

        // Confirm Modal
        if let Some(confirmed) = show_confirm_modal(&ctx, &mut self.confirm) {
            match confirmed {
                ConfirmKind::RemoveAllPlugins => {
                    self.history.clear();
                    self.removing_all_plugins = true;
                    let _ = self.host.remove_all_plugins();
                    self.drain_engine_events();
                }
                ConfirmKind::QuitWithPlugins => {
                    self.quit_confirmed = true;
                    ctx.send_viewport_cmd(egui::ViewportCommand::Close);
                }
            }
        }

        // Alert Modal
        show_alert_modal(&ctx, &mut self.alert);

        // Schedule next refresh tick (keeps meters, transport & engine_idle responsive)
        let interval = Duration::from_millis(self.settings.refresh_interval_ms.clamp(10, 100) as u64);
        ctx.request_repaint_after(interval);
    }

    fn on_exit(&mut self, _gl: Option<&eframe::glow::Context>) {
        let _ = self.settings.save();
        if self.host.is_engine_running() {
            let _ = self.host.set_engine_about_to_close();
            let _ = self.host.engine_close();
        }
        logging::log_shutdown();
    }
}
