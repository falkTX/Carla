// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::fs;
use std::path::{Path, PathBuf};

use eframe::egui;

use crate::ffi::dirs_home;
use crate::ffi::types::{CarlaTransportInfo, ENGINE_TRANSPORT_MODE_JACK};
use crate::patchbay::PatchbayState;
use crate::patchbay::widget::show_minimap;
use crate::project::is_supported_file;

pub struct SidePanelState {
    pub selected_folder_idx: usize,
    pub current_dir: PathBuf,
    pub dir_entries: Vec<(PathBuf, bool)>, // (path, is_dir)
    pub dirty_dir: bool,
    pub bpm_edit: f64,
    pub link_enabled: bool,
}

impl Default for SidePanelState {
    fn default() -> Self {
        Self {
            selected_folder_idx: 0,
            current_dir: dirs_home(),
            dir_entries: Vec::new(),
            dirty_dir: true,
            bpm_edit: 120.0,
            link_enabled: false,
        }
    }
}

#[derive(Clone, Debug)]
pub enum SidePanelAction {
    LoadFile(PathBuf),
    AddDiskFolder(String),
    RemoveDiskFolder(usize),
    TransportPlayPause,
    TransportStop,
    TransportRewind,
    TransportForward,
    TransportSetBpm(f64),
    TransportToggleJack(bool),
    TransportToggleLink(bool),
}

impl SidePanelState {
    pub fn refresh_dir(&mut self, supported_exts: &[String]) {
        self.dir_entries.clear();
        if let Ok(read) = fs::read_dir(&self.current_dir) {
            let mut dirs = Vec::new();
            let mut files = Vec::new();
            for entry in read.flatten() {
                let path = entry.path();
                if path
                    .file_name()
                    .and_then(|n| n.to_str())
                    .is_some_and(|n| n.starts_with('.'))
                {
                    continue;
                }
                if path.is_dir() {
                    dirs.push((path, true));
                } else if is_supported_file(&path, supported_exts) {
                    files.push((path, false));
                }
            }
            dirs.sort_by(|a, b| a.0.cmp(&b.0));
            files.sort_by(|a, b| a.0.cmp(&b.0));
            self.dir_entries.extend(dirs);
            self.dir_entries.extend(files);
        }
        self.dirty_dir = false;
    }
}

