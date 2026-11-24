// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::HashMap;

use eframe::egui::{self, Color32, CornerRadius, Pos2, Rect, Stroke, Vec2};

use crate::ffi::types::*;
use crate::patchbay::arrange::NodeKey;
use crate::patchbay::{
    BoxGeometry, HEADER_HEIGHT, PatchbayAction, PatchbayState, sample_cable_points,
    segments_intersect,
};

#[derive(Clone, Debug)]
enum ContextTarget {
    Canvas,
    Box(NodeKey),
    Port(u32, u32, bool),
}

pub fn show_patchbay_canvas(
    ui: &mut egui::Ui,
    state: &mut PatchbayState,
) -> Vec<PatchbayAction> {
    let mut actions = Vec::new();
    let avail = ui.available_size().max(Vec2::new(200.0, 200.0));
    state.last_viewport_size = avail;

    if state.needs_initial_fit && !state.groups.is_empty() {
        state.zoom_fit();
        state.needs_initial_fit = false;
    }

    let (vp_rect, response) =
        ui.allocate_exact_size(avail, egui::Sense::click_and_drag());

    let scene_to_screen = |p: Pos2, pan: Vec2, zoom: f32| -> Pos2 {
        vp_rect.center() + (p.to_vec2() - pan) * zoom
    };
    let screen_to_scene = |p: Pos2, pan: Vec2, zoom: f32| -> Pos2 {
        Pos2::ZERO + pan + (p - vp_rect.center()) / zoom
    };

    // Mouse wheel zoom (centered on pointer)
    if response.hovered() {
        let scroll = ui.input(|i| i.smooth_scroll_delta.y);
        if scroll.abs() > 0.1 {
            let pointer_screen = ui
                .input(|i| i.pointer.hover_pos())
                .unwrap_or(vp_rect.center());
            let before = screen_to_scene(pointer_screen, state.pan, state.zoom);
            let factor = (1.0 + scroll * 0.002).clamp(0.75, 1.35);
            state.zoom = (state.zoom * factor).clamp(0.2, 3.0);
            let after = screen_to_scene(pointer_screen, state.pan, state.zoom);
            state.pan += before - after;
        }
    }

    // Middle-button pan (without Ctrl) or Alt+Left pan
    let ctrl_held = ui.input(|i| i.modifiers.ctrl);
    let shift_held = ui.input(|i| i.modifiers.shift);

    if response.dragged_by(egui::PointerButton::Middle) && !ctrl_held {
        state.pan -= response.drag_delta() / state.zoom;
    }

    let geoms = state.compute_geometries();
    let pointer_screen = ui.input(|i| i.pointer.interact_pos());
    let pointer_scene = pointer_screen.map(|p| screen_to_scene(p, state.pan, state.zoom));

    // Helper: hit-test port or box in scene coordinates
    let hit_port = |sp: Pos2| -> Option<(u32, u32, bool)> {
        for g in geoms.iter().rev() {
            for &(pid, r) in &g.in_ports {
                if r.expand(3.0).contains(sp) {
                    return Some((g.key.group_id, pid, true));
                }
            }
            for &(pid, r) in &g.out_ports {
                if r.expand(3.0).contains(sp) {
                    return Some((g.key.group_id, pid, false));
                }
            }
        }
        None
    };

    let hit_box = |sp: Pos2| -> Option<NodeKey> {
        for g in geoms.iter().rev() {
            if g.rect.contains(sp) {
                return Some(g.key);
            }
        }
        None
    };

    // Ctrl + Middle drag: cable knife (cut connections intersecting drag line)
    if ctrl_held && response.drag_started_by(egui::PointerButton::Middle) {
        state.knife_start = pointer_scene;
        state.knife_end = pointer_scene;
    }
    if state.knife_start.is_some() {
        if response.dragged_by(egui::PointerButton::Middle) {
            state.knife_end = pointer_scene;
        } else {
            if let (Some(a), Some(b)) = (state.knife_start.take(), state.knife_end.take()) {
                for c in &state.connections {
                    let Some(p1) = state.port_anchor(&geoms, c.group_out, c.port_out, false) else {
                        continue;
                    };
                    let Some(p2) = state.port_anchor(&geoms, c.group_in, c.port_in, true) else {
                        continue;
                    };
                    let pts = sample_cable_points(p1, p2, state.use_bezier, 18);
                    if pts.windows(2).any(|w| segments_intersect(a, b, w[0], w[1])) {
                        actions.push(PatchbayAction::Disconnect(c.connection_id));
                    }
                }
            }
        }
    }

    // Ctrl + Right drag: zoom rectangle
    if ctrl_held && response.drag_started_by(egui::PointerButton::Secondary) {
        state.zoom_rect_start = pointer_scene;
        state.zoom_rect_end = pointer_scene;
    }
    if state.zoom_rect_start.is_some() {
        if response.dragged_by(egui::PointerButton::Secondary) {
            state.zoom_rect_end = pointer_scene;
        } else if let (Some(a), Some(b)) =
            (state.zoom_rect_start.take(), state.zoom_rect_end.take())
        {
            let r = Rect::from_two_pos(a, b);
            if r.width() > 20.0 && r.height() > 20.0 {
                let zx = vp_rect.width() / r.width();
                let zy = vp_rect.height() / r.height();
                state.zoom = zx.min(zy).clamp(0.2, 3.0);
                state.pan = r.center().to_vec2();
            }
        }
    }

    // Primary button drag: port connect / box move / rubberband select
    if response.drag_started_by(egui::PointerButton::Primary) {
        if let Some(sp) = pointer_scene {
            if let Some((gid, pid, is_in)) = hit_port(sp) {
                state.dragging_port = Some((gid, pid, is_in, sp));
            } else if let Some(bk) = hit_box(sp) {
                if !state.selected_boxes.contains(&bk) {
                    if !ctrl_held && !shift_held {
                        state.selected_boxes.clear();
                    }
                    state.selected_boxes.insert(bk);
                    emit_plugin_selection(state, &mut actions);
                }
                let mut origins = HashMap::new();
                for &sel in &state.selected_boxes {
                    if let Some(g) = state.groups.get(&sel.group_id) {
                        origins.insert(sel, if sel.split_part == 1 { g.pos2 } else { g.pos1 });
                    }
                }
                state.dragging_boxes = Some((sp, origins));
            } else {
                if !ctrl_held && !shift_held {
                    state.selected_boxes.clear();
                    emit_plugin_selection(state, &mut actions);
                }
                state.rubberband_start = Some(sp);
                state.rubberband_end = Some(sp);
            }
        }
    }

    // Primary click (without drag) for selection
    if response.clicked_by(egui::PointerButton::Primary) {
        if let Some(sp) = pointer_scene {
            if let Some(bk) = hit_box(sp) {
                if ctrl_held || shift_held {
                    if !state.selected_boxes.remove(&bk) {
                        state.selected_boxes.insert(bk);
                    }
                } else {
                    state.selected_boxes.clear();
                    state.selected_boxes.insert(bk);
                }
                emit_plugin_selection(state, &mut actions);
            } else if !ctrl_held && !shift_held {
                state.selected_boxes.clear();
                emit_plugin_selection(state, &mut actions);
            }
        }
    }

    // Active box dragging
    if let Some((start_sp, ref origins)) = state.dragging_boxes.clone() {
        if response.dragged_by(egui::PointerButton::Primary) {
            if let Some(cur_sp) = pointer_scene {
                let delta = cur_sp - start_sp;
                for (&bk, &orig) in origins {
                    if let Some(g) = state.groups.get_mut(&bk.group_id) {
                        let np = Pos2::new(
                            (orig.x + delta.x).clamp(1.0, state.canvas_size.x - 40.0),
                            (orig.y + delta.y).clamp(1.0, state.canvas_size.y - 40.0),
                        );
                        if bk.split_part == 1 {
                            g.pos2 = np;
                        } else {
                            g.pos1 = np;
                        }
                        g.has_custom_pos = true;
                    }
                }
            }
        } else {
            let mut moved_groups = std::collections::HashSet::new();
            for bk in origins.keys() {
                moved_groups.insert(bk.group_id);
            }
            for gid in moved_groups {
                if let Some(g) = state.groups.get(&gid) {
                    actions.push(PatchbayAction::GroupPosition(
                        gid,
                        g.pos1.x.round() as i32,
                        g.pos1.y.round() as i32,
                        if g.split { g.pos2.x.round() as i32 } else { 0 },
                        if g.split { g.pos2.y.round() as i32 } else { 0 },
                    ));
                }
            }
            state.dragging_boxes = None;
        }
    }

    // Active port drag-to-connect / disconnect
    if let Some((src_g, src_p, src_in, _)) = state.dragging_port {
        if response.dragged_by(egui::PointerButton::Primary) {
            if let Some(cur_sp) = pointer_scene {
                state.dragging_port = Some((src_g, src_p, src_in, cur_sp));
            }
        } else {
            state.dragging_port = None;
            if let Some(cur_sp) = pointer_scene {
                if let Some((dst_g, dst_p, dst_in)) = hit_port(cur_sp) {
                    if src_in != dst_in {
                        let p1 = state
                            .ports
                            .iter()
                            .find(|p| p.group_id == src_g && p.port_id == src_p);
                        let p2 = state
                            .ports
                            .iter()
                            .find(|p| p.group_id == dst_g && p.port_id == dst_p);
                        if let (Some(a), Some(b)) = (p1, p2) {
                            if a.compatible_with(b) {
                                let (g_out, p_out, g_in, p_in) = if !src_in {
                                    (src_g, src_p, dst_g, dst_p)
                                } else {
                                    (dst_g, dst_p, src_g, src_p)
                                };
                                if let Some(cid) = state.find_connection(g_out, p_out, g_in, p_in) {
                                    actions.push(PatchbayAction::Disconnect(cid));
                                } else {
                                    actions.push(PatchbayAction::Connect(g_out, p_out, g_in, p_in));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Active rubberband selection
    if state.rubberband_start.is_some() {
        if response.dragged_by(egui::PointerButton::Primary) {
            state.rubberband_end = pointer_scene;
        } else if let (Some(a), Some(b)) =
            (state.rubberband_start.take(), state.rubberband_end.take())
        {
            let sel_rect = Rect::from_two_pos(a, b);
            for g in &geoms {
                if sel_rect.intersects(g.rect) {
                    state.selected_boxes.insert(g.key);
                }
            }
            emit_plugin_selection(state, &mut actions);
        }
    }

    // --- Paint ---
    let painter = ui.painter_at(vp_rect);
    painter.rect_filled(vp_rect, CornerRadius::ZERO, Color32::from_rgb(24, 26, 31));

    // Subtle background dot grid
    let grid_step = 32.0 * state.zoom;
    if grid_step >= 10.0 {
        let top_left_scene = screen_to_scene(vp_rect.min, state.pan, state.zoom);
        let bot_right_scene = screen_to_scene(vp_rect.max, state.pan, state.zoom);
        let gx0 = (top_left_scene.x / 32.0).floor() as i32;
        let gx1 = (bot_right_scene.x / 32.0).ceil() as i32;
        let gy0 = (top_left_scene.y / 32.0).floor() as i32;
        let gy1 = (bot_right_scene.y / 32.0).ceil() as i32;
        let grid_col = Color32::from_rgb(34, 37, 45);
        for gx in gx0..=gx1 {
            let sx = scene_to_screen(Pos2::new(gx as f32 * 32.0, 0.0), state.pan, state.zoom).x;
            painter.line_segment(
                [Pos2::new(sx, vp_rect.min.y), Pos2::new(sx, vp_rect.max.y)],
                Stroke::new(0.6, grid_col),
            );
        }
        for gy in gy0..=gy1 {
            let sy = scene_to_screen(Pos2::new(0.0, gy as f32 * 32.0), state.pan, state.zoom).y;
            painter.line_segment(
                [Pos2::new(vp_rect.min.x, sy), Pos2::new(vp_rect.max.x, sy)],
                Stroke::new(0.6, grid_col),
            );
        }
    }

    // Canvas boundary outline
    let canvas_screen = Rect::from_min_max(
        scene_to_screen(Pos2::ZERO, state.pan, state.zoom),
        scene_to_screen(Pos2::ZERO + state.canvas_size, state.pan, state.zoom),
    );
    painter.rect_stroke(
        canvas_screen,
        CornerRadius::ZERO,
        Stroke::new(1.0, Color32::from_rgb(52, 56, 68)),
        egui::StrokeKind::Inside,
    );

    // Recompute geometries after any drag update
    let geoms = state.compute_geometries();

    // Draw boxes
    for g in &geoms {
        draw_box(
            &painter,
            state,
            g,
            vp_rect,
            state.pan,
            state.zoom,
            state.selected_boxes.contains(&g.key),
        );
    }

    // Draw existing cables
    for c in &state.connections {
        let Some(p1) = state.port_anchor(&geoms, c.group_out, c.port_out, false) else {
            continue;
        };
        let Some(p2) = state.port_anchor(&geoms, c.group_in, c.port_in, true) else {
            continue;
        };
        let is_selected = state
            .selected_boxes
            .iter()
            .any(|k| k.group_id == c.group_out || k.group_id == c.group_in);
        let base_col = state
            .ports
            .iter()
            .find(|p| p.group_id == c.group_out && p.port_id == c.port_out)
            .map_or(Color32::from_rgb(96, 165, 250), |p| p.cable_color());
        let col = if is_selected {
            Color32::from_rgb(
                base_col.r().saturating_add(35),
                base_col.g().saturating_add(35),
                base_col.b().saturating_add(35),
            )
        } else {
            base_col
        };
        let width = if is_selected {
            (2.8 * state.zoom).clamp(1.6, 5.0)
        } else {
            (2.0 * state.zoom).clamp(1.2, 4.0)
        };
        let pts: Vec<Pos2> = sample_cable_points(p1, p2, state.use_bezier, 24)
            .into_iter()
            .map(|p| scene_to_screen(p, state.pan, state.zoom))
            .collect();
        painter.add(egui::Shape::line(pts, Stroke::new(width, col)));
    }

    // Draw active drag preview cable
    if let Some((src_g, src_p, src_in, cursor_sp)) = state.dragging_port {
        if let Some(anchor) = state.port_anchor(&geoms, src_g, src_p, src_in) {
            let (p1, p2) = if src_in {
                (cursor_sp, anchor)
            } else {
                (anchor, cursor_sp)
            };
            let pts: Vec<Pos2> = sample_cable_points(p1, p2, state.use_bezier, 20)
                .into_iter()
                .map(|p| scene_to_screen(p, state.pan, state.zoom))
                .collect();
            painter.add(egui::Shape::line(
                pts,
                Stroke::new(2.2, Color32::from_rgb(250, 204, 21)),
            ));
        }
    }

    // Rubberband selection overlay
    if let (Some(a), Some(b)) = (state.rubberband_start, state.rubberband_end) {
        let r = Rect::from_two_pos(
            scene_to_screen(a, state.pan, state.zoom),
            scene_to_screen(b, state.pan, state.zoom),
        );
        painter.rect_filled(
            r,
            CornerRadius::ZERO,
            Color32::from_rgba_unmultiplied(59, 130, 246, 36),
        );
        painter.rect_stroke(
            r,
            CornerRadius::ZERO,
            Stroke::new(1.0, Color32::from_rgb(96, 165, 250)),
            egui::StrokeKind::Inside,
        );
    }

    // Zoom rectangle overlay
    if let (Some(a), Some(b)) = (state.zoom_rect_start, state.zoom_rect_end) {
        let r = Rect::from_two_pos(
            scene_to_screen(a, state.pan, state.zoom),
            scene_to_screen(b, state.pan, state.zoom),
        );
        painter.rect_filled(
            r,
            CornerRadius::ZERO,
            Color32::from_rgba_unmultiplied(234, 179, 8, 36),
        );
        painter.rect_stroke(
            r,
            CornerRadius::ZERO,
            Stroke::new(1.0, Color32::from_rgb(250, 204, 21)),
            egui::StrokeKind::Inside,
        );
    }

    // Knife line overlay
    if let (Some(a), Some(b)) = (state.knife_start, state.knife_end) {
        painter.line_segment(
            [
                scene_to_screen(a, state.pan, state.zoom),
                scene_to_screen(b, state.pan, state.zoom),
            ],
            Stroke::new(2.0, Color32::from_rgb(239, 68, 68)),
        );
    }

    // Context menu (only when Ctrl is not held, since Ctrl+Right drags zoom rect)
    if !ctrl_held {
        let menu_target_id = ui.scope_id().with("patchbay_ctx_target");
        if response.secondary_clicked() {
            let target = pointer_scene
                .and_then(|sp| {
                    hit_port(sp)
                        .map(|(g, p, is_in)| ContextTarget::Port(g, p, is_in))
                        .or_else(|| hit_box(sp).map(ContextTarget::Box))
                })
                .unwrap_or(ContextTarget::Canvas);
            ui.data_mut(|d| d.insert_temp(menu_target_id, target));
        }

        response.context_menu(|ui| {
            let target = ui
                .data(|d| d.get_temp::<ContextTarget>(menu_target_id))
                .unwrap_or(ContextTarget::Canvas);
            match target {
                ContextTarget::Port(gid, pid, is_in) => {
                    let port_opt = state
                        .ports
                        .iter()
                        .find(|p| p.group_id == gid && p.port_id == pid)
                        .cloned();
                    if let Some(port) = port_opt {
                        ui.label(egui::RichText::new(&port.name).strong());
                        ui.separator();

                        // Disconnect submenu
                        let existing: Vec<(u32, String)> = state
                            .connections
                            .iter()
                            .filter_map(|c| {
                                if !is_in && c.group_out == gid && c.port_out == pid {
                                    let gn = state
                                        .groups
                                        .get(&c.group_in)
                                        .map(|g| g.name.as_str())
                                        .unwrap_or("?");
                                    let pn = state
                                        .ports
                                        .iter()
                                        .find(|p| p.group_id == c.group_in && p.port_id == c.port_in)
                                        .map(|p| p.name.as_str())
                                        .unwrap_or("?");
                                    Some((c.connection_id, format!("{gn} : {pn}")))
                                } else if is_in && c.group_in == gid && c.port_in == pid {
                                    let gn = state
                                        .groups
                                        .get(&c.group_out)
                                        .map(|g| g.name.as_str())
                                        .unwrap_or("?");
                                    let pn = state
                                        .ports
                                        .iter()
                                        .find(|p| p.group_id == c.group_out && p.port_id == c.port_out)
                                        .map(|p| p.name.as_str())
                                        .unwrap_or("?");
                                    Some((c.connection_id, format!("{gn} : {pn}")))
                                } else {
                                    None
                                }
                            })
                            .collect();

                        if !existing.is_empty() {
                            ui.menu_button("Disconnect", |ui| {
                                for (cid, label) in &existing {
                                    if ui.button(label).clicked() {
                                        actions.push(PatchbayAction::Disconnect(*cid));
                                        ui.close();
                                    }
                                }
                            });
                            if ui.button("Disconnect All").clicked() {
                                for (cid, _) in &existing {
                                    actions.push(PatchbayAction::Disconnect(*cid));
                                }
                                ui.close();
                            }
                        }

                        // Connect submenu
                        let candidates: Vec<(u32, u32, String)> = state
                            .ports
                            .iter()
                            .filter(|other| port.compatible_with(other) && other.group_id != gid)
                            .filter_map(|other| {
                                let (g_out, p_out, g_in, p_in) = if !is_in {
                                    (gid, pid, other.group_id, other.port_id)
                                } else {
                                    (other.group_id, other.port_id, gid, pid)
                                };
                                if state.find_connection(g_out, p_out, g_in, p_in).is_some() {
                                    return None;
                                }
                                let gn = state
                                    .groups
                                    .get(&other.group_id)
                                    .map(|g| g.name.as_str())
                                    .unwrap_or("?");
                                Some((other.group_id, other.port_id, format!("{gn} : {}", other.name)))
                            })
                            .collect();
                        if !candidates.is_empty() {
                            ui.menu_button("Connect to...", |ui| {
                                for (og, op, label) in candidates {
                                    if ui.button(label).clicked() {
                                        if !is_in {
                                            actions.push(PatchbayAction::Connect(gid, pid, og, op));
                                        } else {
                                            actions.push(PatchbayAction::Connect(og, op, gid, pid));
                                        }
                                        ui.close();
                                    }
                                }
                            });
                        }
                    }
                }
                ContextTarget::Box(bk) => {
                    if let Some(g) = state.groups.get(&bk.group_id).cloned() {
                        ui.label(egui::RichText::new(&g.name).strong());
                        ui.separator();

                        let has_in = state.ports.iter().any(|p| p.group_id == g.group_id && p.is_input());
                        let has_out = state.ports.iter().any(|p| p.group_id == g.group_id && !p.is_input());
                        if g.split {
                            if ui.button("Join").clicked() {
                                state.join_group(g.group_id);
                                ui.close();
                            }
                        } else if has_in && has_out && ui.button("Split").clicked() {
                            state.split_group(g.group_id);
                            ui.close();
                        }

                        let cids: Vec<u32> = state
                            .connections
                            .iter()
                            .filter(|c| c.group_out == g.group_id || c.group_in == g.group_id)
                            .map(|c| c.connection_id)
                            .collect();
                        if !cids.is_empty() && ui.button("Disconnect All").clicked() {
                            for cid in cids {
                                actions.push(PatchbayAction::Disconnect(cid));
                            }
                            ui.close();
                        }

                        if g.plugin_id >= 0 {
                            let pid = g.plugin_id as u32;
                            ui.separator();
                            if ui.button("Edit Parameters").clicked() {
                                actions.push(PatchbayAction::PluginEdit(pid));
                                ui.close();
                            }
                            if ui.button("Show Custom UI").clicked() {
                                actions.push(PatchbayAction::PluginCustomUi(pid));
                                ui.close();
                            }
                            ui.separator();
                            if ui.button("Clone").clicked() {
                                actions.push(PatchbayAction::PluginClone(pid));
                                ui.close();
                            }
                            if ui.button("Rename...").clicked() {
                                actions.push(PatchbayAction::PluginRename(pid));
                                ui.close();
                            }
                            if ui.button("Replace...").clicked() {
                                actions.push(PatchbayAction::PluginReplace(pid));
                                ui.close();
                            }
                            if ui.button("Remove").clicked() {
                                actions.push(PatchbayAction::PluginRemove(pid));
                                ui.close();
                            }
                        }
                    }
                }
                ContextTarget::Canvas => {
                    if ui.button("Arrange").clicked() {
                        actions.push(PatchbayAction::ArrangeRequested);
                        ui.close();
                    }
                    if ui.button("Refresh").clicked() {
                        actions.push(PatchbayAction::RefreshRequested);
                        ui.close();
                    }
                    ui.separator();
                    if ui.button("Auto-Fit").clicked() {
                        state.zoom_fit();
                        ui.close();
                    }
                    if ui.button("Zoom 100%").clicked() {
                        state.zoom_reset();
                        ui.close();
                    }
                }
            }
        });
    }

    actions
}

fn emit_plugin_selection(state: &PatchbayState, actions: &mut Vec<PatchbayAction>) {
    let mut pids: Vec<u32> = state
        .selected_boxes
        .iter()
        .filter_map(|bk| state.groups.get(&bk.group_id))
        .filter(|g| g.plugin_id >= 0)
        .map(|g| g.plugin_id as u32)
        .collect();
    pids.sort_unstable();
    pids.dedup();
    actions.push(PatchbayAction::SelectPlugins(pids));
}

fn draw_box(
    painter: &egui::Painter,
    state: &PatchbayState,
    g: &BoxGeometry,
    vp_rect: Rect,
    pan: Vec2,
    zoom: f32,
    selected: bool,
) {
    let to_screen = |p: Pos2| vp_rect.center() + (p.to_vec2() - pan) * zoom;
    let rect = Rect::from_min_max(to_screen(g.rect.min), to_screen(g.rect.max));
    if !rect.intersects(vp_rect) {
        return;
    }

    let bg = Color32::from_rgb(36, 39, 46);
    let hdr_bg = Color32::from_rgb(48, 52, 62);
    let border = if selected {
        Color32::from_rgb(59, 130, 246)
    } else {
        Color32::from_rgb(76, 82, 96)
    };

    painter.rect_filled(rect, CornerRadius::same(4), bg);

    let hdr_rect = Rect::from_min_max(
        rect.min,
        Pos2::new(rect.max.x, rect.min.y + HEADER_HEIGHT * zoom),
    );
    painter.rect_filled(hdr_rect, CornerRadius::same(4), hdr_bg);

    painter.rect_stroke(
        rect,
        CornerRadius::same(4),
        Stroke::new(if selected { 1.8 } else { 1.0 }, border),
        egui::StrokeKind::Inside,
    );

    if zoom >= 0.35 {
        let font_size = (12.0 * zoom).clamp(7.5, 18.0);
        let icon_prefix = state
            .groups
            .get(&g.key.group_id)
            .map(|gr| match gr.icon {
                PATCHBAY_ICON_HARDWARE => "◈ ",
                PATCHBAY_ICON_CARLA => "◆ ",
                PATCHBAY_ICON_PLUGIN => "▪ ",
                _ => "",
            })
            .unwrap_or("");
        painter.text(
            hdr_rect.center(),
            egui::Align2::CENTER_CENTER,
            format!("{icon_prefix}{}", g.title),
            egui::FontId::proportional(font_size),
            Color32::from_rgb(236, 238, 244),
        );
    }

    let port_font = (10.5 * zoom).clamp(7.0, 15.0);
    let tip = (6.0 * zoom).clamp(2.0, 9.0);

    // Input ports (left side, right-pointing chevron on right edge)
    for &(pid, r_scene) in &g.in_ports {
        let r = Rect::from_min_max(to_screen(r_scene.min), to_screen(r_scene.max));
        let port = state
            .ports
            .iter()
            .find(|p| p.group_id == g.key.group_id && p.port_id == pid);
        let fill = port.map_or(Color32::from_rgb(78, 122, 168), |p| p.color());
        let poly = vec![
            r.left_top(),
            Pos2::new(r.max.x - tip, r.min.y),
            Pos2::new(r.max.x, r.center().y),
            Pos2::new(r.max.x - tip, r.max.y),
            r.left_bottom(),
        ];
        painter.add(egui::Shape::convex_polygon(
            poly,
            fill,
            Stroke::new(0.8, Color32::from_rgba_unmultiplied(0, 0, 0, 90)),
        ));
        if zoom >= 0.4 {
            if let Some(p) = port {
                painter.text(
                    Pos2::new(r.min.x + 5.0 * zoom, r.center().y),
                    egui::Align2::LEFT_CENTER,
                    &p.name,
                    egui::FontId::proportional(port_font),
                    Color32::from_rgb(245, 247, 250),
                );
            }
        }
    }

    // Output ports (right side, left-pointing chevron on left edge)
    for &(pid, r_scene) in &g.out_ports {
        let r = Rect::from_min_max(to_screen(r_scene.min), to_screen(r_scene.max));
        let port = state
            .ports
            .iter()
            .find(|p| p.group_id == g.key.group_id && p.port_id == pid);
        let fill = port.map_or(Color32::from_rgb(78, 122, 168), |p| p.color());
        let poly = vec![
            Pos2::new(r.min.x + tip, r.min.y),
            r.right_top(),
            r.right_bottom(),
            Pos2::new(r.min.x + tip, r.max.y),
            Pos2::new(r.min.x, r.center().y),
        ];
        painter.add(egui::Shape::convex_polygon(
            poly,
            fill,
            Stroke::new(0.8, Color32::from_rgba_unmultiplied(0, 0, 0, 90)),
        ));
        if zoom >= 0.4 {
            if let Some(p) = port {
                painter.text(
                    Pos2::new(r.max.x - 5.0 * zoom, r.center().y),
                    egui::Align2::RIGHT_CENTER,
                    &p.name,
                    egui::FontId::proportional(port_font),
                    Color32::from_rgb(245, 247, 250),
                );
            }
        }
    }
}

/// Interactive Minimap widget rendered in the left Side Panel.
pub fn show_minimap(ui: &mut egui::Ui, state: &mut PatchbayState) {
    let w = ui.available_width().max(120.0);
    let aspect = (state.canvas_size.y / state.canvas_size.x.max(1.0)).clamp(0.45, 1.0);
    let h = (w * aspect).clamp(90.0, 180.0);
    let (rect, response) = ui.allocate_exact_size(Vec2::new(w, h), egui::Sense::click_and_drag());

    let sx = rect.width() / state.canvas_size.x.max(1.0);
    let sy = rect.height() / state.canvas_size.y.max(1.0);

    if response.clicked() || response.dragged() {
        if let Some(pos) = response.interact_pointer_pos() {
            let nx = ((pos.x - rect.min.x) / sx).clamp(0.0, state.canvas_size.x);
            let ny = ((pos.y - rect.min.y) / sy).clamp(0.0, state.canvas_size.y);
            state.pan = Vec2::new(nx, ny);
        }
    }

    let painter = ui.painter_at(rect);
    painter.rect_filled(rect, CornerRadius::same(3), Color32::from_rgb(18, 20, 25));
    painter.rect_stroke(
        rect,
        CornerRadius::same(3),
        Stroke::new(1.0, Color32::from_rgb(58, 63, 74)),
        egui::StrokeKind::Inside,
    );

    let to_mini = |p: Pos2| Pos2::new(rect.min.x + p.x * sx, rect.min.y + p.y * sy);
    let geoms = state.compute_geometries();

    for c in &state.connections {
        if let (Some(p1), Some(p2)) = (
            state.port_anchor(&geoms, c.group_out, c.port_out, false),
            state.port_anchor(&geoms, c.group_in, c.port_in, true),
        ) {
            painter.line_segment(
                [to_mini(p1), to_mini(p2)],
                Stroke::new(0.8, Color32::from_rgb(96, 165, 250)),
            );
        }
    }

    for g in &geoms {
        let mr = Rect::from_min_max(to_mini(g.rect.min), to_mini(g.rect.max));
        let col = if state.selected_boxes.contains(&g.key) {
            Color32::from_rgb(59, 130, 246)
        } else {
            Color32::from_rgb(110, 118, 135)
        };
        painter.rect_filled(mr, CornerRadius::ZERO, col);
    }

    // Current viewport indicator
    let half_vp = (state.last_viewport_size * 0.5) / state.zoom.max(0.1);
    let vp_scene = Rect::from_center_size(Pos2::ZERO + state.pan, half_vp * 2.0);
    let vp_mini = Rect::from_min_max(to_mini(vp_scene.min), to_mini(vp_scene.max));
    painter.rect_stroke(
        vp_mini,
        CornerRadius::ZERO,
        Stroke::new(1.2, Color32::from_rgb(250, 204, 21)),
        egui::StrokeKind::Inside,
    );
}
