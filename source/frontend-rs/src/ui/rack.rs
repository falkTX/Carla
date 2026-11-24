// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use eframe::egui::{self, Color32, CornerRadius, Pos2, Rect, Stroke, Vec2};

use crate::ffi::host::{CarlaHost, PluginInfo};
use crate::ffi::types::*;
use crate::ui::plugin_edit::PluginEditState;
use crate::widgets::category::{
    FRANCA_CATEGORY_PROPERTY, FRANCA_PLUGIN_CATEGORIES, category_color, category_key,
    draw_category_accent_bg, draw_category_swatch, parse_category_key,
};
use crate::widgets::meters::{ClipWarningState, MeterColor, PeakMeterState};

pub const NAME_COLUMN_WIDTH: f32 = 220.0;

pub struct RackPluginSlot {
    pub plugin_id: u32,
    pub info: PluginInfo,
    pub active: bool,
    pub category: i32,
    pub custom_ui_visible: bool,
    pub custom_ui_failed: bool,
    pub unavailable_error: Option<String>,
    pub edit_open: bool,
    pub in_channels: usize,
    pub out_channels: usize,
    pub peak_in: PeakMeterState,
    pub peak_out: PeakMeterState,
    pub clip_warning: ClipWarningState,
    pub edit_state: PluginEditState,
}

