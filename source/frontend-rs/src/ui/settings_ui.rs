// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use eframe::egui;

use crate::ffi::host::{CarlaHost, DriverDeviceInfo, RuntimeDriverDeviceInfo};
use crate::ffi::types::*;
use crate::settings::{
    FrancaSettings, default_clap_paths, default_dssi_paths, default_jsfx_paths,
    default_ladspa_paths, default_lv2_paths, default_sf2_paths, default_sfz_paths,
    default_vst2_paths, default_vst3_paths,
};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
pub enum SettingsSection {
    #[default]
    Main,
    Engine,
    Canvas,
    Paths,
    Osc,
    Wine,
    Experimental,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
pub enum PathCategory {
    Audio,
    Midi,
    Ladspa,
    Dssi,
    #[default]
    Lv2,
    Vst2,
    Vst3,
    Clap,
    Sf2,
    Sfz,
    Jsfx,
}

impl PathCategory {
    pub const ALL: &'static [PathCategory] = &[
        PathCategory::Audio,
        PathCategory::Midi,
        PathCategory::Ladspa,
        PathCategory::Dssi,
        PathCategory::Lv2,
        PathCategory::Vst2,
        PathCategory::Vst3,
        PathCategory::Clap,
        PathCategory::Sf2,
        PathCategory::Sfz,
        PathCategory::Jsfx,
    ];

    pub fn label(self) -> &'static str {
        match self {
            PathCategory::Audio => "Audio Files",
            PathCategory::Midi => "MIDI Files",
            PathCategory::Ladspa => "LADSPA",
            PathCategory::Dssi => "DSSI",
            PathCategory::Lv2 => "LV2",
            PathCategory::Vst2 => "VST2",
            PathCategory::Vst3 => "VST3",
            PathCategory::Clap => "CLAP",
            PathCategory::Sf2 => "SF2",
            PathCategory::Sfz => "SFZ",
            PathCategory::Jsfx => "JSFX",
        }
    }
}

pub struct SettingsDialogState {
    pub open: bool,
    pub section: SettingsSection,
    pub draft: FrancaSettings,
    pub path_cat: PathCategory,
    pub selected_path_idx: Option<usize>,
    pub driver_names: Vec<String>,
    pub cached_driver_idx: Option<u32>,
    pub cached_devices: Vec<String>,
    pub cached_device_info: DriverDeviceInfo,
}

impl Default for SettingsDialogState {
    fn default() -> Self {
        Self {
            open: false,
            section: SettingsSection::Main,
            draft: FrancaSettings::default(),
            path_cat: PathCategory::Lv2,
            selected_path_idx: None,
            driver_names: Vec::new(),
            cached_driver_idx: None,
            cached_devices: Vec::new(),
            cached_device_info: DriverDeviceInfo::default(),
        }
    }
}

impl SettingsDialogState {
    pub fn open_with(&mut self, current: &FrancaSettings, host: &CarlaHost) {
        self.draft = current.clone();
        self.open = true;
        self.driver_names.clear();
        let count = host.get_engine_driver_count();
        for i in 0..count {
            self.driver_names.push(host.get_engine_driver_name(i));
        }
        self.refresh_driver_devices(host);
    }

    fn refresh_driver_devices(&mut self, host: &CarlaHost) {
        let idx = self
            .driver_names
            .iter()
            .position(|d| d == &self.draft.audio_driver)
            .map(|i| i as u32);
        self.cached_driver_idx = idx;
        if let Some(i) = idx {
            self.cached_devices = host.get_engine_driver_device_names(i);
            let dev_name = self.draft.driver_config(&self.draft.audio_driver).device;
            let query_dev = if dev_name.is_empty() {
                self.cached_devices.first().cloned().unwrap_or_default()
            } else {
                dev_name
            };
            self.cached_device_info = host.get_engine_driver_device_info(i, &query_dev);
        } else {
            self.cached_devices.clear();
            self.cached_device_info = DriverDeviceInfo::default();
        }
    }
}