#[allow(clippy::too_many_arguments)]
pub fn show_side_panel(
    ui: &mut egui::Ui,
    state: &mut SidePanelState,
    disk_folders: &[String],
    supported_exts: &[String],
    engine_running: bool,
    transport: &CarlaTransportInfo,
    sample_rate: f64,
    transport_mode: i32,
    patchbay: &mut PatchbayState,
) -> Vec<SidePanelAction> {
    let mut actions = Vec::new();

    if state.dirty_dir {
        state.refresh_dir(supported_exts);
    }

    // 1. Files / Disk Browser
    ui.heading("Files");
    ui.horizontal(|ui| {
        let mut choices: Vec<String> = vec!["Home".into()];
        choices.extend(disk_folders.iter().cloned());
        if state.selected_folder_idx >= choices.len() {
            state.selected_folder_idx = 0;
        }
        let prev_idx = state.selected_folder_idx;
        egui::ComboBox::from_id_salt("side_disk_combo")
            .selected_text(folder_display_name(&choices[state.selected_folder_idx]))
            .width(135.0)
            .show_ui(ui, |ui| {
                for (i, ch) in choices.iter().enumerate() {
                    ui.selectable_value(
                        &mut state.selected_folder_idx,
                        i,
                        folder_display_name(ch),
                    );
                }
            });
        if state.selected_folder_idx != prev_idx {
            state.current_dir = if state.selected_folder_idx == 0 {
                dirs_home()
            } else {
                PathBuf::from(&choices[state.selected_folder_idx])
            };
            state.dirty_dir = true;
        }
        if ui.small_button("＋").on_hover_text("Add folder").clicked() {
            if let Some(dir) = rfd::FileDialog::new().set_title("Add Folder").pick_folder() {
                actions.push(SidePanelAction::AddDiskFolder(
                    dir.to_string_lossy().into_owned(),
                ));
            }
        }
        if ui
            .add_enabled(state.selected_folder_idx > 0, egui::Button::new("−").small())
            .on_hover_text("Remove folder")
            .clicked()
        {
            actions.push(SidePanelAction::RemoveDiskFolder(
                state.selected_folder_idx - 1,
            ));
            state.selected_folder_idx = 0;
            state.current_dir = dirs_home();
            state.dirty_dir = true;
        }
    });

    // Up button + current path
    ui.horizontal(|ui| {
        if let Some(parent) = state.current_dir.parent().map(Path::to_path_buf) {
            if ui.small_button("⬆").on_hover_text("Parent folder").clicked() {
                state.current_dir = parent;
                state.dirty_dir = true;
            }
        }
        ui.add(
            egui::Label::new(
                egui::RichText::new(state.current_dir.to_string_lossy())
                    .small()
                    .weak(),
            )
            .truncate(),
        );
    });

    egui::ScrollArea::vertical()
        .id_salt("side_files_scroll")
        .max_height(190.0)
        .auto_shrink([false, false])
        .show(ui, |ui| {
            let entries = state.dir_entries.clone();
            for (path, is_dir) in entries {
                let name = path
                    .file_name()
                    .map(|n| n.to_string_lossy().into_owned())
                    .unwrap_or_default();
                let prefix = if is_dir { "📁 " } else { "🎵 " };
                let resp = ui.selectable_label(false, format!("{prefix}{name}"));
                if resp.double_clicked() || (is_dir && resp.clicked()) {
                    if is_dir {
                        state.current_dir = path;
                        state.dirty_dir = true;
                    } else if engine_running {
                        actions.push(SidePanelAction::LoadFile(path));
                    }
                }
            }
        });

    ui.separator();

    // 2. Transport
    ui.heading("Transport");
    ui.add_enabled_ui(engine_running, |ui| {
        ui.horizontal(|ui| {
            if ui
                .button(if transport.playing { "⏸" } else { "▶" })
                .on_hover_text("Play / Pause")
                .clicked()
            {
                actions.push(SidePanelAction::TransportPlayPause);
            }
            if ui.button("⏹").on_hover_text("Stop").clicked() {
                actions.push(SidePanelAction::TransportStop);
            }
            if ui.button("⏪").on_hover_text("Rewind").clicked() {
                actions.push(SidePanelAction::TransportRewind);
            }
            if ui.button("⏩").on_hover_text("Fast Forward").clicked() {
                actions.push(SidePanelAction::TransportForward);
            }
        });

        ui.horizontal(|ui| {
            ui.label("BPM:");
            if transport.bpm > 0.0 && (state.bpm_edit - transport.bpm).abs() > 0.01 {
                state.bpm_edit = transport.bpm;
            }
            if ui
                .add(
                    egui::DragValue::new(&mut state.bpm_edit)
                        .speed(0.5)
                        .range(20.0..=300.0)
                        .fixed_decimals(2),
                )
                .changed()
            {
                actions.push(SidePanelAction::TransportSetBpm(state.bpm_edit));
            }
        });

        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let secs = transport.frame as f64 / sr;
        let mins = (secs / 60.0).floor() as u64;
        let rem_secs = secs - (mins as f64 * 60.0);
        let hrs = mins / 60;
        let rem_mins = mins % 60;

        egui::Grid::new("transport_readout_grid")
            .num_columns(2)
            .spacing([8.0, 2.0])
            .show(ui, |ui| {
                ui.label(egui::RichText::new("Frame:").small().weak());
                ui.label(egui::RichText::new(format!("{:09}", transport.frame)).monospace().small());
                ui.end_row();
                ui.label(egui::RichText::new("Time:").small().weak());
                ui.label(
                    egui::RichText::new(format!("{hrs:02}:{rem_mins:02}:{rem_secs:06.3}"))
                        .monospace()
                        .small(),
                );
                ui.end_row();
                ui.label(egui::RichText::new("BBT:").small().weak());
                ui.label(
                    egui::RichText::new(format!(
                        "{:03}|{:02}|{:04}",
                        transport.bar.max(1),
                        transport.beat.max(1),
                        transport.tick.max(0)
                    ))
                    .monospace()
                    .small(),
                );
                ui.end_row();
            });

        let mut jack_t = transport_mode == ENGINE_TRANSPORT_MODE_JACK;
        if ui.checkbox(&mut jack_t, "JACK Transport").changed() {
            actions.push(SidePanelAction::TransportToggleJack(jack_t));
        }
        let mut link_t = state.link_enabled;
        if ui.checkbox(&mut link_t, "Ableton Link").changed() {
            state.link_enabled = link_t;
            actions.push(SidePanelAction::TransportToggleLink(link_t));
        }
    });

    ui.separator();

    // 3. Patchbay Minimap
    ui.heading("Minimap");
    show_minimap(ui, patchbay);

    actions
}

fn folder_display_name(s: &str) -> String {
    if s == "Home" {
        return "Home".into();
    }
    Path::new(s)
        .file_name()
        .map(|n| n.to_string_lossy().into_owned())
        .unwrap_or_else(|| s.to_string())
}