impl RackPluginSlot {
    pub fn from_host(host: &CarlaHost, plugin_id: u32) -> Self {
        let info = host.get_plugin_info(plugin_id);
        let active = host.get_internal_parameter_value(plugin_id, PARAMETER_ACTIVE) >= 0.5;
        let saved_cat_str =
            host.get_custom_data_value(plugin_id, CUSTOM_DATA_TYPE_PROPERTY, FRANCA_CATEGORY_PROPERTY);
        let category = parse_category_key(&saved_cat_str).unwrap_or(info.category);

        let audio = host.get_audio_port_count_info(plugin_id);
        let in_ch = (audio.ins as usize).min(2);
        let out_ch = (audio.outs as usize).min(2);

        let mut slot = Self {
            plugin_id,
            info,
            active,
            category,
            custom_ui_visible: false,
            custom_ui_failed: false,
            unavailable_error: None,
            edit_open: false,
            in_channels: in_ch,
            out_channels: out_ch,
            peak_in: PeakMeterState::new(in_ch, MeterColor::Green),
            peak_out: PeakMeterState::new(out_ch, MeterColor::Blue),
            clip_warning: ClipWarningState::new(),
            edit_state: PluginEditState::default(),
        };
        slot.edit_state.reload_all(host, plugin_id);
        slot
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

#[derive(Clone, Debug)]
pub enum RackAction {
    Select(u32),
    SetActive(u32, bool),
    ToggleCustomUi(u32, bool),
    ToggleEdit(u32),
    MovePlugin(u32, u32),
    SetCategory(u32, i32),
    ResetParameters(u32),
    RandomizeParameters(u32),
    ClonePlugin(u32),
    RenamePlugin(u32),
    ReplacePlugin(u32),
    RemovePlugin(u32),
    ExportLv2(u32),
    FindInPatchbay(u32),
    AddPlugin,
}

#[derive(Default)]
pub struct RackDragState {
    pub dragging_from: Option<usize>,
    pub drop_target: Option<usize>,
}

pub fn show_rack_view(
    ui: &mut egui::Ui,
    slots: &mut [RackPluginSlot],
    selected_plugin: Option<u32>,
    drag_state: &mut RackDragState,
    now_secs: f64,
    allow_export_lv2: bool,
    engine_running: bool,
) -> Vec<RackAction> {
    let mut actions = Vec::new();

    if slots.is_empty() {
        ui.centered_and_justified(|ui| {
            ui.vertical_centered(|ui| {
                ui.add_space(60.0);
                ui.label(
                    egui::RichText::new("No plugins loaded in the rack")
                        .size(16.0)
                        .weak(),
                );
                ui.add_space(8.0);
                if ui
                    .add_enabled(engine_running, egui::Button::new("＋ Add Plugin..."))
                    .clicked()
                {
                    actions.push(RackAction::AddPlugin);
                }
            });
        });
        return actions;
    }

    let mut row_rects: Vec<Rect> = Vec::with_capacity(slots.len());
    let total_plugins = slots.len();

    egui::ScrollArea::vertical()
        .id_salt("franca_rack_scroll")
        .auto_shrink([false, false])
        .show(ui, |ui| {
            ui.spacing_mut().item_spacing.y = 2.0;

            for idx in 0..total_plugins {
                let slot = &mut slots[idx];
                let pid = slot.plugin_id;
                let selected = selected_plugin == Some(pid);
                let cat_color = category_color(slot.category);
                let unavailable = slot.unavailable_error.is_some();

                let row_h = 38.0;
                let avail_w = ui.available_width().max(520.0);
                let (row_rect, row_resp) =
                    ui.allocate_exact_size(Vec2::new(avail_w, row_h), egui::Sense::click());
                row_rects.push(row_rect);

                // Row background (alternating + selection + category accent bar & tint)
                let painter = ui.painter();
                let base_bg = if selected {
                    Color32::from_rgba_unmultiplied(59, 130, 246, 75)
                } else if idx % 2 == 1 {
                    Color32::from_rgba_unmultiplied(255, 255, 255, 7)
                } else {
                    Color32::TRANSPARENT
                };
                painter.rect_filled(row_rect, CornerRadius::same(2), base_bg);
                draw_category_accent_bg(painter, row_rect, cat_color, selected);
                if selected {
                    painter.rect_stroke(
                        row_rect,
                        CornerRadius::same(2),
                        Stroke::new(1.0, Color32::from_rgb(59, 130, 246)),
                        egui::StrokeKind::Inside,
                    );
                }

                if row_resp.clicked() {
                    actions.push(RackAction::Select(pid));
                }

                // Layout contents inside row_rect
                let content_rect = Rect::from_min_max(
                    Pos2::new(row_rect.min.x + 10.0, row_rect.min.y + 3.0),
                    Pos2::new(row_rect.max.x - 8.0, row_rect.max.y - 3.0),
                );
                let mut child_ui = ui.new_child(
                    egui::UiBuilder::new()
                        .max_rect(content_rect)
                        .layout(egui::Layout::left_to_right(egui::Align::Center)),
                );
                child_ui.spacing_mut().item_spacing.x = 6.0;

                // 1. Drag handle
                let grip_resp = child_ui
                    .add(
                        egui::Label::new(egui::RichText::new("⠿").size(15.0).weak())
                            .sense(egui::Sense::drag()),
                    )
                    .on_hover_cursor(egui::CursorIcon::Grab)
                    .on_hover_text("Drag to reorder");
                if grip_resp.drag_started() {
                    drag_state.dragging_from = Some(idx);
                }

                // 2. Active / Bypass checkbox
                let mut active = slot.active;
                let cb = child_ui
                    .add_enabled(!unavailable, egui::Checkbox::without_text(&mut active))
                    .on_hover_text(if slot.active {
                        "Bypass effect"
                    } else {
                        "Enable effect"
                    });
                if cb.changed() {
                    actions.push(RackAction::SetActive(pid, active));
                }

                // 3. Fixed-width elided plugin name column (220px)
                let (name_rect, name_resp) = child_ui.allocate_exact_size(
                    Vec2::new(NAME_COLUMN_WIDTH, 24.0),
                    egui::Sense::click(),
                );
                if name_resp.clicked() {
                    actions.push(RackAction::Select(pid));
                }
                if name_resp.double_clicked() && !unavailable {
                    actions.push(RackAction::ToggleEdit(pid));
                }
                name_resp.on_hover_text(&slot.info.name);
                let mut name_text = egui::RichText::new(elide_text(&slot.info.name, 29));
                if selected {
                    name_text = name_text.strong();
                }
                if !slot.active {
                    name_text = name_text.weak();
                }
                let mut name_ui = child_ui.new_child(
                    egui::UiBuilder::new()
                        .max_rect(name_rect)
                        .layout(egui::Layout::left_to_right(egui::Align::Center)),
                );
                name_ui.add(egui::Label::new(name_text).truncate());

                // 4. Aligned button column right next to plugin name: [Editor] [Parameters] [⋯]
                let has_gui = (slot.info.hints & PLUGIN_HAS_CUSTOM_UI) != 0 && !unavailable;
                let gui_btn = child_ui
                    .add_enabled(
                        has_gui,
                        egui::Button::new("🗖")
                            .selected(slot.custom_ui_visible)
                            .min_size(Vec2::new(26.0, 24.0)),
                    )
                    .on_hover_text(if slot.custom_ui_failed {
                        "Editor failed. See Logs for details."
                    } else {
                        "Plugin Editor"
                    });
                if gui_btn.clicked() {
                    actions.push(RackAction::ToggleCustomUi(pid, !slot.custom_ui_visible));
                }

                let edit_btn = child_ui
                    .add_enabled(
                        !unavailable,
                        egui::Button::new("🎚")
                            .selected(slot.edit_open)
                            .min_size(Vec2::new(26.0, 24.0)),
                    )
                    .on_hover_text("Parameters");
                if edit_btn.clicked() {
                    actions.push(RackAction::ToggleEdit(pid));
                }

                // Actions menu button (⋯)
                let more_id = child_ui.make_persistent_id(("rack_more", pid));
                let more_btn = child_ui
                    .add(egui::Button::new("⋯").min_size(Vec2::new(26.0, 24.0)))
                    .on_hover_text("Plugin actions");
                if more_btn.clicked() {
                    egui::Popup::toggle_id(child_ui.ctx(), more_id);
                }
                egui::Popup::menu(&more_btn)
                    .id(more_id)
                    .show(|ui| {
                        render_plugin_context_menu(
                            ui,
                            slot,
                            idx,
                            total_plugins,
                            allow_export_lv2,
                            &mut actions,
                        );
                    });

                // 5. Right-aligned group: Format label + Yellow Clip Warning + Stacked Peak Meters
                child_ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    // Stacked Input (green) & Output (blue) meters
                    ui.vertical(|ui| {
                        ui.spacing_mut().item_spacing.y = 3.0;
                        slot.peak_in
                            .draw_horizontal(ui, Vec2::new(62.0, 12.0), now_secs, "Input level");
                        slot.peak_out
                            .draw_horizontal(ui, Vec2::new(62.0, 12.0), now_secs, "Output level");
                    });

                    // Yellow clip warning triangle
                    slot.clip_warning.draw(ui);

                    // Format label
                    let fmt_text = if slot.unavailable_error.is_some() {
                        egui::RichText::new("Unavailable")
                            .color(Color32::from_rgb(239, 68, 68))
                            .small()
                            .strong()
                    } else {
                        egui::RichText::new(plugin_type_label(slot.info.ptype))
                            .weak()
                            .small()
                    };
                    let lbl = ui.add_sized([74.0, 18.0], egui::Label::new(fmt_text));
                    if let Some(ref err) = slot.unavailable_error {
                        lbl.on_hover_text(err);
                    }
                });

                // Right-click context menu on the entire row
                row_resp.context_menu(|ui| {
                    render_plugin_context_menu(
                        ui,
                        slot,
                        idx,
                        total_plugins,
                        allow_export_lv2,
                        &mut actions,
                    );
                });
            }
        });