/// Returns `Some(new_settings)` when the user clicks Save/Apply.
pub fn show_settings_dialog(
    ctx: &egui::Context,
    state: &mut SettingsDialogState,
    host: &CarlaHost,
) -> Option<FrancaSettings> {
    if !state.open {
        return None;
    }
    let mut saved: Option<FrancaSettings> = None;
    let mut open = state.open;
    let mut close_now = false;

    egui::Window::new("Configure Franca")
        .id(egui::Id::unique("franca_settings_dialog"))
        .open(&mut open)
        .default_size([740.0, 500.0])
        .resizable(true)
        .show(ctx, |ui| {
            ui.horizontal(|ui| {
                // Left navigation sidebar
                ui.vertical(|ui| {
                    ui.set_width(135.0);
                    ui.selectable_value(&mut state.section, SettingsSection::Main, "Main");
                    ui.selectable_value(&mut state.section, SettingsSection::Engine, "Engine");
                    ui.selectable_value(&mut state.section, SettingsSection::Canvas, "Canvas");
                    ui.selectable_value(&mut state.section, SettingsSection::Paths, "Paths");
                    ui.selectable_value(&mut state.section, SettingsSection::Osc, "OSC");
                    if state.draft.experimental {
                        ui.selectable_value(&mut state.section, SettingsSection::Wine, "Wine");
                        ui.selectable_value(
                            &mut state.section,
                            SettingsSection::Experimental,
                            "Experimental",
                        );
                    }
                });

                ui.separator();

                // Right settings panel
                ui.vertical(|ui| {
                    egui::ScrollArea::vertical()
                        .max_height(410.0)
                        .auto_shrink([false, false])
                        .show(ui, |ui| match state.section {
                            SettingsSection::Main => show_main_section(ui, &mut state.draft),
                            SettingsSection::Engine => show_engine_section(ui, state, host),
                            SettingsSection::Canvas => show_canvas_section(ui, &mut state.draft),
                            SettingsSection::Paths => show_paths_section(ui, state),
                            SettingsSection::Osc => show_osc_section(ui, &mut state.draft, host),
                            SettingsSection::Wine => show_wine_section(ui, &mut state.draft),
                            SettingsSection::Experimental => {
                                show_experimental_section(ui, &mut state.draft)
                            }
                        });
                });
            });

            ui.separator();
            ui.horizontal(|ui| {
                if ui.button("Restore Defaults").clicked() {
                    state.draft = FrancaSettings::default();
                    state.refresh_driver_devices(host);
                }
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    if ui.button("OK").clicked() {
                        saved = Some(state.draft.clone());
                        close_now = true;
                    }
                    if ui.button("Cancel").clicked() {
                        close_now = true;
                    }
                });
            });
        });

    state.open = open && !close_now;
    saved
}

fn show_main_section(ui: &mut egui::Ui, d: &mut FrancaSettings) {
    ui.heading("Main Settings");
    ui.add_space(6.0);
    ui.horizontal(|ui| {
        ui.label("Project Folder:");
        ui.text_edit_singleline(&mut d.project_folder);
        if ui.button("Browse...").clicked() {
            if let Some(dir) = rfd::FileDialog::new()
                .set_title("Select Project Folder")
                .pick_folder()
            {
                d.project_folder = dir.to_string_lossy().into_owned();
            }
        }
    });
    ui.horizontal(|ui| {
        ui.label("UI Refresh Interval (ms):");
        ui.add(egui::DragValue::new(&mut d.refresh_interval_ms).range(10..=200));
    });
    ui.checkbox(&mut d.confirm_exit, "Confirm before closing when plugins are loaded");
    ui.checkbox(&mut d.show_logs, "Show Logs tab");
    ui.checkbox(&mut d.dark_theme, "Use dark theme");
    ui.checkbox(&mut d.experimental, "Enable experimental features (Wine, JACK Apps, Bridges, LV2 Export)");
}

