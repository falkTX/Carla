// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use eframe::egui;

use crate::ffi::types::CARLA_VERSION_STRING;

#[derive(Default)]
pub struct RenameDialogState {
    pub open: bool,
    pub plugin_id: u32,
    pub name: String,
    pub focus_requested: bool,
}

impl RenameDialogState {
    pub fn open_for(&mut self, plugin_id: u32, current_name: &str) {
        self.open = true;
        self.plugin_id = plugin_id;
        self.name = current_name.to_string();
        self.focus_requested = true;
    }
}

pub fn show_rename_dialog(
    ctx: &egui::Context,
    state: &mut RenameDialogState,
) -> Option<(u32, String)> {
    if !state.open {
        return None;
    }
    let mut result = None;
    let mut open = state.open;
    let mut close_now = false;

    egui::Window::new("Rename Plugin")
        .id(egui::Id::unique("franca_rename_dialog"))
        .open(&mut open)
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            ui.label("New plugin name:");
            let resp = ui.text_edit_singleline(&mut state.name);
            if state.focus_requested {
                resp.request_focus();
                state.focus_requested = false;
            }
            let enter = resp.lost_focus() && ui.input(|i| i.key_pressed(egui::Key::Enter));
            ui.add_space(6.0);
            ui.horizontal(|ui| {
                let valid = !state.name.trim().is_empty();
                if ui.add_enabled(valid, egui::Button::new("Rename")).clicked() || (enter && valid) {
                    result = Some((state.plugin_id, state.name.trim().to_string()));
                    close_now = true;
                }
                if ui.button("Cancel").clicked() {
                    close_now = true;
                }
            });
        });

    state.open = open && !close_now;
    result
}

#[derive(Clone, Debug)]
pub struct JackAppSpec {
    pub command: String,
    pub name: String,
    pub label_setup: String,
}

pub struct AddJackAppDialogState {
    pub open: bool,
    pub command: String,
    pub name: String,
    pub session_mgr: usize, // 0=None, 1=LADISH, 2=NSM
    pub audio_ins: u32,
    pub audio_outs: u32,
    pub midi_ins: u32,
    pub midi_outs: u32,
    pub manage_window: bool,
}

impl Default for AddJackAppDialogState {
    fn default() -> Self {
        Self {
            open: false,
            command: String::new(),
            name: String::new(),
            session_mgr: 0,
            audio_ins: 2,
            audio_outs: 2,
            midi_ins: 1,
            midi_outs: 1,
            manage_window: false,
        }
    }
}

pub fn show_add_jack_app_dialog(
    ctx: &egui::Context,
    state: &mut AddJackAppDialogState,
) -> Option<JackAppSpec> {
    if !state.open {
        return None;
    }
    let mut result = None;
    let mut open = state.open;
    let mut close_now = false;

    egui::Window::new("Add JACK Application")
        .id(egui::Id::unique("franca_add_jack_app"))
        .open(&mut open)
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            egui::Grid::new("jack_app_grid")
                .num_columns(2)
                .spacing([10.0, 6.0])
                .show(ui, |ui| {
                    ui.label("Command:");
                    ui.text_edit_singleline(&mut state.command);
                    ui.end_row();
                    ui.label("Name:");
                    ui.text_edit_singleline(&mut state.name);
                    ui.end_row();
                    ui.label("Session Manager:");
                    let sm_labels = ["None", "LADISH (SIGUSR1)", "NSM"];
                    egui::ComboBox::from_id_salt("jack_app_sm")
                        .selected_text(sm_labels[state.session_mgr])
                        .show_ui(ui, |ui| {
                            for (i, lbl) in sm_labels.iter().enumerate() {
                                ui.selectable_value(&mut state.session_mgr, i, *lbl);
                            }
                        });
                    ui.end_row();
                    ui.label("Audio Ins / Outs:");
                    ui.horizontal(|ui| {
                        ui.add(egui::DragValue::new(&mut state.audio_ins).range(0..=32));
                        ui.label("/");
                        ui.add(egui::DragValue::new(&mut state.audio_outs).range(0..=32));
                    });
                    ui.end_row();
                    ui.label("MIDI Ins / Outs:");
                    ui.horizontal(|ui| {
                        ui.add(egui::DragValue::new(&mut state.midi_ins).range(0..=16));
                        ui.label("/");
                        ui.add(egui::DragValue::new(&mut state.midi_outs).range(0..=16));
                    });
                    ui.end_row();
                });

            ui.checkbox(&mut state.manage_window, "Manage application window");
            ui.separator();
            ui.horizontal(|ui| {
                let valid = !state.command.trim().is_empty();
                if ui.add_enabled(valid, egui::Button::new("Add")).clicked() {
                    let sm = match state.session_mgr {
                        1 => 1,
                        2 => 2,
                        _ => 0,
                    };
                    let flags = u32::from(state.manage_window);
                    let label_setup = format!(
                        "{}:{}:{}:{}:{}:{}",
                        state.audio_ins,
                        state.audio_outs,
                        state.midi_ins,
                        state.midi_outs,
                        sm,
                        flags
                    );
                    let name = if state.name.trim().is_empty() {
                        state.command.trim().to_string()
                    } else {
                        state.name.trim().to_string()
                    };
                    result = Some(JackAppSpec {
                        command: state.command.trim().to_string(),
                        name,
                        label_setup,
                    });
                    close_now = true;
                }
                if ui.button("Cancel").clicked() {
                    close_now = true;
                }
            });
        });

    state.open = open && !close_now;
    result
}