    // Handle active drag-to-reorder
    if let Some(src_idx) = drag_state.dragging_from {
        let pointer_down = ui.input(|i| i.pointer.primary_down());
        if let Some(pos) = ui.input(|i| i.pointer.interact_pos()) {
            let mut target = total_plugins.saturating_sub(1);
            for (i, r) in row_rects.iter().enumerate() {
                if pos.y < r.center().y {
                    target = i;
                    break;
                }
            }
            drag_state.drop_target = Some(target);

            // Draw crisp drop line indicator
            if let Some(r) = row_rects.get(target) {
                let y = if target > src_idx { r.max.y } else { r.min.y };
                ui.painter().line_segment(
                    [Pos2::new(r.min.x, y), Pos2::new(r.max.x, y)],
                    Stroke::new(2.5, Color32::from_rgb(59, 130, 246)),
                );
            }
        }
        if !pointer_down {
            if let Some(dst_idx) = drag_state.drop_target.take() {
                if dst_idx != src_idx {
                    actions.push(RackAction::MovePlugin(src_idx as u32, dst_idx as u32));
                }
            }
            drag_state.dragging_from = None;
        }
    }

    actions
}

fn render_plugin_context_menu(
    ui: &mut egui::Ui,
    slot: &RackPluginSlot,
    idx: usize,
    total_plugins: usize,
    allow_export_lv2: bool,
    actions: &mut Vec<RackAction>,
) {
    let pid = slot.plugin_id;
    let unavailable = slot.unavailable_error.is_some();

    if ui
        .add_enabled(
            !unavailable,
            egui::Button::new(if slot.active { "Bypass" } else { "Enable" }),
        )
        .clicked()
    {
        actions.push(RackAction::SetActive(pid, !slot.active));
        ui.close();
    }

    ui.separator();

    if ui
        .add_enabled(idx > 0, egui::Button::new("Move Up"))
        .clicked()
    {
        actions.push(RackAction::MovePlugin(pid, (idx - 1) as u32));
        ui.close();
    }
    if ui
        .add_enabled(idx + 1 < total_plugins, egui::Button::new("Move Down"))
        .clicked()
    {
        actions.push(RackAction::MovePlugin(pid, (idx + 1) as u32));
        ui.close();
    }

    ui.separator();

    ui.menu_button("Set Category", |ui| {
        for cat in FRANCA_PLUGIN_CATEGORIES {
            ui.horizontal(|ui| {
                draw_category_swatch(ui, cat.color);
                let is_cur = slot.category == cat.id;
                let text = if is_cur {
                    format!("✓ {}", cat.label)
                } else {
                    cat.label.to_string()
                };
                if ui.selectable_label(is_cur, text).clicked() {
                    actions.push(RackAction::SetCategory(pid, cat.id));
                    ui.close();
                }
            });
        }
    });

    ui.separator();

    if ui
        .add_enabled(!unavailable, egui::Button::new("Reset Parameters"))
        .clicked()
    {
        actions.push(RackAction::ResetParameters(pid));
        ui.close();
    }
    if ui
        .add_enabled(!unavailable, egui::Button::new("Randomize Parameters"))
        .clicked()
    {
        actions.push(RackAction::RandomizeParameters(pid));
        ui.close();
    }

    ui.separator();

    if ui
        .add_enabled(!unavailable, egui::Button::new("Edit Parameters"))
        .clicked()
    {
        actions.push(RackAction::ToggleEdit(pid));
        ui.close();
    }
    let has_gui = (slot.info.hints & PLUGIN_HAS_CUSTOM_UI) != 0 && !unavailable;
    if ui
        .add_enabled(
            has_gui,
            egui::Button::new(if slot.custom_ui_visible {
                "Hide Custom UI"
            } else {
                "Show Custom UI"
            }),
        )
        .clicked()
    {
        actions.push(RackAction::ToggleCustomUi(pid, !slot.custom_ui_visible));
        ui.close();
    }

    ui.separator();

    if ui.button("Clone").clicked() {
        actions.push(RackAction::ClonePlugin(pid));
        ui.close();
    }
    if ui.button("Rename...").clicked() {
        actions.push(RackAction::RenamePlugin(pid));
        ui.close();
    }
    if ui.button("Replace...").clicked() {
        actions.push(RackAction::ReplacePlugin(pid));
        ui.close();
    }
    if ui.button("Remove").clicked() {
        actions.push(RackAction::RemovePlugin(pid));
        ui.close();
    }

    if allow_export_lv2 {
        ui.separator();
        if ui.button("Export as LV2...").clicked() {
            actions.push(RackAction::ExportLv2(pid));
            ui.close();
        }
    }

    ui.separator();
    if ui.button("Find in Patchbay").clicked() {
        actions.push(RackAction::FindInPatchbay(pid));
        ui.close();
    }
}

fn elide_text(s: &str, max_chars: usize) -> String {
    let count = s.chars().count();
    if count <= max_chars {
        s.to_string()
    } else {
        let mut out: String = s.chars().take(max_chars.saturating_sub(1)).collect();
        out.push('…');
        out
    }
}