fn show_engine_section(ui: &mut egui::Ui, state: &mut SettingsDialogState, host: &CarlaHost) {
    ui.heading("Audio Engine");
    ui.add_space(6.0);

    let prev_driver = state.draft.audio_driver.clone();
    ui.horizontal(|ui| {
        ui.label("Audio Driver:");
        egui::ComboBox::from_id_salt("settings_audio_driver")
            .selected_text(&state.draft.audio_driver)
            .show_ui(ui, |ui| {
                for name in &state.driver_names {
                    ui.selectable_value(&mut state.draft.audio_driver, name.clone(), name);
                }
            });
    });
    if state.draft.audio_driver != prev_driver {
        state.refresh_driver_devices(host);
    }

    // Per-driver device settings
    let drv_name = state.draft.audio_driver.clone();
    let mut drv_cfg = state.draft.driver_config(&drv_name);
    let prev_dev = drv_cfg.device.clone();

    ui.group(|ui| {
        ui.label(egui::RichText::new(format!("{drv_name} Device Settings")).strong());
        if !state.cached_devices.is_empty() {
            ui.horizontal(|ui| {
                ui.label("Device:");
                let sel = if drv_cfg.device.is_empty() {
                    "(Default)".to_string()
                } else {
                    drv_cfg.device.clone()
                };
                egui::ComboBox::from_id_salt("settings_drv_device")
                    .selected_text(sel)
                    .show_ui(ui, |ui| {
                        ui.selectable_value(&mut drv_cfg.device, String::new(), "(Default)");
                        for dev in &state.cached_devices {
                            ui.selectable_value(&mut drv_cfg.device, dev.clone(), dev);
                        }
                    });
            });
        }
        ui.horizontal(|ui| {
            ui.label("Buffer Size:");
            let sizes: Vec<u32> = if state.cached_device_info.buffer_sizes.is_empty() {
                vec![64, 128, 256, 512, 1024, 2048]
            } else {
                state.cached_device_info.buffer_sizes.clone()
            };
            egui::ComboBox::from_id_salt("settings_buf_size")
                .selected_text(format!("{}", drv_cfg.buffer_size))
                .show_ui(ui, |ui| {
                    for s in sizes {
                        ui.selectable_value(&mut drv_cfg.buffer_size, s, format!("{s}"));
                    }
                });

            ui.label("Sample Rate:");
            let rates: Vec<u32> = if state.cached_device_info.sample_rates.is_empty() {
                vec![44100, 48000, 88200, 96000]
            } else {
                state
                    .cached_device_info
                    .sample_rates
                    .iter()
                    .map(|r| *r as u32)
                    .collect()
            };
            egui::ComboBox::from_id_salt("settings_sample_rate")
                .selected_text(format!("{} Hz", drv_cfg.sample_rate))
                .show_ui(ui, |ui| {
                    for r in rates {
                        ui.selectable_value(&mut drv_cfg.sample_rate, r, format!("{r} Hz"));
                    }
                });
        });

        if (state.cached_device_info.hints & ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER) != 0 {
            ui.checkbox(&mut drv_cfg.triple_buffer, "Triple buffering");
        }
        if (state.cached_device_info.hints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL) != 0 {
            if let Some(didx) = state.cached_driver_idx {
                if ui.button("Open Device Control Panel").clicked() {
                    let _ = host.show_engine_driver_device_control_panel(didx, &drv_cfg.device);
                }
            }
        }
    });

    let dev_changed = drv_cfg.device != prev_dev;
    *state.draft.driver_config_mut(&drv_name) = drv_cfg;
    if dev_changed {
        state.refresh_driver_devices(host);
    }

    ui.add_space(6.0);
    ui.horizontal(|ui| {
        ui.label("Process Mode:");
        egui::ComboBox::from_id_salt("settings_proc_mode")
            .selected_text(process_mode_label(state.draft.process_mode))
            .show_ui(ui, |ui| {
                for m in [
                    ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
                    ENGINE_PROCESS_MODE_PATCHBAY,
                    ENGINE_PROCESS_MODE_SINGLE_CLIENT,
                    ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS,
                ] {
                    ui.selectable_value(&mut state.draft.process_mode, m, process_mode_label(m));
                }
            });
    });

    ui.horizontal(|ui| {
        ui.label("Transport Mode:");
        let tlabel = |t: i32| match t {
            ENGINE_TRANSPORT_MODE_DISABLED => "Disabled",
            ENGINE_TRANSPORT_MODE_INTERNAL => "Internal",
            ENGINE_TRANSPORT_MODE_JACK => "JACK",
            _ => "Internal",
        };
        egui::ComboBox::from_id_salt("settings_trans_mode")
            .selected_text(tlabel(state.draft.transport_mode))
            .show_ui(ui, |ui| {
                for t in [
                    ENGINE_TRANSPORT_MODE_INTERNAL,
                    ENGINE_TRANSPORT_MODE_JACK,
                    ENGINE_TRANSPORT_MODE_DISABLED,
                ] {
                    ui.selectable_value(&mut state.draft.transport_mode, t, tlabel(t));
                }
            });
    });

    ui.checkbox(&mut state.draft.force_stereo, "Force stereo plugins");
    ui.checkbox(&mut state.draft.prefer_ui_bridges, "Prefer UI bridges");
    ui.checkbox(&mut state.draft.manage_uis, "Manage plugin UIs");
    ui.checkbox(&mut state.draft.uis_always_on_top, "Plugin UIs always on top");
    ui.checkbox(&mut state.draft.reset_xruns, "Reset xruns on transport start");
    ui.horizontal(|ui| {
        ui.label("Max parameters per plugin:");
        ui.add(egui::DragValue::new(&mut state.draft.max_parameters).range(16..=4096));
    });
    ui.horizontal(|ui| {
        ui.label("UI bridges timeout (ms):");
        ui.add(egui::DragValue::new(&mut state.draft.ui_bridges_timeout).range(500..=30000));
    });
}

