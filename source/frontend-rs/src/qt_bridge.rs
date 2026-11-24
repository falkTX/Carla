// SPDX-License-Identifier: GPL-2.0-or-later

#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;

        include!("cpp/qt_boot.h");
        #[namespace = "franca_qt"]
        pub fn run_qt_app() -> i32;
    }

    unsafe extern "RustQt" {
        #[qobject]
        #[qml_element]
        #[qproperty(QString, window_title)]
        #[qproperty(QString, project_name)]
        #[qproperty(bool, engine_running)]
        #[qproperty(QString, driver_name)]
        #[qproperty(i32, plugin_count)]
        #[qproperty(f32, dsp_load)]
        #[qproperty(i32, xruns)]
        #[qproperty(i32, buffer_size)]
        #[qproperty(i32, sample_rate)]
        #[qproperty(bool, variable_buffer_size)]
        #[qproperty(bool, variable_sample_rate)]
        #[qproperty(QString, buffer_sizes_json)]
        #[qproperty(QString, sample_rates_json)]
        #[qproperty(bool, can_undo)]
        #[qproperty(bool, can_redo)]
        #[qproperty(QString, undo_label)]
        #[qproperty(QString, redo_label)]
        #[qproperty(bool, show_toolbar)]
        #[qproperty(bool, show_side_panel)]
        #[qproperty(bool, show_meters)]
        #[qproperty(bool, show_keyboard)]
        #[qproperty(bool, show_logs)]
        #[qproperty(bool, confirm_exit)]
        #[qproperty(bool, experimental_enabled)]
        #[qproperty(bool, export_lv2_enabled)]
        #[qproperty(bool, jack_apps_enabled)]
        #[qproperty(i32, refresh_interval_ms)]
        #[qproperty(bool, external_patchbay)]
        #[qproperty(bool, show_routing_mode_toggle)]
        #[qproperty(bool, patchbay_use_bezier)]
        #[qproperty(f32, canvas_width)]
        #[qproperty(f32, canvas_height)]
        #[qproperty(i32, selected_patchbay_plugin_id)]
        #[qproperty(bool, transport_playing)]
        #[qproperty(f64, transport_bpm)]
        #[qproperty(QString, transport_frame_str)]
        #[qproperty(QString, transport_time_str)]
        #[qproperty(QString, transport_bbt_str)]
        #[qproperty(bool, transport_jack)]
        #[qproperty(bool, transport_link)]
        #[qproperty(QString, rack_json)]
        #[qproperty(QString, categories_json)]
        #[qproperty(QString, meters_json)]
        #[qproperty(QString, patchbay_json)]
        #[qproperty(QString, files_json)]
        #[qproperty(QString, logs_text)]
        #[qproperty(QString, log_dir)]
        #[qproperty(QString, cancelable_action_text)]
        #[qproperty(i32, discovery_revision)]
        #[qproperty(i32, plugin_edit_revision)]
        type FrancaController = super::FrancaControllerRust;

        #[qsignal]
        fn alert_requested(self: Pin<&mut FrancaController>, title: QString, message: QString);
        #[qsignal]
        fn focus_patchbay_requested(self: Pin<&mut FrancaController>);
        #[qsignal]
        fn quit_signal_received(self: Pin<&mut FrancaController>);

        // Lifecycle & Engine
        #[qinvokable]
        fn poll_tick(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn start_engine(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn stop_engine(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn engine_panic(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn clear_xruns(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn apply_buffer_size(self: Pin<&mut FrancaController>, size: i32);
        #[qinvokable]
        fn apply_sample_rate(self: Pin<&mut FrancaController>, rate: i32);
        #[qinvokable]
        fn open_device_control_panel(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn cancel_engine_action(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn shutdown_clean(self: Pin<&mut FrancaController>);

        // File & Project
        #[qinvokable]
        fn file_new(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn file_open(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn file_save(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn file_save_as(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn load_file_url(self: Pin<&mut FrancaController>, path_or_url: QString);

        // Undo / Redo
        #[qinvokable]
        fn undo(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn redo(self: Pin<&mut FrancaController>);

        // Rack Plugins
        #[qinvokable]
        fn set_plugin_active(self: Pin<&mut FrancaController>, plugin_id: i32, active: bool);
        #[qinvokable]
        fn toggle_custom_ui(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn move_plugin(self: Pin<&mut FrancaController>, source: i32, target: i32);
        #[qinvokable]
        fn set_plugin_category(self: Pin<&mut FrancaController>, plugin_id: i32, category: i32);
        #[qinvokable]
        fn dismiss_clip_warning(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn reset_parameters(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn randomize_parameters(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn clone_plugin(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn rename_plugin(self: Pin<&mut FrancaController>, plugin_id: i32, new_name: QString);
        #[qinvokable]
        fn remove_plugin(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn remove_all_plugins(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn export_plugin_lv2(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn find_plugin_in_patchbay(self: Pin<&mut FrancaController>, plugin_id: i32);

        // Plugin Macros
        #[qinvokable]
        fn macro_enable_all(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn macro_disable_all(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn macro_volume_100(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn macro_mute_all(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn macro_wet_100(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn macro_bypass_all(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn macro_center_balance(self: Pin<&mut FrancaController>);

        // Plugin Parameter Editor
        #[qinvokable]
        fn get_plugin_edit_json(self: &FrancaController, plugin_id: i32) -> QString;
        #[qinvokable]
        fn set_internal_param(self: Pin<&mut FrancaController>, plugin_id: i32, param_id: i32, value: f32);
        #[qinvokable]
        fn set_ctrl_channel(self: Pin<&mut FrancaController>, plugin_id: i32, channel: i32);
        #[qinvokable]
        fn set_plugin_option(self: Pin<&mut FrancaController>, plugin_id: i32, option_bit: i32, enabled: bool);
        #[qinvokable]
        fn set_param_value(self: Pin<&mut FrancaController>, plugin_id: i32, param_id: i32, value: f32);
        #[qinvokable]
        fn set_param_midi_channel(self: Pin<&mut FrancaController>, plugin_id: i32, param_id: i32, channel: i32);
        #[qinvokable]
        fn set_param_mapped_control(self: Pin<&mut FrancaController>, plugin_id: i32, param_id: i32, cc: i32);
        #[qinvokable]
        fn set_param_mapped_range(self: Pin<&mut FrancaController>, plugin_id: i32, param_id: i32, min_val: f32, max_val: f32);
        #[qinvokable]
        fn set_program(self: Pin<&mut FrancaController>, plugin_id: i32, program_id: i32);
        #[qinvokable]
        fn set_midi_program(self: Pin<&mut FrancaController>, plugin_id: i32, midi_program_id: i32);
        #[qinvokable]
        fn load_plugin_state(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn save_plugin_state(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn reload_plugin_all(self: Pin<&mut FrancaController>, plugin_id: i32);
        #[qinvokable]
        fn send_midi_note(self: Pin<&mut FrancaController>, plugin_id: i32, channel: i32, note: i32, velocity: i32);
        #[qinvokable]
        fn send_canvas_midi_note(self: Pin<&mut FrancaController>, channel: i32, note: i32, velocity: i32);

        // Add / Replace Plugin & Discovery
        #[qinvokable]
        fn query_plugins_json(
            self: &FrancaController,
            search: QString,
            favorites_only: bool,
            category_filter: i32,
            formats_mask: i32,
            types_mask: i32,
            sort_col: i32,
            sort_asc: bool,
        ) -> QString;
        #[qinvokable]
        fn add_plugin_by_key(self: Pin<&mut FrancaController>, key: QString, replace_id: i32);
        #[qinvokable]
        fn toggle_favorite(self: Pin<&mut FrancaController>, key: QString);
        #[qinvokable]
        fn start_discovery(self: Pin<&mut FrancaController>, formats_mask: i32, check_invalid: bool);
        #[qinvokable]
        fn skip_discovery(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn cancel_discovery(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn add_jack_app(
            self: Pin<&mut FrancaController>,
            name: QString,
            command: QString,
            audio_ins: i32,
            audio_outs: i32,
            midi_ins: i32,
            midi_outs: i32,
            session_mgr: i32,
            manage_window: bool,
        );

        // Patchbay
        #[qinvokable]
        fn switch_external_patchbay(self: Pin<&mut FrancaController>, external: bool);
        #[qinvokable]
        fn refresh_patchbay(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn arrange_patchbay(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn patchbay_connect(self: Pin<&mut FrancaController>, g_out: i32, p_out: i32, g_in: i32, p_in: i32);
        #[qinvokable]
        fn patchbay_disconnect(self: Pin<&mut FrancaController>, connection_id: i32);
        #[qinvokable]
        fn patchbay_disconnect_port(self: Pin<&mut FrancaController>, group_id: i32, port_id: i32, is_input: bool);
        #[qinvokable]
        fn patchbay_disconnect_group(self: Pin<&mut FrancaController>, group_id: i32);
        #[qinvokable]
        fn patchbay_move_box(self: Pin<&mut FrancaController>, group_id: i32, split_part: i32, x: f32, y: f32);
        #[qinvokable]
        fn patchbay_commit_group_pos(self: Pin<&mut FrancaController>, group_id: i32);
        #[qinvokable]
        fn patchbay_split_group(self: Pin<&mut FrancaController>, group_id: i32);
        #[qinvokable]
        fn patchbay_join_group(self: Pin<&mut FrancaController>, group_id: i32);
        #[qinvokable]
        fn patchbay_select_boxes(self: Pin<&mut FrancaController>, csv: QString);
        #[qinvokable]
        fn patchbay_cut_line(self: Pin<&mut FrancaController>, x1: f32, y1: f32, x2: f32, y2: f32);
        #[qinvokable]
        fn save_patchbay_image(self: Pin<&mut FrancaController>, scale: f32);
        #[qinvokable]
        fn copy_patchbay_image(self: Pin<&mut FrancaController>);

        // Side Panel: Files & Transport
        #[qinvokable]
        fn files_select_folder_index(self: Pin<&mut FrancaController>, idx: i32);
        #[qinvokable]
        fn files_navigate(self: Pin<&mut FrancaController>, path: QString);
        #[qinvokable]
        fn files_go_up(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn files_add_folder(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn files_remove_folder(self: Pin<&mut FrancaController>, idx: i32);
        #[qinvokable]
        fn transport_play_pause(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn transport_stop(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn transport_rewind(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn transport_forward(self: Pin<&mut FrancaController>);
        #[qinvokable]
        fn transport_set_bpm(self: Pin<&mut FrancaController>, bpm: f64);
        #[qinvokable]
        fn transport_set_jack(self: Pin<&mut FrancaController>, enabled: bool);
        #[qinvokable]
        fn transport_set_link(self: Pin<&mut FrancaController>, enabled: bool);

        // Layout Toggles & Settings
        #[qinvokable]
        fn set_layout_toggle(self: Pin<&mut FrancaController>, key: QString, value: bool);
        #[qinvokable]
        fn load_settings_draft_json(self: &FrancaController) -> QString;
        #[qinvokable]
        fn query_driver_devices_json(self: &FrancaController, driver: QString, device: QString) -> QString;
        #[qinvokable]
        fn open_driver_control_panel(self: &FrancaController, driver: QString, device: QString);
        #[qinvokable]
        fn save_settings_json(self: Pin<&mut FrancaController>, json: QString);
        #[qinvokable]
        fn default_paths_json(self: &FrancaController, cat: QString) -> QString;
        #[qinvokable]
        fn restore_default_settings_json(self: &FrancaController) -> QString;
        #[qinvokable]
        fn browse_folder(self: &FrancaController, title: QString) -> QString;
        #[qinvokable]
        fn get_runtime_driver_json(self: &FrancaController) -> QString;
        #[qinvokable]
        fn get_about_json(self: &FrancaController) -> QString;

        // Logs
        #[qinvokable]
        fn open_log_folder(self: &FrancaController);
        #[qinvokable]
        fn save_logs_dialog(self: &FrancaController);
        #[qinvokable]
        fn clear_logs(self: Pin<&mut FrancaController>);
    }
}

use std::fs;
use std::path::PathBuf;
use std::pin::Pin;

use cxx_qt::CxxQtType;
use cxx_qt_lib::QString;

use crate::controller::FrancaCore;
use crate::ffi::dirs_home;
use crate::ffi::types::*;
use crate::logging;
use crate::settings::FrancaSettings;

fn percent_decode_path(input: &str) -> String {
    let bytes = input.as_bytes();
    let mut out = Vec::with_capacity(bytes.len());
    let mut i = 0;
    while i < bytes.len() {
        if bytes[i] == b'%' && i + 2 < bytes.len() {
            if let (Some(hi), Some(lo)) = (
                (bytes[i + 1] as char).to_digit(16),
                (bytes[i + 2] as char).to_digit(16),
            ) {
                out.push(((hi << 4) | lo) as u8);
                i += 3;
                continue;
            }
        }
        out.push(bytes[i]);
        i += 1;
    }
    String::from_utf8(out).unwrap_or_else(|_| input.to_string())
}

pub struct FrancaControllerRust {
    core: FrancaCore,
    window_title: QString,
    project_name: QString,
    engine_running: bool,
    driver_name: QString,
    plugin_count: i32,
    dsp_load: f32,
    xruns: i32,
    buffer_size: i32,
    sample_rate: i32,
    variable_buffer_size: bool,
    variable_sample_rate: bool,
    buffer_sizes_json: QString,
    sample_rates_json: QString,
    can_undo: bool,
    can_redo: bool,
    undo_label: QString,
    redo_label: QString,
    show_toolbar: bool,
    show_side_panel: bool,
    show_meters: bool,
    show_keyboard: bool,
    show_logs: bool,
    confirm_exit: bool,
    experimental_enabled: bool,
    export_lv2_enabled: bool,
    jack_apps_enabled: bool,
    refresh_interval_ms: i32,
    external_patchbay: bool,
    show_routing_mode_toggle: bool,
    patchbay_use_bezier: bool,
    canvas_width: f32,
    canvas_height: f32,
    selected_patchbay_plugin_id: i32,
    transport_playing: bool,
    transport_bpm: f64,
    transport_frame_str: QString,
    transport_time_str: QString,
    transport_bbt_str: QString,
    transport_jack: bool,
    transport_link: bool,
    rack_json: QString,
    categories_json: QString,
    meters_json: QString,
    patchbay_json: QString,
    files_json: QString,
    logs_text: QString,
    log_dir: QString,
    cancelable_action_text: QString,
    discovery_revision: i32,
    plugin_edit_revision: i32,
}

impl Default for FrancaControllerRust {
    fn default() -> Self {
        let core = FrancaCore::new();
        let cats = QString::from(&FrancaCore::categories_json());
        let rack = QString::from(&core.rack_json());
        let pb = QString::from(&core.patchbay_json());
        let files = QString::from(&core.files_json());
        let logs = QString::from(&logging::ui_lines_snapshot().join("\n"));
        let log_dir = QString::from(&logging::current_log_dir().to_string_lossy().into_owned());
        let meters = QString::from(&core.meters_json());
        let title = QString::from(&core.window_title());
        let driver = QString::from(&core.running_driver_name);
        let buf_sizes = QString::from(
            &serde_json::to_string(&core.effective_buffer_sizes()).unwrap_or_else(|_| "[]".into()),
        );
        let sample_rates = QString::from(
            &serde_json::to_string(&core.effective_sample_rates()).unwrap_or_else(|_| "[]".into()),
        );

        Self {
            window_title: title,
            project_name: QString::from(""),
            engine_running: core.engine_running,
            driver_name: driver,
            plugin_count: core.slots.len() as i32,
            dsp_load: core.runtime_info.load,
            xruns: core.runtime_info.xruns as i32,
            buffer_size: core.buffer_size as i32,
            sample_rate: core.sample_rate as i32,
            variable_buffer_size: (core.runtime_dev_info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE) != 0,
            variable_sample_rate: (core.runtime_dev_info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE) != 0,
            buffer_sizes_json: buf_sizes,
            sample_rates_json: sample_rates,
            can_undo: false,
            can_redo: false,
            undo_label: QString::from("Undo"),
            redo_label: QString::from("Redo"),
            show_toolbar: core.settings.show_toolbar,
            show_side_panel: core.settings.show_side_panel,
            show_meters: core.settings.show_meters,
            show_keyboard: core.settings.show_keyboard,
            show_logs: core.settings.show_logs,
            confirm_exit: core.settings.confirm_exit,
            experimental_enabled: core.settings.experimental,
            export_lv2_enabled: core.settings.experimental && core.settings.exp_export_lv2,
            jack_apps_enabled: core.settings.experimental && core.settings.exp_jack_apps,
            refresh_interval_ms: core.settings.refresh_interval_ms.clamp(10, 100) as i32,
            external_patchbay: core.external_patchbay,
            show_routing_mode_toggle: core.process_mode == ENGINE_PROCESS_MODE_PATCHBAY,
            patchbay_use_bezier: core.patchbay.use_bezier,
            canvas_width: core.patchbay.canvas_width,
            canvas_height: core.patchbay.canvas_height,
            selected_patchbay_plugin_id: -1,
            transport_playing: core.transport_info.playing,
            transport_bpm: if core.transport_info.bpm > 0.0 { core.transport_info.bpm } else { 120.0 },
            transport_frame_str: QString::from("000000000"),
            transport_time_str: QString::from("00:00:00.000"),
            transport_bbt_str: QString::from("001|01|0000"),
            transport_jack: core.transport_mode == ENGINE_TRANSPORT_MODE_JACK,
            transport_link: core.transport_link,
            rack_json: rack,
            categories_json: cats,
            meters_json: meters,
            patchbay_json: pb,
            files_json: files,
            logs_text: logs,
            log_dir,
            cancelable_action_text: QString::from(""),
            discovery_revision: 0,
            plugin_edit_revision: 0,
            core,
        }
    }
}

impl qobject::FrancaController {
    fn sync_properties(mut self: Pin<&mut Self>) {
        // Take pending signals
        let alerts = std::mem::take(&mut self.as_mut().rust_mut().core.pending.alerts);
        let focus_pb = std::mem::take(&mut self.as_mut().rust_mut().core.pending.focus_patchbay);
        let quit_req = std::mem::take(&mut self.as_mut().rust_mut().core.pending.quit_requested);
        let rack_dirty = std::mem::take(&mut self.as_mut().rust_mut().core.pending.rack_dirty);
        let pb_dirty = std::mem::take(&mut self.as_mut().rust_mut().core.pending.patchbay_dirty);
        let edit_dirty = !std::mem::take(&mut self.as_mut().rust_mut().core.pending.plugin_edit_dirty_ids).is_empty();
        let logs_dirty = std::mem::take(&mut self.as_mut().rust_mut().core.pending.logs_dirty);
        let files_dirty = std::mem::take(&mut self.as_mut().rust_mut().core.pending.files_dirty);
        let disc_dirty = std::mem::take(&mut self.as_mut().rust_mut().core.pending.discovery_dirty);

        let title = QString::from(&self.rust().core.window_title());
        if *self.window_title() != title {
            self.as_mut().set_window_title(title);
        }
        let pname = QString::from(
            self.rust()
                .core
                .project_filename
                .as_ref()
                .and_then(|p| p.file_name())
                .and_then(|n| n.to_str())
                .unwrap_or(""),
        );
        if *self.project_name() != pname {
            self.as_mut().set_project_name(pname);
        }

        let running = self.rust().core.engine_running;
        if *self.engine_running() != running {
            self.as_mut().set_engine_running(running);
        }
        let drv = QString::from(&self.rust().core.running_driver_name);
        if *self.driver_name() != drv {
            self.as_mut().set_driver_name(drv);
        }
        let pcount = self.rust().core.slots.len() as i32;
        if *self.plugin_count() != pcount {
            self.as_mut().set_plugin_count(pcount);
        }
        let load = self.rust().core.runtime_info.load.clamp(0.0, 100.0);
        if (*self.dsp_load() - load).abs() > 0.2 {
            self.as_mut().set_dsp_load(load);
        }
        let xruns = self.rust().core.runtime_info.xruns as i32;
        if *self.xruns() != xruns {
            self.as_mut().set_xruns(xruns);
        }
        let buf = self.rust().core.buffer_size as i32;
        if *self.buffer_size() != buf {
            self.as_mut().set_buffer_size(buf);
        }
        let sr = self.rust().core.sample_rate as i32;
        if *self.sample_rate() != sr {
            self.as_mut().set_sample_rate(sr);
        }
        let var_buf = (self.rust().core.runtime_dev_info.hints
            & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE)
            != 0;
        if *self.variable_buffer_size() != var_buf {
            self.as_mut().set_variable_buffer_size(var_buf);
        }
        let var_sr = (self.rust().core.runtime_dev_info.hints
            & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE)
            != 0;
        if *self.variable_sample_rate() != var_sr {
            self.as_mut().set_variable_sample_rate(var_sr);
        }
        let bufs_j = QString::from(
            &serde_json::to_string(&self.rust().core.effective_buffer_sizes())
                .unwrap_or_else(|_| "[]".into()),
        );
        if *self.buffer_sizes_json() != bufs_j {
            self.as_mut().set_buffer_sizes_json(bufs_j);
        }
        let srs_j = QString::from(
            &serde_json::to_string(&self.rust().core.effective_sample_rates())
                .unwrap_or_else(|_| "[]".into()),
        );
        if *self.sample_rates_json() != srs_j {
            self.as_mut().set_sample_rates_json(srs_j);
        }

        // Undo / Redo
        let cu = self.rust().core.history.can_undo();
        if *self.can_undo() != cu {
            self.as_mut().set_can_undo(cu);
        }
        let cr = self.rust().core.history.can_redo();
        if *self.can_redo() != cr {
            self.as_mut().set_can_redo(cr);
        }
        let ul = QString::from(&self.rust().core.history.undo_label());
        if *self.undo_label() != ul {
            self.as_mut().set_undo_label(ul);
        }
        let rl = QString::from(&self.rust().core.history.redo_label());
        if *self.redo_label() != rl {
            self.as_mut().set_redo_label(rl);
        }

        // Layout & Settings flags
        let s = self.rust().core.settings.clone();
        if *self.show_toolbar() != s.show_toolbar {
            self.as_mut().set_show_toolbar(s.show_toolbar);
        }
        if *self.show_side_panel() != s.show_side_panel {
            self.as_mut().set_show_side_panel(s.show_side_panel);
        }
        if *self.show_meters() != s.show_meters {
            self.as_mut().set_show_meters(s.show_meters);
        }
        if *self.show_keyboard() != s.show_keyboard {
            self.as_mut().set_show_keyboard(s.show_keyboard);
        }
        if *self.show_logs() != s.show_logs {
            self.as_mut().set_show_logs(s.show_logs);
        }
        if *self.confirm_exit() != s.confirm_exit {
            self.as_mut().set_confirm_exit(s.confirm_exit);
        }
        if *self.experimental_enabled() != s.experimental {
            self.as_mut().set_experimental_enabled(s.experimental);
        }
        let exp_lv2 = s.experimental && s.exp_export_lv2;
        if *self.export_lv2_enabled() != exp_lv2 {
            self.as_mut().set_export_lv2_enabled(exp_lv2);
        }
        let exp_jack = s.experimental && s.exp_jack_apps;
        if *self.jack_apps_enabled() != exp_jack {
            self.as_mut().set_jack_apps_enabled(exp_jack);
        }
        let rint = s.refresh_interval_ms.clamp(10, 100) as i32;
        if *self.refresh_interval_ms() != rint {
            self.as_mut().set_refresh_interval_ms(rint);
        }

        // Patchbay flags
        let ext_pb = self.rust().core.external_patchbay;
        if *self.external_patchbay() != ext_pb {
            self.as_mut().set_external_patchbay(ext_pb);
        }
        let show_rt = self.rust().core.process_mode == ENGINE_PROCESS_MODE_PATCHBAY;
        if *self.show_routing_mode_toggle() != show_rt {
            self.as_mut().set_show_routing_mode_toggle(show_rt);
        }
        let bez = self.rust().core.patchbay.use_bezier;
        if *self.patchbay_use_bezier() != bez {
            self.as_mut().set_patchbay_use_bezier(bez);
        }
        let cw = self.rust().core.patchbay.canvas_width;
        if (*self.canvas_width() - cw).abs() > 0.5 {
            self.as_mut().set_canvas_width(cw);
        }
        let ch = self.rust().core.patchbay.canvas_height;
        if (*self.canvas_height() - ch).abs() > 0.5 {
            self.as_mut().set_canvas_height(ch);
        }
        let sel_pb_pid = self
            .rust()
            .core
            .selected_patchbay_plugins
            .first()
            .map(|&id| id as i32)
            .unwrap_or(-1);
        if *self.selected_patchbay_plugin_id() != sel_pb_pid {
            self.as_mut().set_selected_patchbay_plugin_id(sel_pb_pid);
        }

        // Transport
        let t = self.rust().core.transport_info;
        if *self.transport_playing() != t.playing {
            self.as_mut().set_transport_playing(t.playing);
        }
        if t.bpm > 0.0 && (*self.transport_bpm() - t.bpm).abs() > 0.01 {
            self.as_mut().set_transport_bpm(t.bpm);
        }
        let sr_f = if self.rust().core.sample_rate > 0.0 {
            self.rust().core.sample_rate
        } else {
            44100.0
        };
        let secs = t.frame as f64 / sr_f;
        let mins = (secs / 60.0).floor() as u64;
        let rem_secs = secs - (mins as f64 * 60.0);
        let hrs = mins / 60;
        let rem_mins = mins % 60;
        let f_str = QString::from(&format!("{:09}", t.frame));
        if *self.transport_frame_str() != f_str {
            self.as_mut().set_transport_frame_str(f_str);
        }
        let t_str = QString::from(&format!("{hrs:02}:{rem_mins:02}:{rem_secs:06.3}"));
        if *self.transport_time_str() != t_str {
            self.as_mut().set_transport_time_str(t_str);
        }
        let bbt_str = QString::from(&format!(
            "{:03}|{:02}|{:04}",
            t.bar.max(1),
            t.beat.max(1),
            t.tick.max(0)
        ));
        if *self.transport_bbt_str() != bbt_str {
            self.as_mut().set_transport_bbt_str(bbt_str);
        }
        let t_jack = self.rust().core.transport_mode == ENGINE_TRANSPORT_MODE_JACK;
        if *self.transport_jack() != t_jack {
            self.as_mut().set_transport_jack(t_jack);
        }
        let t_link = self.rust().core.transport_link;
        if *self.transport_link() != t_link {
            self.as_mut().set_transport_link(t_link);
        }

        // Cancelable action
        let ca = QString::from(&self.rust().core.cancelable_action_msg);
        if *self.cancelable_action_text() != ca {
            self.as_mut().set_cancelable_action_text(ca);
        }

        // Dirty JSON view-models
        if rack_dirty {
            let j = QString::from(&self.rust().core.rack_json());
            self.as_mut().set_rack_json(j);
        }
        if pb_dirty {
            let j = QString::from(&self.rust().core.patchbay_json());
            self.as_mut().set_patchbay_json(j);
        }
        if files_dirty {
            let j = QString::from(&self.rust().core.files_json());
            self.as_mut().set_files_json(j);
        }
        if logs_dirty {
            let txt = QString::from(&logging::ui_lines_snapshot().join("\n"));
            self.as_mut().set_logs_text(txt);
        }
        if disc_dirty {
            let rev = *self.discovery_revision() + 1;
            self.as_mut().set_discovery_revision(rev);
        }
        if edit_dirty {
            let rev = *self.plugin_edit_revision() + 1;
            self.as_mut().set_plugin_edit_revision(rev);
        }

        // Fast meters update (only push when running or non-empty)
        let mj = QString::from(&self.rust().core.meters_json());
        if *self.meters_json() != mj {
            self.as_mut().set_meters_json(mj);
        }

        // Emit signals
        for (t, m) in alerts {
            self.as_mut()
                .alert_requested(QString::from(&t), QString::from(&m));
        }
        if focus_pb {
            self.as_mut().focus_patchbay_requested();
        }
        if quit_req {
            self.as_mut().quit_signal_received();
        }
    }

    pub fn poll_tick(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.poll_tick();
        self.sync_properties();
    }

    pub fn start_engine(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.start_engine(false);
        self.sync_properties();
    }

    pub fn stop_engine(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.stop_engine();
        self.sync_properties();
    }

    pub fn engine_panic(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let Some(ref host) = core.host {
            for pid in 0..core.slots.len() as u32 {
                for ch in 0..16u8 {
                    for note in 0..128u8 {
                        host.send_midi_note(pid, ch, note, 0);
                    }
                }
            }
        }
        core.canvas_active_notes.clear();
        for s in &mut core.slots {
            s.active_notes.clear();
        }
        self.sync_properties();
    }

    pub fn clear_xruns(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let Some(ref host) = core.host {
            host.clear_engine_xruns();
        }
        core.runtime_info.xruns = 0;
        self.sync_properties();
    }

    pub fn apply_buffer_size(mut self: Pin<&mut Self>, size: i32) {
        let core = &mut self.as_mut().rust_mut().core;
        if size > 0 {
            if let Some(ref host) = core.host {
                let _ = host.set_engine_buffer_size_and_sample_rate(size as u32, core.sample_rate);
            }
            core.refresh_runtime_state();
        }
        self.sync_properties();
    }

    pub fn apply_sample_rate(mut self: Pin<&mut Self>, rate: i32) {
        let core = &mut self.as_mut().rust_mut().core;
        if rate > 0 {
            if let Some(ref host) = core.host {
                let _ = host.set_engine_buffer_size_and_sample_rate(core.buffer_size, rate as f64);
            }
            core.refresh_runtime_state();
        }
        self.sync_properties();
    }

    pub fn open_device_control_panel(mut self: Pin<&mut Self>) {
        if let Some(ref host) = self.as_mut().rust_mut().core.host {
            let _ = host.show_engine_device_control_panel();
        }
    }

    pub fn cancel_engine_action(mut self: Pin<&mut Self>) {
        if let Some(ref host) = self.as_mut().rust_mut().core.host {
            host.cancel_engine_action();
        }
    }

    pub fn shutdown_clean(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.shutdown_clean();
        if *self.engine_running() {
            self.as_mut().set_engine_running(false);
        }
    }

    pub fn file_new(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.file_new();
        self.sync_properties();
    }

    pub fn file_open(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.file_open_dialog();
        self.sync_properties();
    }

    pub fn file_save(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.file_save();
        self.sync_properties();
    }

    pub fn file_save_as(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.file_save_as_dialog();
        self.sync_properties();
    }

    pub fn load_file_url(mut self: Pin<&mut Self>, path_or_url: QString) {
        let raw = path_or_url.to_string();
        let stripped = raw.strip_prefix("file://").unwrap_or(&raw);
        let decoded = percent_decode_path(stripped);
        self.as_mut()
            .rust_mut()
            .core
            .load_project_or_file(&PathBuf::from(decoded));
        self.sync_properties();
    }

    pub fn undo(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.perform_undo();
        self.sync_properties();
    }

    pub fn redo(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.perform_redo();
        self.sync_properties();
    }

    pub fn set_plugin_active(mut self: Pin<&mut Self>, plugin_id: i32, active: bool) {
        if plugin_id >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .set_plugin_active(plugin_id as u32, active);
        }
        self.sync_properties();
    }

    pub fn toggle_custom_ui(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if let (Some(host), Some(s)) = (core.host.as_ref(), core.slots.get_mut(plugin_id as usize)) {
                let next = !s.custom_ui_visible;
                host.show_custom_ui(plugin_id as u32, next);
                s.custom_ui_visible = next;
                core.pending.rack_dirty = true;
            }
        }
        self.sync_properties();
    }

    pub fn move_plugin(mut self: Pin<&mut Self>, source: i32, target: i32) {
        if source >= 0 && target >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .move_plugin(source as u32, target as u32);
        }
        self.sync_properties();
    }

    pub fn set_plugin_category(mut self: Pin<&mut Self>, plugin_id: i32, category: i32) {
        if plugin_id >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .set_plugin_category(plugin_id as u32, category);
        }
        self.sync_properties();
    }

    pub fn dismiss_clip_warning(mut self: Pin<&mut Self>, plugin_id: i32) {
        if let Some(s) = self.as_mut().rust_mut().core.slots.get_mut(plugin_id as usize) {
            s.clip_warning.clear();
        }
        self.sync_properties();
    }

    pub fn reset_parameters(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            self.as_mut().rust_mut().core.change_plugin_params_batch(
                plugin_id as u32,
                "Reset Parameters",
                |h, id| h.reset_parameters(id),
            );
        }
        self.sync_properties();
    }

    pub fn randomize_parameters(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            self.as_mut().rust_mut().core.change_plugin_params_batch(
                plugin_id as u32,
                "Randomize Parameters",
                |h, id| h.randomize_parameters(id),
            );
        }
        self.sync_properties();
    }

    pub fn clone_plugin(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if let Some(ref host) = core.host {
                if !host.clone_plugin(plugin_id as u32) {
                    core.pending
                        .alerts
                        .push(("Clone Failed".into(), host.get_last_error()));
                }
            }
            core.drain_engine_events();
        }
        self.sync_properties();
    }

    pub fn rename_plugin(mut self: Pin<&mut Self>, plugin_id: i32, new_name: QString) {
        if plugin_id >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .rename_plugin(plugin_id as u32, &new_name.to_string());
        }
        self.sync_properties();
    }

    pub fn remove_plugin(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            self.as_mut().rust_mut().core.remove_plugin(plugin_id as u32);
        }
        self.sync_properties();
    }

    pub fn remove_all_plugins(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.remove_all_plugins();
        self.sync_properties();
    }

    pub fn export_plugin_lv2(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            if let Some(dir) = rfd::FileDialog::new()
                .set_title("Export Plugin as LV2 Bundle")
                .pick_folder()
            {
                let core = &mut self.as_mut().rust_mut().core;
                if let Some(ref host) = core.host {
                    if !host.export_plugin_lv2(plugin_id as u32, &dir.to_string_lossy()) {
                        core.pending
                            .alerts
                            .push(("LV2 Export Failed".into(), host.get_last_error()));
                    }
                }
            }
        }
        self.sync_properties();
    }

    pub fn find_plugin_in_patchbay(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            let name = core
                .slots
                .get(plugin_id as usize)
                .map(|s| s.info.name.clone())
                .unwrap_or_default();
            if core.external_patchbay {
                core.switch_patchbay_view(false);
            }
            if core.patchbay.focus_plugin(plugin_id as u32, &name) {
                core.pending.patchbay_dirty = true;
                core.pending.focus_patchbay = true;
            }
        }
        self.sync_properties();
    }

    // --- Plugin Macros ---

    pub fn macro_enable_all(mut self: Pin<&mut Self>) {
        let n = self.rust().core.slots.len() as u32;
        for i in 0..n {
            self.as_mut().rust_mut().core.set_plugin_active(i, true);
        }
        self.sync_properties();
    }

    pub fn macro_disable_all(mut self: Pin<&mut Self>) {
        let n = self.rust().core.slots.len() as u32;
        for i in 0..n {
            self.as_mut().rust_mut().core.set_plugin_active(i, false);
        }
        self.sync_properties();
    }

    pub fn macro_volume_100(mut self: Pin<&mut Self>) {
        let n = self.rust().core.slots.len() as u32;
        for i in 0..n {
            if (self.rust().core.slots[i as usize].info.hints & PLUGIN_CAN_VOLUME) != 0 {
                self.as_mut()
                    .rust_mut()
                    .core
                    .set_plugin_internal_param(i, PARAMETER_VOLUME, 1.0);
            }
        }
        self.sync_properties();
    }

    pub fn macro_mute_all(mut self: Pin<&mut Self>) {
        let n = self.rust().core.slots.len() as u32;
        for i in 0..n {
            if (self.rust().core.slots[i as usize].info.hints & PLUGIN_CAN_VOLUME) != 0 {
                self.as_mut()
                    .rust_mut()
                    .core
                    .set_plugin_internal_param(i, PARAMETER_VOLUME, 0.0);
            }
        }
        self.sync_properties();
    }

    pub fn macro_wet_100(mut self: Pin<&mut Self>) {
        let n = self.rust().core.slots.len() as u32;
        for i in 0..n {
            if (self.rust().core.slots[i as usize].info.hints & PLUGIN_CAN_DRYWET) != 0 {
                self.as_mut()
                    .rust_mut()
                    .core
                    .set_plugin_internal_param(i, PARAMETER_DRYWET, 1.0);
            }
        }
        self.sync_properties();
    }

    pub fn macro_bypass_all(mut self: Pin<&mut Self>) {
        let n = self.rust().core.slots.len() as u32;
        for i in 0..n {
            if (self.rust().core.slots[i as usize].info.hints & PLUGIN_CAN_DRYWET) != 0 {
                self.as_mut()
                    .rust_mut()
                    .core
                    .set_plugin_internal_param(i, PARAMETER_DRYWET, 0.0);
            }
        }
        self.sync_properties();
    }

    pub fn macro_center_balance(mut self: Pin<&mut Self>) {
        let n = self.rust().core.slots.len() as u32;
        for i in 0..n {
            let hints = self.rust().core.slots[i as usize].info.hints;
            if (hints & PLUGIN_CAN_BALANCE) != 0 {
                self.as_mut().rust_mut().core.set_plugin_internal_param(
                    i,
                    PARAMETER_BALANCE_LEFT,
                    -1.0,
                );
                self.as_mut().rust_mut().core.set_plugin_internal_param(
                    i,
                    PARAMETER_BALANCE_RIGHT,
                    1.0,
                );
            }
            if (hints & PLUGIN_CAN_PANNING) != 0 {
                self.as_mut()
                    .rust_mut()
                    .core
                    .set_plugin_internal_param(i, PARAMETER_PANNING, 0.0);
            }
        }
        self.sync_properties();
    }

    // --- Plugin Parameter Editor ---

    pub fn get_plugin_edit_json(&self, plugin_id: i32) -> QString {
        if plugin_id < 0 {
            return QString::from("{}");
        }
        QString::from(&self.rust().core.plugin_edit_json(plugin_id as u32))
    }

    pub fn set_internal_param(mut self: Pin<&mut Self>, plugin_id: i32, param_id: i32, value: f32) {
        if plugin_id >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .set_plugin_internal_param(plugin_id as u32, param_id, value);
        }
        self.sync_properties();
    }

    pub fn set_ctrl_channel(mut self: Pin<&mut Self>, plugin_id: i32, channel: i32) {
        if plugin_id >= 0 {
            if let Some(ref host) = self.as_mut().rust_mut().core.host {
                host.set_ctrl_channel(plugin_id as u32, channel as i8);
            }
        }
    }

    pub fn set_plugin_option(
        mut self: Pin<&mut Self>,
        plugin_id: i32,
        option_bit: i32,
        enabled: bool,
    ) {
        if plugin_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if let Some(ref host) = core.host {
                host.set_option(plugin_id as u32, option_bit as u32, enabled);
            }
            if let Some(s) = core.slots.get_mut(plugin_id as usize) {
                if enabled {
                    s.info.options_enabled |= option_bit as u32;
                } else {
                    s.info.options_enabled &= !(option_bit as u32);
                }
            }
            core.pending.plugin_edit_dirty_ids.push(plugin_id as u32);
        }
        self.sync_properties();
    }

    pub fn set_param_value(mut self: Pin<&mut Self>, plugin_id: i32, param_id: i32, value: f32) {
        if plugin_id >= 0 && param_id >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .set_plugin_param(plugin_id as u32, param_id as u32, value);
        }
        self.sync_properties();
    }

    pub fn set_param_midi_channel(
        mut self: Pin<&mut Self>,
        plugin_id: i32,
        param_id: i32,
        channel: i32,
    ) {
        if plugin_id >= 0 && param_id >= 0 {
            if let Some(ref host) = self.as_mut().rust_mut().core.host {
                host.set_parameter_midi_channel(plugin_id as u32, param_id as u32, channel as u8);
            }
        }
    }

    pub fn set_param_mapped_control(
        mut self: Pin<&mut Self>,
        plugin_id: i32,
        param_id: i32,
        cc: i32,
    ) {
        if plugin_id >= 0 && param_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if let Some(ref host) = core.host {
                host.set_parameter_mapped_control_index(
                    plugin_id as u32,
                    param_id as u32,
                    cc as i16,
                );
            }
            core.pending.plugin_edit_dirty_ids.push(plugin_id as u32);
        }
        self.sync_properties();
    }

    pub fn set_param_mapped_range(
        mut self: Pin<&mut Self>,
        plugin_id: i32,
        param_id: i32,
        min_val: f32,
        max_val: f32,
    ) {
        if plugin_id >= 0 && param_id >= 0 {
            if let Some(ref host) = self.as_mut().rust_mut().core.host {
                host.set_parameter_mapped_range(
                    plugin_id as u32,
                    param_id as u32,
                    min_val,
                    max_val,
                );
            }
        }
    }

    pub fn set_program(mut self: Pin<&mut Self>, plugin_id: i32, program_id: i32) {
        if plugin_id >= 0 && program_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if let Some(ref host) = core.host {
                host.set_program(plugin_id as u32, program_id as u32);
            }
            core.pending.plugin_edit_dirty_ids.push(plugin_id as u32);
        }
        self.sync_properties();
    }

    pub fn set_midi_program(mut self: Pin<&mut Self>, plugin_id: i32, midi_program_id: i32) {
        if plugin_id >= 0 && midi_program_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if let Some(ref host) = core.host {
                host.set_midi_program(plugin_id as u32, midi_program_id as u32);
            }
            core.pending.plugin_edit_dirty_ids.push(plugin_id as u32);
        }
        self.sync_properties();
    }

    pub fn load_plugin_state(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id < 0 {
            return;
        }
        if let Some(path) = rfd::FileDialog::new()
            .set_title("Load Plugin State")
            .add_filter("Carla Plugin State", &["carxs"])
            .pick_file()
        {
            let core = &mut self.as_mut().rust_mut().core;
            if let Some(ref host) = core.host {
                if !host.load_plugin_state(plugin_id as u32, &path.to_string_lossy()) {
                    core.pending
                        .alerts
                        .push(("Load State Failed".into(), host.get_last_error()));
                } else {
                    core.pending.plugin_edit_dirty_ids.push(plugin_id as u32);
                }
            }
        }
        self.sync_properties();
    }

    pub fn save_plugin_state(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id < 0 {
            return;
        }
        if let Some(mut path) = rfd::FileDialog::new()
            .set_title("Save Plugin State")
            .add_filter("Carla Plugin State", &["carxs"])
            .set_file_name("preset.carxs")
            .save_file()
        {
            if path.extension().is_none() {
                path.set_extension("carxs");
            }
            let core = &mut self.as_mut().rust_mut().core;
            if let Some(ref host) = core.host {
                if !host.save_plugin_state(plugin_id as u32, &path.to_string_lossy()) {
                    core.pending
                        .alerts
                        .push(("Save State Failed".into(), host.get_last_error()));
                }
            }
        }
        self.sync_properties();
    }

    pub fn reload_plugin_all(mut self: Pin<&mut Self>, plugin_id: i32) {
        if plugin_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if let (Some(host), Some(s)) = (core.host.as_ref(), core.slots.get_mut(plugin_id as usize)) {
                s.reload_info(host);
                core.pending.rack_dirty = true;
                core.pending.plugin_edit_dirty_ids.push(plugin_id as u32);
            }
        }
        self.sync_properties();
    }

    pub fn send_midi_note(
        mut self: Pin<&mut Self>,
        plugin_id: i32,
        channel: i32,
        note: i32,
        velocity: i32,
    ) {
        if plugin_id >= 0 {
            if let Some(ref host) = self.as_mut().rust_mut().core.host {
                host.send_midi_note(
                    plugin_id as u32,
                    channel.clamp(0, 15) as u8,
                    note.clamp(0, 127) as u8,
                    velocity.clamp(0, 127) as u8,
                );
            }
        }
    }

    pub fn send_canvas_midi_note(
        mut self: Pin<&mut Self>,
        channel: i32,
        note: i32,
        velocity: i32,
    ) {
        let core = &self.as_mut().rust_mut().core;
        if let Some(ref host) = core.host {
            for &pid in &core.selected_patchbay_plugins {
                host.send_midi_note(
                    pid,
                    channel.clamp(0, 15) as u8,
                    note.clamp(0, 127) as u8,
                    velocity.clamp(0, 127) as u8,
                );
            }
        }
    }

    // --- Add / Replace Plugin & Discovery ---

    #[allow(clippy::too_many_arguments)]
    pub fn query_plugins_json(
        &self,
        search: QString,
        favorites_only: bool,
        category_filter: i32,
        formats_mask: i32,
        types_mask: i32,
        sort_col: i32,
        sort_asc: bool,
    ) -> QString {
        QString::from(&self.rust().core.query_plugins_json(
            &search.to_string(),
            favorites_only,
            category_filter,
            formats_mask as u32,
            types_mask as u32,
            sort_col,
            sort_asc,
        ))
    }

    pub fn add_plugin_by_key(mut self: Pin<&mut Self>, key: QString, replace_id: i32) {
        let k = key.to_string();
        let found = self
            .rust()
            .core
            .db
            .plugins
            .iter()
            .find(|p| p.unique_key() == k)
            .cloned();
        if let Some(p) = found {
            let rid = if replace_id >= 0 {
                Some(replace_id as u32)
            } else {
                None
            };
            self.as_mut().rust_mut().core.add_discovered_plugin(&p, rid);
        }
        self.sync_properties();
    }

    pub fn toggle_favorite(mut self: Pin<&mut Self>, key: QString) {
        let k = key.to_string();
        let core = &mut self.as_mut().rust_mut().core;
        if !core.settings.favorite_plugins.remove(&k) {
            core.settings.favorite_plugins.insert(k);
        }
        let _ = core.settings.save();
        core.pending.discovery_dirty = true;
        self.sync_properties();
    }

    pub fn start_discovery(mut self: Pin<&mut Self>, formats_mask: i32, check_invalid: bool) {
        self.as_mut()
            .rust_mut()
            .core
            .start_discovery(formats_mask as u32, check_invalid);
        self.sync_properties();
    }

    pub fn skip_discovery(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let (Some(r), Some(u)) = (core.discovery_runner.as_mut(), core.utils.as_ref()) {
            r.skip_current(u);
        }
    }

    pub fn cancel_discovery(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let (Some(r), Some(u)) = (core.discovery_runner.as_mut(), core.utils.as_ref()) {
            r.cancel(u);
        }
        core.discovery_runner = None;
        core.pending.discovery_dirty = true;
        self.sync_properties();
    }

    #[allow(clippy::too_many_arguments)]
    pub fn add_jack_app(
        mut self: Pin<&mut Self>,
        name: QString,
        command: QString,
        audio_ins: i32,
        audio_outs: i32,
        midi_ins: i32,
        midi_outs: i32,
        session_mgr: i32,
        manage_window: bool,
    ) {
        let cmd = command.to_string();
        let nm = name.to_string();
        let display = if nm.trim().is_empty() {
            cmd.trim().to_string()
        } else {
            nm.trim().to_string()
        };
        let label_setup = format!(
            "{}:{}:{}:{}:{}:{}",
            audio_ins.max(0),
            audio_outs.max(0),
            midi_ins.max(0),
            midi_outs.max(0),
            session_mgr.clamp(0, 2),
            u32::from(manage_window)
        );
        let core = &mut self.as_mut().rust_mut().core;
        if let Some(ref host) = core.host {
            if !host.add_plugin(
                BINARY_NATIVE,
                PLUGIN_JACK,
                cmd.trim(),
                Some(&display),
                &label_setup,
                0,
                PLUGIN_OPTIONS_NULL,
            ) {
                core.pending
                    .alerts
                    .push(("Failed to Add JACK App".into(), host.get_last_error()));
            }
        }
        core.drain_engine_events();
        self.sync_properties();
    }

    // --- Patchbay ---

    pub fn switch_external_patchbay(mut self: Pin<&mut Self>, external: bool) {
        self.as_mut().rust_mut().core.switch_patchbay_view(external);
        self.sync_properties();
    }

    pub fn refresh_patchbay(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.refresh_patchbay();
        self.sync_properties();
    }

    pub fn arrange_patchbay(mut self: Pin<&mut Self>) {
        self.as_mut().rust_mut().core.arrange_patchbay();
        self.sync_properties();
    }

    pub fn patchbay_connect(
        mut self: Pin<&mut Self>,
        g_out: i32,
        p_out: i32,
        g_in: i32,
        p_in: i32,
    ) {
        if g_out >= 0 && p_out >= 0 && g_in >= 0 && p_in >= 0 {
            self.as_mut().rust_mut().core.connect_patchbay_ports(
                g_out as u32,
                p_out as u32,
                g_in as u32,
                p_in as u32,
            );
        }
        self.sync_properties();
    }

    pub fn patchbay_disconnect(mut self: Pin<&mut Self>, connection_id: i32) {
        if connection_id >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .disconnect_patchbay_ports(connection_id as u32);
        }
        self.sync_properties();
    }

    pub fn patchbay_disconnect_port(
        mut self: Pin<&mut Self>,
        group_id: i32,
        port_id: i32,
        is_input: bool,
    ) {
        if group_id < 0 || port_id < 0 {
            return;
        }
        let gid = group_id as u32;
        let pid = port_id as u32;
        let cids: Vec<u32> = self
            .rust()
            .core
            .patchbay
            .connections
            .iter()
            .filter(|c| {
                if is_input {
                    c.group_in == gid && c.port_in == pid
                } else {
                    c.group_out == gid && c.port_out == pid
                }
            })
            .map(|c| c.connection_id)
            .collect();
        for cid in cids {
            self.as_mut().rust_mut().core.disconnect_patchbay_ports(cid);
        }
        self.sync_properties();
    }

    pub fn patchbay_disconnect_group(mut self: Pin<&mut Self>, group_id: i32) {
        if group_id < 0 {
            return;
        }
        let gid = group_id as u32;
        let cids: Vec<u32> = self
            .rust()
            .core
            .patchbay
            .connections
            .iter()
            .filter(|c| c.group_out == gid || c.group_in == gid)
            .map(|c| c.connection_id)
            .collect();
        for cid in cids {
            self.as_mut().rust_mut().core.disconnect_patchbay_ports(cid);
        }
        self.sync_properties();
    }

    pub fn patchbay_move_box(
        mut self: Pin<&mut Self>,
        group_id: i32,
        split_part: i32,
        x: f32,
        y: f32,
    ) {
        if group_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            core.patchbay
                .move_box(group_id as u32, split_part.max(0) as u8, x, y);
            core.pending.patchbay_dirty = true;
        }
        self.sync_properties();
    }

    pub fn patchbay_commit_group_pos(mut self: Pin<&mut Self>, group_id: i32) {
        if group_id >= 0 {
            self.as_mut()
                .rust_mut()
                .core
                .commit_group_position(group_id as u32);
        }
    }

    pub fn patchbay_split_group(mut self: Pin<&mut Self>, group_id: i32) {
        if group_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if core.patchbay.split_group(group_id as u32) {
                core.commit_group_position(group_id as u32);
                core.pending.patchbay_dirty = true;
            }
        }
        self.sync_properties();
    }

    pub fn patchbay_join_group(mut self: Pin<&mut Self>, group_id: i32) {
        if group_id >= 0 {
            let core = &mut self.as_mut().rust_mut().core;
            if core.patchbay.join_group(group_id as u32) {
                core.commit_group_position(group_id as u32);
                core.pending.patchbay_dirty = true;
            }
        }
        self.sync_properties();
    }

    pub fn patchbay_select_boxes(mut self: Pin<&mut Self>, csv: QString) {
        self.as_mut()
            .rust_mut()
            .core
            .apply_selected_boxes_csv(&csv.to_string());
        self.sync_properties();
    }

    pub fn patchbay_cut_line(mut self: Pin<&mut Self>, x1: f32, y1: f32, x2: f32, y2: f32) {
        let hits = self
            .rust()
            .core
            .patchbay
            .connections_intersecting_segment(x1, y1, x2, y2);
        for cid in hits {
            self.as_mut().rust_mut().core.disconnect_patchbay_ports(cid);
        }
        self.sync_properties();
    }

    pub fn save_patchbay_image(mut self: Pin<&mut Self>, scale: f32) {
        if let Some(mut path) = rfd::FileDialog::new()
            .set_title("Save Patchbay Image")
            .add_filter("PNG Image", &["png"])
            .set_file_name("patchbay.png")
            .save_file()
        {
            if path.extension().is_none() {
                path.set_extension("png");
            }
            let core = &mut self.as_mut().rust_mut().core;
            if let Err(e) = core.patchbay.save_png(&path, scale) {
                core.pending
                    .alerts
                    .push(("Save Image Failed".into(), format!("Could not save PNG:\n{e}")));
            } else {
                logging::log_ui(&format!("Saved patchbay image to {}", path.display()));
            }
        }
        self.sync_properties();
    }

    pub fn copy_patchbay_image(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        let (w, h, rgba) = core.patchbay.render_to_rgba(2.0);
        match arboard::Clipboard::new() {
            Ok(mut cb) => {
                let img = arboard::ImageData {
                    width: w as usize,
                    height: h as usize,
                    bytes: std::borrow::Cow::Owned(rgba),
                };
                if let Err(e) = cb.set_image(img) {
                    core.pending
                        .alerts
                        .push(("Clipboard Error".into(), format!("Could not copy image:\n{e}")));
                } else {
                    logging::log_ui("Copied patchbay image to clipboard");
                }
            }
            Err(e) => {
                core.pending
                    .alerts
                    .push(("Clipboard Error".into(), format!("Clipboard unavailable:\n{e}")));
            }
        }
        self.sync_properties();
    }

    // --- Side Panel: Files & Transport ---

    pub fn files_select_folder_index(mut self: Pin<&mut Self>, idx: i32) {
        let core = &mut self.as_mut().rust_mut().core;
        let idx = idx.max(0) as usize;
        core.selected_folder_idx = idx;
        core.current_dir = if idx == 0 {
            dirs_home()
        } else {
            core.settings
                .disk_folders
                .get(idx - 1)
                .map(PathBuf::from)
                .unwrap_or_else(dirs_home)
        };
        core.pending.files_dirty = true;
        self.sync_properties();
    }

    pub fn files_navigate(mut self: Pin<&mut Self>, path: QString) {
        let core = &mut self.as_mut().rust_mut().core;
        core.current_dir = PathBuf::from(path.to_string());
        core.pending.files_dirty = true;
        self.sync_properties();
    }

    pub fn files_go_up(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let Some(parent) = core.current_dir.parent().map(|p| p.to_path_buf()) {
            core.current_dir = parent;
            core.pending.files_dirty = true;
        }
        self.sync_properties();
    }

    pub fn files_add_folder(mut self: Pin<&mut Self>) {
        if let Some(dir) = rfd::FileDialog::new().set_title("Add Folder").pick_folder() {
            let s = dir.to_string_lossy().into_owned();
            let core = &mut self.as_mut().rust_mut().core;
            if !core.settings.disk_folders.contains(&s) {
                core.settings.disk_folders.push(s);
                let _ = core.settings.save();
                core.pending.files_dirty = true;
            }
        }
        self.sync_properties();
    }

    pub fn files_remove_folder(mut self: Pin<&mut Self>, idx: i32) {
        let core = &mut self.as_mut().rust_mut().core;
        if idx > 0 {
            let i = (idx - 1) as usize;
            if i < core.settings.disk_folders.len() {
                core.settings.disk_folders.remove(i);
                let _ = core.settings.save();
                core.selected_folder_idx = 0;
                core.current_dir = dirs_home();
                core.pending.files_dirty = true;
            }
        }
        self.sync_properties();
    }

    pub fn transport_play_pause(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let Some(ref host) = core.host {
            if core.transport_info.playing {
                host.transport_pause();
            } else {
                host.transport_play();
            }
        }
        self.sync_properties();
    }

    pub fn transport_stop(mut self: Pin<&mut Self>) {
        if let Some(ref host) = self.as_mut().rust_mut().core.host {
            host.transport_pause();
            host.transport_relocate(0);
        }
        self.sync_properties();
    }

    pub fn transport_rewind(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let Some(ref host) = core.host {
            let step = (core.sample_rate.max(44100.0) * 2.0) as u64;
            host.transport_relocate(core.transport_info.frame.saturating_sub(step));
        }
    }

    pub fn transport_forward(mut self: Pin<&mut Self>) {
        let core = &mut self.as_mut().rust_mut().core;
        if let Some(ref host) = core.host {
            let step = (core.sample_rate.max(44100.0) * 2.0) as u64;
            host.transport_relocate(core.transport_info.frame.saturating_add(step));
        }
    }

    pub fn transport_set_bpm(mut self: Pin<&mut Self>, bpm: f64) {
        if let Some(ref host) = self.as_mut().rust_mut().core.host {
            host.transport_bpm(bpm.clamp(20.0, 300.0));
        }
    }

    pub fn transport_set_jack(mut self: Pin<&mut Self>, enabled: bool) {
        let core = &mut self.as_mut().rust_mut().core;
        let mode = if enabled {
            ENGINE_TRANSPORT_MODE_JACK
        } else {
            ENGINE_TRANSPORT_MODE_INTERNAL
        };
        core.transport_mode = mode;
        core.settings.transport_mode = mode;
        if let Some(ref host) = core.host {
            host.set_engine_option(
                ENGINE_OPTION_TRANSPORT_MODE,
                mode,
                &core.settings.transport_extra,
            );
        }
        self.sync_properties();
    }

    pub fn transport_set_link(mut self: Pin<&mut Self>, enabled: bool) {
        let core = &mut self.as_mut().rust_mut().core;
        core.transport_link = enabled;
        core.settings.transport_extra = if enabled {
            ":link:".into()
        } else {
            String::new()
        };
        if let Some(ref host) = core.host {
            host.set_engine_option(
                ENGINE_OPTION_TRANSPORT_MODE,
                core.transport_mode,
                &core.settings.transport_extra,
            );
        }
        self.sync_properties();
    }

    // --- Layout & Settings ---

    pub fn set_layout_toggle(mut self: Pin<&mut Self>, key: QString, value: bool) {
        let core = &mut self.as_mut().rust_mut().core;
        match key.to_string().as_str() {
            "show_toolbar" => core.settings.show_toolbar = value,
            "show_side_panel" => core.settings.show_side_panel = value,
            "show_meters" => core.settings.show_meters = value,
            "show_keyboard" => core.settings.show_keyboard = value,
            _ => {}
        }
        let _ = core.settings.save();
        self.sync_properties();
    }

    pub fn load_settings_draft_json(&self) -> QString {
        QString::from(&self.rust().core.settings_draft_json())
    }

    pub fn query_driver_devices_json(&self, driver: QString, device: QString) -> QString {
        QString::from(
            &self
                .rust()
                .core
                .query_driver_devices_json(&driver.to_string(), &device.to_string()),
        )
    }

    pub fn open_driver_control_panel(&self, driver: QString, device: QString) {
        let Some(ref host) = self.rust().core.host else {
            return;
        };
        let drv = driver.to_string();
        let dev = device.to_string();
        let count = host.get_engine_driver_count();
        for i in 0..count {
            if host.get_engine_driver_name(i) == drv {
                let _ = host.show_engine_driver_device_control_panel(i, &dev);
                break;
            }
        }
    }

    pub fn save_settings_json(mut self: Pin<&mut Self>, json: QString) {
        let raw = json.to_string();
        match serde_json::from_str::<FrancaSettings>(&raw) {
            Ok(new_settings) => {
                let core = &mut self.as_mut().rust_mut().core;
                core.settings = new_settings;
                let _ = core.settings.save();
                core.patchbay.canvas_width = core.settings.canvas_width as f32;
                core.patchbay.canvas_height = core.settings.canvas_height as f32;
                core.patchbay.use_bezier = core.settings.canvas_use_bezier;
                core.patchbay.auto_hide_groups = core.settings.canvas_auto_hide_groups;
                if let Some(ref host) = core.host {
                    core.settings.apply_to_host(host, &core.bin_dir, &core.res_dir);
                }
                if core.engine_running {
                    core.refresh_patchbay();
                } else {
                    core.pending.patchbay_dirty = true;
                }
                logging::log_ui("Settings updated");
            }
            Err(e) => {
                self.as_mut().rust_mut().core.pending.alerts.push((
                    "Settings Error".into(),
                    format!("Invalid settings JSON: {e}"),
                ));
            }
        }
        self.sync_properties();
    }

    pub fn default_paths_json(&self, cat: QString) -> QString {
        let list = FrancaCore::default_paths_for_category(&cat.to_string());
        QString::from(&serde_json::to_string(&list).unwrap_or_else(|_| "[]".into()))
    }

    pub fn restore_default_settings_json(&self) -> QString {
        QString::from(
            &serde_json::to_string(&FrancaSettings::default()).unwrap_or_else(|_| "{}".into()),
        )
    }

    pub fn browse_folder(&self, title: QString) -> QString {
        let picked = rfd::FileDialog::new()
            .set_title(&title.to_string())
            .pick_folder()
            .map(|p| p.to_string_lossy().into_owned())
            .unwrap_or_default();
        QString::from(&picked)
    }

    pub fn get_runtime_driver_json(&self) -> QString {
        QString::from(&self.rust().core.runtime_driver_json())
    }

    pub fn get_about_json(&self) -> QString {
        QString::from(&self.rust().core.about_json())
    }

    // --- Logs ---

    pub fn open_log_folder(&self) {
        let dir = logging::current_log_dir();
        let _ = fs::create_dir_all(&dir);
        let _ = open::that(&dir);
    }

    pub fn save_logs_dialog(&self) {
        if let Some(path) = rfd::FileDialog::new()
            .set_title("Save Session Log")
            .set_file_name("franca-session.log")
            .save_file()
        {
            let content = logging::ui_lines_snapshot().join("\n");
            let _ = fs::write(path, content);
        }
    }

    pub fn clear_logs(mut self: Pin<&mut Self>) {
        logging::clear_ui_logs();
        self.as_mut().rust_mut().core.pending.logs_dirty = true;
        self.sync_properties();
    }
}