#[derive(Default)]
pub struct AboutDialogState {
    pub open: bool,
    pub features: Vec<String>,
    pub license_text: String,
    pub show_license: bool,
}

pub fn show_about_dialog(ctx: &egui::Context, state: &mut AboutDialogState) {
    if !state.open {
        return;
    }
    let mut open = state.open;
    egui::Window::new("About Franca")
        .id(egui::Id::unique("franca_about_dialog"))
        .open(&mut open)
        .default_size([500.0, 380.0])
        .resizable(true)
        .show(ctx, |ui| {
            ui.heading("Franca");
            ui.label(format!("Audio Plugin Host (Carla {CARLA_VERSION_STRING} backend)"));
            ui.label("Native Rust GUI powered by egui/eframe");
            ui.add_space(6.0);
            ui.horizontal(|ui| {
                ui.selectable_value(&mut state.show_license, false, "Info & Features");
                ui.selectable_value(&mut state.show_license, true, "License");
            });
            ui.separator();
            egui::ScrollArea::vertical().show(ui, |ui| {
                if !state.show_license {
                    ui.label(egui::RichText::new("Supported Backend Features:").strong());
                    ui.label(state.features.join(", "));
                } else {
                    ui.label(egui::RichText::new(&state.license_text).monospace().size(11.0));
                }
            });
        });
    state.open = open;
}

#[derive(Clone, Debug)]
pub enum ConfirmKind {
    RemoveAllPlugins,
    QuitWithPlugins,
}

#[derive(Default)]
pub struct AlertState {
    pub title: String,
    pub message: String,
    pub open: bool,
}

impl AlertState {
    pub fn show(&mut self, title: impl Into<String>, message: impl Into<String>) {
        self.title = title.into();
        self.message = message.into();
        self.open = true;
    }
}

pub fn show_alert_modal(ctx: &egui::Context, alert: &mut AlertState) {
    if !alert.open {
        return;
    }
    let mut open = alert.open;
    let mut close_now = false;
    egui::Window::new(&alert.title)
        .id(egui::Id::unique("franca_alert_modal"))
        .open(&mut open)
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            ui.label(&alert.message);
            ui.add_space(8.0);
            if ui.button("OK").clicked() {
                close_now = true;
            }
        });
    alert.open = open && !close_now;
}

pub fn show_confirm_modal(
    ctx: &egui::Context,
    pending: &mut Option<ConfirmKind>,
) -> Option<ConfirmKind> {
    let kind = pending.clone()?;
    let (title, body, yes_label) = match kind {
        ConfirmKind::RemoveAllPlugins => (
            "Remove All Plugins",
            "Are you sure you want to remove all plugins from the rack?",
            "Remove All",
        ),
        ConfirmKind::QuitWithPlugins => (
            "Quit Franca",
            "Plugins are currently loaded. Are you sure you want to exit?",
            "Quit",
        ),
    };
    let mut accepted = None;
    let mut close = false;
    egui::Window::new(title)
        .id(egui::Id::unique("franca_confirm_modal"))
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            ui.label(body);
            ui.add_space(8.0);
            ui.horizontal(|ui| {
                if ui.button(yes_label).clicked() {
                    accepted = Some(kind.clone());
                    close = true;
                }
                if ui.button("Cancel").clicked() {
                    close = true;
                }
            });
        });
    if close {
        *pending = None;
    }
    accepted
}

pub fn show_cancelable_action_modal(
    ctx: &egui::Context,
    text: Option<&str>,
) -> bool {
    let Some(msg) = text else {
        return false;
    };
    let mut cancel_clicked = false;
    egui::Window::new("Working...")
        .id(egui::Id::unique("franca_cancelable_modal"))
        .collapsible(false)
        .resizable(false)
        .show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.spinner();
                ui.label(msg);
            });
            ui.add_space(6.0);
            if ui.button("Cancel").clicked() {
                cancel_clicked = true;
            }
        });
    cancel_clicked
}