fn show_canvas_section(ui: &mut egui::Ui, d: &mut FrancaSettings) {
    ui.heading("Patchbay Canvas");
    ui.add_space(6.0);
    ui.horizontal(|ui| {
        ui.label("Canvas Size:");
        ui.add(egui::DragValue::new(&mut d.canvas_width).range(1000..=10000));
        ui.label("×");
        ui.add(egui::DragValue::new(&mut d.canvas_height).range(800..=10000));
    });
    ui.checkbox(&mut d.canvas_use_bezier, "Use curved Bezier cables");
    ui.checkbox(&mut d.canvas_auto_hide_groups, "Auto-hide empty groups");
    ui.checkbox(&mut d.canvas_auto_select_items, "Auto-select items");
    ui.checkbox(&mut d.canvas_eye_candy, "Eye-candy rendering");
    ui.checkbox(&mut d.canvas_inline_displays, "Show inline displays");
}

fn show_paths_section(ui: &mut egui::Ui, state: &mut SettingsDialogState) {
    ui.heading("Plugin & File Paths");
    ui.add_space(6.0);

    ui.horizontal(|ui| {
        ui.label("Category:");
        egui::ComboBox::from_id_salt("settings_path_cat")
            .selected_text(state.path_cat.label())
            .show_ui(ui, |ui| {
                for &cat in PathCategory::ALL {
                    if ui
                        .selectable_value(&mut state.path_cat, cat, cat.label())
                        .clicked()
                    {
                        state.selected_path_idx = None;
                    }
                }
            });
    });

    let list: &mut Vec<String> = match state.path_cat {
        PathCategory::Audio => &mut state.draft.paths_audio,
        PathCategory::Midi => &mut state.draft.paths_midi,
        PathCategory::Ladspa => &mut state.draft.paths_ladspa,
        PathCategory::Dssi => &mut state.draft.paths_dssi,
        PathCategory::Lv2 => &mut state.draft.paths_lv2,
        PathCategory::Vst2 => &mut state.draft.paths_vst2,
        PathCategory::Vst3 => &mut state.draft.paths_vst3,
        PathCategory::Clap => &mut state.draft.paths_clap,
        PathCategory::Sf2 => &mut state.draft.paths_sf2,
        PathCategory::Sfz => &mut state.draft.paths_sfz,
        PathCategory::Jsfx => &mut state.draft.paths_jsfx,
    };

    ui.group(|ui| {
        egui::ScrollArea::vertical()
            .id_salt("settings_path_list")
            .max_height(240.0)
            .show(ui, |ui| {
                if list.is_empty() {
                    ui.label(egui::RichText::new("(No folders configured)").weak());
                }
                for (i, p) in list.iter().enumerate() {
                    let sel = state.selected_path_idx == Some(i);
                    if ui.selectable_label(sel, p).clicked() {
                        state.selected_path_idx = Some(i);
                    }
                }
            });
    });

    ui.horizontal(|ui| {
        if ui.button("Add Folder...").clicked() {
            if let Some(dir) = rfd::FileDialog::new()
                .set_title("Add Search Folder")
                .pick_folder()
            {
                let s = dir.to_string_lossy().into_owned();
                if !list.contains(&s) {
                    list.push(s);
                }
            }
        }
        if ui
            .add_enabled(state.selected_path_idx.is_some(), egui::Button::new("Remove"))
            .clicked()
        {
            if let Some(idx) = state.selected_path_idx.take() {
                if idx < list.len() {
                    list.remove(idx);
                }
            }
        }
        if ui.button("Reset to Defaults").clicked() {
            *list = match state.path_cat {
                PathCategory::Audio | PathCategory::Midi => Vec::new(),
                PathCategory::Ladspa => default_ladspa_paths(),
                PathCategory::Dssi => default_dssi_paths(),
                PathCategory::Lv2 => default_lv2_paths(),
                PathCategory::Vst2 => default_vst2_paths(),
                PathCategory::Vst3 => default_vst3_paths(),
                PathCategory::Clap => default_clap_paths(),
                PathCategory::Sf2 => default_sf2_paths(),
                PathCategory::Sfz => default_sfz_paths(),
                PathCategory::Jsfx => default_jsfx_paths(),
            };
            state.selected_path_idx = None;
        }
    });
}

