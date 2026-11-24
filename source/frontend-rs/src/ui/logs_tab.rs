// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::fs;

use eframe::egui;

use crate::logging;

pub struct LogsTabState {
    pub auto_scroll: bool,
    pub last_line_count: usize,
}

impl Default for LogsTabState {
    fn default() -> Self {
        Self {
            auto_scroll: true,
            last_line_count: 0,
        }
    }
}

pub fn show_logs_tab(ui: &mut egui::Ui, state: &mut LogsTabState) {
    let lines = logging::ui_lines_snapshot();
    let log_dir = logging::current_log_dir();

    ui.horizontal(|ui| {
        if ui
            .button("📁 Open Folder")
            .on_hover_text(log_dir.to_string_lossy())
            .clicked()
        {
            let _ = fs::create_dir_all(&log_dir);
            let _ = open::that(&log_dir);
        }
        if ui.button("💾 Save...").clicked() {
            if let Some(path) = rfd::FileDialog::new()
                .set_title("Save Session Log")
                .set_file_name("franca-session.log")
                .save_file()
            {
                let content = lines.join("\n");
                let _ = fs::write(path, content);
            }
        }
        if ui.button("🗑 Clear").clicked() {
            logging::clear_ui_logs();
        }
        ui.separator();
        ui.checkbox(&mut state.auto_scroll, "Auto-Scroll");
        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
            ui.label(
                egui::RichText::new(format!("{} lines", lines.len()))
                    .small()
                    .weak(),
            );
        });
    });

    ui.separator();

    let stick = state.auto_scroll && lines.len() != state.last_line_count;
    state.last_line_count = lines.len();

    egui::ScrollArea::vertical()
        .id_salt("franca_logs_scroll")
        .stick_to_bottom(state.auto_scroll)
        .auto_shrink([false, false])
        .show_rows(ui, 16.0, lines.len(), |ui, row_range| {
            for idx in row_range {
                ui.label(egui::RichText::new(&lines[idx]).monospace().size(12.0));
            }
            if stick {
                ui.scroll_to_cursor(Some(egui::Align::BOTTOM));
            }
        });
}