fn show_osc_section(ui: &mut egui::Ui, d: &mut FrancaSettings, host: &CarlaHost) {
    ui.heading("OSC (Open Sound Control)");
    ui.add_space(6.0);
    ui.checkbox(&mut d.osc_enabled, "Enable OSC server");
    ui.add_enabled_ui(d.osc_enabled, |ui| {
        ui.horizontal(|ui| {
            ui.checkbox(&mut d.osc_tcp_enabled, "TCP Port:");
            ui.checkbox(&mut d.osc_tcp_random, "Random");
            ui.add_enabled(
                d.osc_tcp_enabled && !d.osc_tcp_random,
                egui::DragValue::new(&mut d.osc_tcp_port).range(1024..=65535),
            );
        });
        ui.horizontal(|ui| {
            ui.checkbox(&mut d.osc_udp_enabled, "UDP Port:");
            ui.checkbox(&mut d.osc_udp_random, "Random");
            ui.add_enabled(
                d.osc_udp_enabled && !d.osc_udp_random,
                egui::DragValue::new(&mut d.osc_udp_port).range(1024..=65535),
            );
        });
    });

    if host.is_engine_running() {
        let tcp = host.get_host_osc_url_tcp();
        let udp = host.get_host_osc_url_udp();
        if !tcp.is_empty() || !udp.is_empty() {
            ui.add_space(8.0);
            ui.label(egui::RichText::new("Active OSC URLs:").strong());
            if !tcp.is_empty() {
                ui.label(format!("TCP: {tcp}"));
            }
            if !udp.is_empty() {
                ui.label(format!("UDP: {udp}"));
            }
        }
    }
}

fn show_wine_section(ui: &mut egui::Ui, d: &mut FrancaSettings) {
    ui.heading("Wine Bridge Settings");
    ui.add_space(6.0);
    ui.horizontal(|ui| {
        ui.label("Wine Executable:");
        ui.text_edit_singleline(&mut d.wine_executable);
    });
    ui.checkbox(&mut d.wine_auto_prefix, "Auto-detect Wine prefix");
    ui.horizontal(|ui| {
        ui.label("Fallback Prefix:");
        ui.text_edit_singleline(&mut d.wine_fallback_prefix);
    });
    ui.checkbox(&mut d.wine_rt_prio_enabled, "Enable Wine RT priority");
    ui.add_enabled_ui(d.wine_rt_prio_enabled, |ui| {
        ui.horizontal(|ui| {
            ui.label("Base RT Priority:");
            ui.add(egui::DragValue::new(&mut d.wine_base_rt_prio).range(1..=99));
        });
        ui.horizontal(|ui| {
            ui.label("Server RT Priority:");
            ui.add(egui::DragValue::new(&mut d.wine_server_rt_prio).range(1..=99));
        });
    });
}

fn show_experimental_section(ui: &mut egui::Ui, d: &mut FrancaSettings) {
    ui.heading("Experimental Features");
    ui.add_space(6.0);
    ui.checkbox(&mut d.exp_plugin_bridges, "Enable plugin bridges");
    ui.add_enabled(
        d.exp_plugin_bridges,
        egui::Checkbox::new(&mut d.prefer_plugin_bridges, "Prefer plugin bridges"),
    );
    ui.checkbox(&mut d.exp_wine_bridges, "Enable Wine plugin bridges");
    ui.checkbox(&mut d.exp_jack_apps, "Enable JACK applications");
    ui.checkbox(&mut d.exp_export_lv2, "Enable LV2 plugin export");
    ui.checkbox(
        &mut d.exp_prevent_bad_behaviour,
        "Prevent bad plugin behaviour",
    );
    ui.checkbox(&mut d.exp_load_lib_global, "Load plugin libraries globally");
}

#[derive(Default)]
pub struct RuntimeDriverDialogState {
    pub open: bool,
    pub info: RuntimeDriverDeviceInfo,
    pub buffer_size: u32,
    pub sample_rate: f64,
}

impl RuntimeDriverDialogState {
    pub fn open_from_host(&mut self, host: &CarlaHost) {
        self.info = host.get_runtime_engine_driver_device_info();
        self.buffer_size = self.info.buffer_size;
        self.sample_rate = self.info.sample_rate;
        self.open = true;
    }
}

pub fn show_runtime_driver_dialog(
    ctx: &egui::Context,
    state: &mut RuntimeDriverDialogState,
    host: &CarlaHost,
) -> Option<(u32, f64)> {
    if !state.open {
        return None;
    }
    let mut apply: Option<(u32, f64)> = None;
    let mut open = state.open;
    let mut close_now = false;

    egui::Window::new("Runtime Audio Driver Settings")
        .id(egui::Id::unique("franca_runtime_driver_dialog"))
        .open(&mut open)
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            ui.label(format!("Device: {}", state.info.name));
            ui.separator();

            let var_buf = (state.info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE) != 0;
            let var_sr = (state.info.hints & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE) != 0;

            ui.horizontal(|ui| {
                ui.label("Buffer Size:");
                ui.add_enabled_ui(var_buf && !state.info.buffer_sizes.is_empty(), |ui| {
                    egui::ComboBox::from_id_salt("rt_buf_cb")
                        .selected_text(format!("{}", state.buffer_size))
                        .show_ui(ui, |ui| {
                            for &b in &state.info.buffer_sizes {
                                ui.selectable_value(&mut state.buffer_size, b, format!("{b}"));
                            }
                        });
                });
            });

            ui.horizontal(|ui| {
                ui.label("Sample Rate:");
                ui.add_enabled_ui(var_sr && !state.info.sample_rates.is_empty(), |ui| {
                    egui::ComboBox::from_id_salt("rt_sr_cb")
                        .selected_text(format!("{} Hz", state.sample_rate as u32))
                        .show_ui(ui, |ui| {
                            for &sr in &state.info.sample_rates {
                                ui.selectable_value(
                                    &mut state.sample_rate,
                                    sr,
                                    format!("{} Hz", sr as u32),
                                );
                            }
                        });
                });
            });

            if (state.info.hints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL) != 0 {
                if ui.button("Open Device Control Panel").clicked() {
                    let _ = host.show_engine_device_control_panel();
                }
            }

            ui.separator();
            ui.horizontal(|ui| {
                if ui.button("Apply").clicked() {
                    apply = Some((state.buffer_size, state.sample_rate));
                    close_now = true;
                }
                if ui.button("Close").clicked() {
                    close_now = true;
                }
            });
        });

    state.open = open && !close_now;
    apply
}
