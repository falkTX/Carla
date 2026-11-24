// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

pub mod arrange;
pub mod widget;

use std::collections::{BTreeMap, HashMap, HashSet};
use std::path::Path;

use eframe::egui::{Color32, Pos2, Rect, Vec2};
use image::{ImageBuffer, Rgba};

use crate::ffi::types::*;
use crate::history::{GroupPosSnapshot, PatchbayEndpoint};
use crate::patchbay::arrange::{LayoutEdge, LayoutNode, NodeKey};

pub const HEADER_HEIGHT: f32 = 26.0;
pub const PORT_HEIGHT: f32 = 18.0;
pub const PORT_GAP: f32 = 4.0;
pub const BOX_PAD_BOTTOM: f32 = 6.0;
pub const PORT_TEXT_PAD: f32 = 18.0;

#[derive(Clone, Debug)]
pub struct PatchbayPort {
    pub group_id: u32,
    pub port_id: u32,
    pub name: String,
    pub hints: u32,
}

impl PatchbayPort {
    pub fn is_input(&self) -> bool {
        (self.hints & PATCHBAY_PORT_IS_INPUT) != 0
    }

    pub fn is_audio(&self) -> bool {
        (self.hints & PATCHBAY_PORT_TYPE_AUDIO) != 0
    }

    pub fn is_midi(&self) -> bool {
        (self.hints & PATCHBAY_PORT_TYPE_MIDI) != 0
    }

    pub fn is_cv(&self) -> bool {
        (self.hints & PATCHBAY_PORT_TYPE_CV) != 0
    }

    pub fn color(&self) -> Color32 {
        if self.is_audio() {
            Color32::from_rgb(0x4E, 0x7A, 0xA8)
        } else if self.is_midi() {
            Color32::from_rgb(0xB8, 0x4A, 0x5A)
        } else if self.is_cv() {
            Color32::from_rgb(0x3B, 0x9C, 0x75)
        } else {
            Color32::from_rgb(0x88, 0x70, 0xA8)
        }
    }

    pub fn cable_color(&self) -> Color32 {
        if self.is_audio() {
            Color32::from_rgb(0x60, 0xA5, 0xFA)
        } else if self.is_midi() {
            Color32::from_rgb(0xF8, 0x71, 0x71)
        } else if self.is_cv() {
            Color32::from_rgb(0x34, 0xD3, 0x99)
        } else {
            Color32::from_rgb(0xC0, 0x84, 0xFC)
        }
    }

    pub fn compatible_with(&self, other: &PatchbayPort) -> bool {
        if self.is_input() == other.is_input() {
            return false;
        }
        let type_mask = PATCHBAY_PORT_TYPE_AUDIO
            | PATCHBAY_PORT_TYPE_CV
            | PATCHBAY_PORT_TYPE_MIDI
            | PATCHBAY_PORT_TYPE_OSC;
        let t1 = self.hints & type_mask;
        let t2 = other.hints & type_mask;
        t1 == t2 || (self.is_audio() && other.is_cv()) || (self.is_cv() && other.is_audio())
    }
}

#[derive(Clone, Debug)]
pub struct PatchbayGroup {
    pub group_id: u32,
    pub plugin_id: i32, // -1 if not a plugin
    pub icon: i32,
    pub name: String,
    pub split: bool,
    pub pos1: Pos2, // joined box, or output box when split
    pub pos2: Pos2, // input box when split
    pub has_custom_pos: bool,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PatchbayConnection {
    pub connection_id: u32,
    pub group_out: u32,
    pub port_out: u32,
    pub group_in: u32,
    pub port_in: u32,
}

#[derive(Clone, Debug)]
pub struct BoxGeometry {
    pub key: NodeKey,
    pub rect: Rect,
    pub title: String,
    pub in_ports: Vec<(u32, Rect)>,
    pub out_ports: Vec<(u32, Rect)>,
}

#[derive(Clone, Debug)]
pub enum PatchbayAction {
    Connect(u32, u32, u32, u32),
    Disconnect(u32),
    GroupPosition(u32, i32, i32, i32, i32),
    SelectPlugins(Vec<u32>),
    PluginEdit(u32),
    PluginCustomUi(u32),
    PluginClone(u32),
    PluginRename(u32),
    PluginReplace(u32),
    PluginRemove(u32),
    ArrangeRequested,
    RefreshRequested,
}

pub struct PatchbayState {
    pub groups: BTreeMap<u32, PatchbayGroup>,
    pub ports: Vec<PatchbayPort>,
    pub connections: Vec<PatchbayConnection>,

    pub canvas_size: Vec2,
    pub pan: Vec2,
    pub zoom: f32,
    pub selected_boxes: HashSet<NodeKey>,
    pub auto_hide_groups: bool,
    pub use_bezier: bool,

    // Interactive drag states
    pub dragging_boxes: Option<(Pos2, HashMap<NodeKey, Pos2>)>,
    pub dragging_port: Option<(u32, u32, bool, Pos2)>, // (group_id, port_id, is_input, cursor_scene_pos)
    pub rubberband_start: Option<Pos2>,
    pub rubberband_end: Option<Pos2>,
    pub zoom_rect_start: Option<Pos2>,
    pub zoom_rect_end: Option<Pos2>,
    pub knife_start: Option<Pos2>,
    pub knife_end: Option<Pos2>,
    pub needs_initial_fit: bool,
    pub last_viewport_size: Vec2,
    pub next_auto_pos_index: usize,
}

impl Default for PatchbayState {
    fn default() -> Self {
        Self::new(3100.0, 2400.0)
    }
}

impl PatchbayState {
    pub fn new(w: f32, h: f32) -> Self {
        Self {
            groups: BTreeMap::new(),
            ports: Vec::new(),
            connections: Vec::new(),
            canvas_size: Vec2::new(w, h),
            pan: Vec2::new(w * 0.5, h * 0.5),
            zoom: 1.0,
            selected_boxes: HashSet::new(),
            auto_hide_groups: true,
            use_bezier: true,
            dragging_boxes: None,
            dragging_port: None,
            rubberband_start: None,
            rubberband_end: None,
            zoom_rect_start: None,
            zoom_rect_end: None,
            knife_start: None,
            knife_end: None,
            needs_initial_fit: true,
            last_viewport_size: Vec2::new(900.0, 600.0),
            next_auto_pos_index: 0,
        }
    }

    pub fn clear(&mut self) {
        self.groups.clear();
        self.ports.clear();
        self.connections.clear();
        self.selected_boxes.clear();
        self.dragging_boxes = None;
        self.dragging_port = None;
        self.rubberband_start = None;
        self.rubberband_end = None;
        self.zoom_rect_start = None;
        self.zoom_rect_end = None;
        self.knife_start = None;
        self.knife_end = None;
        self.next_auto_pos_index = 0;
    }

    pub fn add_group(&mut self, group_id: u32, plugin_id: i32, icon: i32, name: String) {
        let should_split = icon == PATCHBAY_ICON_HARDWARE;
        let idx = self.next_auto_pos_index;
        self.next_auto_pos_index += 1;
        let cx = self.canvas_size.x * 0.5;
        let cy = self.canvas_size.y * 0.5;
        let default_pos1 = if should_split {
            Pos2::new(cx - 320.0, cy - 180.0 + (idx % 6) as f32 * 95.0)
        } else {
            Pos2::new(
                cx - 100.0 + ((idx % 3) as f32 - 1.0) * 230.0,
                cy - 180.0 + (idx / 3) as f32 * 110.0,
            )
        };
        let default_pos2 = Pos2::new(cx + 220.0, cy - 180.0 + (idx % 6) as f32 * 95.0);

        self.groups.insert(
            group_id,
            PatchbayGroup {
                group_id,
                plugin_id,
                icon,
                name,
                split: should_split,
                pos1: default_pos1,
                pos2: default_pos2,
                has_custom_pos: false,
            },
        );
    }

    pub fn remove_group(&mut self, group_id: u32) {
        self.groups.remove(&group_id);
        self.ports.retain(|p| p.group_id != group_id);
        self.connections
            .retain(|c| c.group_out != group_id && c.group_in != group_id);
        self.selected_boxes.retain(|k| k.group_id != group_id);
    }

    pub fn rename_group(&mut self, group_id: u32, name: String) {
        if let Some(g) = self.groups.get_mut(&group_id) {
            g.name = name;
        }
    }

    pub fn set_group_data(&mut self, group_id: u32, plugin_id: i32, icon: i32) {
        if let Some(g) = self.groups.get_mut(&group_id) {
            g.plugin_id = plugin_id;
            g.icon = icon;
        }
    }

    pub fn set_group_position(&mut self, group_id: u32, x1: i32, y1: i32, x2: i32, y2: i32) {
        if let Some(g) = self.groups.get_mut(&group_id) {
            g.pos1 = Pos2::new(x1 as f32, y1 as f32);
            if g.split || (x2 != 0 || y2 != 0) {
                g.pos2 = Pos2::new(x2 as f32, y2 as f32);
            }
            g.has_custom_pos = true;
        }
    }

    pub fn add_port(&mut self, group_id: u32, port_id: u32, hints: u32, name: String) {
        if let Some(existing) = self
            .ports
            .iter_mut()
            .find(|p| p.group_id == group_id && p.port_id == port_id)
        {
            existing.hints = hints;
            existing.name = name;
            return;
        }
        self.ports.push(PatchbayPort {
            group_id,
            port_id,
            name,
            hints,
        });
    }

    pub fn remove_port(&mut self, group_id: u32, port_id: u32) {
        self.ports
            .retain(|p| !(p.group_id == group_id && p.port_id == port_id));
        self.connections.retain(|c| {
            !((c.group_out == group_id && c.port_out == port_id)
                || (c.group_in == group_id && c.port_in == port_id))
        });
    }

    pub fn update_port(&mut self, group_id: u32, port_id: u32, hints: u32, name: String) {
        if let Some(p) = self
            .ports
            .iter_mut()
            .find(|p| p.group_id == group_id && p.port_id == port_id)
        {
            p.hints = hints;
            p.name = name;
        }
    }

    pub fn add_connection(
        &mut self,
        connection_id: u32,
        group_out: u32,
        port_out: u32,
        group_in: u32,
        port_in: u32,
    ) {
        if self
            .connections
            .iter()
            .any(|c| c.connection_id == connection_id)
        {
            return;
        }
        self.connections.push(PatchbayConnection {
            connection_id,
            group_out,
            port_out,
            group_in,
            port_in,
        });
    }

    pub fn remove_connection(&mut self, connection_id: u32) {
        self.connections
            .retain(|c| c.connection_id != connection_id);
    }

    pub fn split_group(&mut self, group_id: u32) -> bool {
        let Some(g) = self.groups.get_mut(&group_id) else {
            return false;
        };
        if g.split {
            return false;
        }
        g.split = true;
        g.pos2 = Pos2::new(g.pos1.x + 260.0, g.pos1.y);
        true
    }

    pub fn join_group(&mut self, group_id: u32) -> bool {
        let Some(g) = self.groups.get_mut(&group_id) else {
            return false;
        };
        if !g.split {
            return false;
        }
        g.split = false;
        true
    }

    pub fn handle_plugin_removed(&mut self, plugin_id: u32) {
        let pid = plugin_id as i32;
        for g in self.groups.values_mut() {
            if g.plugin_id == pid {
                g.plugin_id = -1;
            } else if g.plugin_id > pid {
                g.plugin_id -= 1;
            }
        }
    }

    pub fn compute_geometries(&self) -> Vec<BoxGeometry> {
        let mut out = Vec::new();
        for g in self.groups.values() {
            let ins: Vec<&PatchbayPort> = self
                .ports
                .iter()
                .filter(|p| p.group_id == g.group_id && p.is_input())
                .collect();
            let outs: Vec<&PatchbayPort> = self
                .ports
                .iter()
                .filter(|p| p.group_id == g.group_id && !p.is_input())
                .collect();

            if g.split {
                if !outs.is_empty() || !self.auto_hide_groups {
                    out.push(Self::build_box_geom(
                        NodeKey {
                            group_id: g.group_id,
                            split_part: 0,
                        },
                        g.pos1,
                        &g.name,
                        &[],
                        &outs,
                    ));
                }
                if !ins.is_empty() || !self.auto_hide_groups {
                    out.push(Self::build_box_geom(
                        NodeKey {
                            group_id: g.group_id,
                            split_part: 1,
                        },
                        g.pos2,
                        &g.name,
                        &ins,
                        &[],
                    ));
                }
            } else if !ins.is_empty() || !outs.is_empty() || !self.auto_hide_groups {
                out.push(Self::build_box_geom(
                    NodeKey {
                        group_id: g.group_id,
                        split_part: 0,
                    },
                    g.pos1,
                    &g.name,
                    &ins,
                    &outs,
                ));
            }
        }
        out
    }

    fn estimate_text_width(s: &str, char_w: f32) -> f32 {
        (s.chars().count() as f32) * char_w
    }

    fn build_box_geom(
        key: NodeKey,
        pos: Pos2,
        title: &str,
        ins: &[&PatchbayPort],
        outs: &[&PatchbayPort],
    ) -> BoxGeometry {
        let max_in_w = ins
            .iter()
            .map(|p| Self::estimate_text_width(&p.name, 7.0) + PORT_TEXT_PAD)
            .fold(0.0_f32, f32::max);
        let max_out_w = outs
            .iter()
            .map(|p| Self::estimate_text_width(&p.name, 7.0) + PORT_TEXT_PAD)
            .fold(0.0_f32, f32::max);
        let title_w = Self::estimate_text_width(title, 7.6) + 28.0;

        let ports_w = if !ins.is_empty() && !outs.is_empty() {
            max_in_w + max_out_w + 18.0
        } else {
            max_in_w.max(max_out_w) + 12.0
        };
        let width = title_w.max(ports_w).max(116.0);
        let rows = ins.len().max(outs.len());
        let height = HEADER_HEIGHT
            + if rows > 0 {
                rows as f32 * (PORT_HEIGHT + PORT_GAP) + BOX_PAD_BOTTOM
            } else {
                12.0
            };

        let rect = Rect::from_min_size(pos, Vec2::new(width, height));
        let mut in_ports = Vec::with_capacity(ins.len());
        for (i, p) in ins.iter().enumerate() {
            let y = pos.y + HEADER_HEIGHT + 2.0 + i as f32 * (PORT_HEIGHT + PORT_GAP);
            let pw = if outs.is_empty() {
                (width - 6.0).max(48.0)
            } else {
                max_in_w.max(48.0)
            };
            in_ports.push((
                p.port_id,
                Rect::from_min_size(Pos2::new(pos.x, y), Vec2::new(pw, PORT_HEIGHT)),
            ));
        }

        let mut out_ports = Vec::with_capacity(outs.len());
        for (i, p) in outs.iter().enumerate() {
            let y = pos.y + HEADER_HEIGHT + 2.0 + i as f32 * (PORT_HEIGHT + PORT_GAP);
            let pw = if ins.is_empty() {
                (width - 6.0).max(48.0)
            } else {
                max_out_w.max(48.0)
            };
            out_ports.push((
                p.port_id,
                Rect::from_min_size(Pos2::new(pos.x + width - pw, y), Vec2::new(pw, PORT_HEIGHT)),
            ));
        }

        BoxGeometry {
            key,
            rect,
            title: title.to_string(),
            in_ports,
            out_ports,
        }
    }

    pub fn port_anchor(&self, geoms: &[BoxGeometry], group_id: u32, port_id: u32, is_input: bool) -> Option<Pos2> {
        for g in geoms {
            if g.key.group_id != group_id {
                continue;
            }
            if is_input {
                if let Some((_, r)) = g.in_ports.iter().find(|(pid, _)| *pid == port_id) {
                    return Some(Pos2::new(r.min.x, r.center().y));
                }
            } else if let Some((_, r)) = g.out_ports.iter().find(|(pid, _)| *pid == port_id) {
                return Some(Pos2::new(r.max.x, r.center().y));
            }
        }
        None
    }

    pub fn arrange_nodes(&mut self) -> Vec<PatchbayAction> {
        let geoms = self.compute_geometries();
        if geoms.is_empty() {
            return Vec::new();
        }
        let mut nodes = Vec::with_capacity(geoms.len());
        let mut port_owner: HashMap<(u32, u32, bool), NodeKey> = HashMap::new();

        for g in &geoms {
            let mut in_y = HashMap::new();
            for &(pid, r) in &g.in_ports {
                in_y.insert(pid, (r.center().y - g.rect.min.y) as f64);
                port_owner.insert((g.key.group_id, pid, true), g.key);
            }
            let mut out_y = HashMap::new();
            for &(pid, r) in &g.out_ports {
                out_y.insert(pid, (r.center().y - g.rect.min.y) as f64);
                port_owner.insert((g.key.group_id, pid, false), g.key);
            }
            nodes.push(LayoutNode {
                key: g.key,
                name: g.title.clone(),
                width: g.rect.width() as f64,
                height: g.rect.height() as f64,
                has_inputs: !g.in_ports.is_empty(),
                has_outputs: !g.out_ports.is_empty(),
                in_port_y: in_y,
                out_port_y: out_y,
            });
        }

        let mut edges = Vec::new();
        for c in &self.connections {
            if let (Some(&sk), Some(&dk)) = (
                port_owner.get(&(c.group_out, c.port_out, false)),
                port_owner.get(&(c.group_in, c.port_in, true)),
            ) {
                edges.push(LayoutEdge {
                    src_key: sk,
                    src_port_id: c.port_out,
                    dst_key: dk,
                    dst_port_id: c.port_in,
                });
            }
        }

        let rect = (0.0, 0.0, self.canvas_size.x as f64, self.canvas_size.y as f64);
        let placed = arrange::compute_layout(&nodes, &edges, Some(rect), self.use_bezier);

        for (k, (ix, iy)) in &placed {
            if let Some(g) = self.groups.get_mut(&k.group_id) {
                if k.split_part == 1 {
                    g.pos2 = Pos2::new(*ix as f32, *iy as f32);
                } else {
                    g.pos1 = Pos2::new(*ix as f32, *iy as f32);
                }
                g.has_custom_pos = true;
            }
        }

        let actions: Vec<PatchbayAction> = self
            .groups
            .values()
            .map(|g| {
                let (x2, y2) = if g.split {
                    (g.pos2.x.round() as i32, g.pos2.y.round() as i32)
                } else {
                    (0, 0)
                };
                PatchbayAction::GroupPosition(
                    g.group_id,
                    g.pos1.x.round() as i32,
                    g.pos1.y.round() as i32,
                    x2,
                    y2,
                )
            })
            .collect();

        self.zoom_fit();
        actions
    }

    pub fn capture_positions(&self) -> Vec<GroupPosSnapshot> {
        self.groups
            .values()
            .map(|g| GroupPosSnapshot {
                group_name: g.name.clone(),
                split: g.split,
                pos1: (g.pos1.x.round() as i32, g.pos1.y.round() as i32),
                pos2: if g.split {
                    (g.pos2.x.round() as i32, g.pos2.y.round() as i32)
                } else {
                    (0, 0)
                },
            })
            .collect()
    }

    pub fn apply_positions(&mut self, positions: &[GroupPosSnapshot]) -> Vec<PatchbayAction> {
        let mut by_name: HashMap<&str, u32> = HashMap::new();
        for g in self.groups.values() {
            by_name.insert(&g.name, g.group_id);
        }
        let mut updates = Vec::new();
        for snap in positions {
            if let Some(&gid) = by_name.get(snap.group_name.as_str()) {
                updates.push((gid, snap.pos1, snap.pos2));
            }
        }
        let mut actions = Vec::new();
        for (gid, (x1, y1), (x2, y2)) in updates {
            if let Some(g) = self.groups.get_mut(&gid) {
                g.pos1 = Pos2::new(x1 as f32, y1 as f32);
                if g.split {
                    g.pos2 = Pos2::new(x2 as f32, y2 as f32);
                }
                g.has_custom_pos = true;
                actions.push(PatchbayAction::GroupPosition(
                    gid,
                    x1,
                    y1,
                    if g.split { x2 } else { 0 },
                    if g.split { y2 } else { 0 },
                ));
            }
        }
        self.zoom_fit();
        actions
    }

    pub fn endpoint_for(&self, group_id: u32, port_id: u32) -> Option<PatchbayEndpoint> {
        let g = self.groups.get(&group_id)?;
        let p = self
            .ports
            .iter()
            .find(|p| p.group_id == group_id && p.port_id == port_id)?;
        Some(PatchbayEndpoint {
            group_name: g.name.clone(),
            port_name: p.name.clone(),
            is_input: p.is_input(),
        })
    }

    pub fn lookup_endpoint(&self, ep: &PatchbayEndpoint) -> Option<(u32, u32)> {
        let g = self.groups.values().find(|g| g.name == ep.group_name)?;
        let p = self
            .ports
            .iter()
            .find(|p| p.group_id == g.group_id && p.name == ep.port_name && p.is_input() == ep.is_input)?;
        Some((g.group_id, p.port_id))
    }

    pub fn find_connection(&self, g_out: u32, p_out: u32, g_in: u32, p_in: u32) -> Option<u32> {
        self.connections
            .iter()
            .find(|c| {
                c.group_out == g_out
                    && c.port_out == p_out
                    && c.group_in == g_in
                    && c.port_in == p_in
            })
            .map(|c| c.connection_id)
    }

    pub fn bounding_rect(&self) -> Option<Rect> {
        let geoms = self.compute_geometries();
        if geoms.is_empty() {
            return None;
        }
        let mut r = geoms[0].rect;
        for g in &geoms[1..] {
            r = r.union(g.rect);
        }
        Some(r)
    }

    pub fn zoom_fit(&mut self) {
        let Some(bounds) = self.bounding_rect() else {
            self.pan = self.canvas_size * 0.5;
            self.zoom = 1.0;
            return;
        };
        let padded = bounds.expand(48.0);
        let vp = self.last_viewport_size.max(Vec2::new(200.0, 200.0));
        let zx = vp.x / padded.width().max(1.0);
        let zy = vp.y / padded.height().max(1.0);
        self.zoom = zx.min(zy).clamp(0.2, 2.0);
        self.pan = padded.center().to_vec2();
    }

    pub fn zoom_in(&mut self) {
        self.zoom = (self.zoom * 1.18).clamp(0.2, 3.0);
    }

    pub fn zoom_out(&mut self) {
        self.zoom = (self.zoom / 1.18).clamp(0.2, 3.0);
    }

    pub fn zoom_reset(&mut self) {
        self.zoom = 1.0;
    }

    pub fn focus_plugin(&mut self, plugin_id: u32, fallback_name: &str) -> bool {
        let pid = plugin_id as i32;
        let gid = self
            .groups
            .values()
            .find(|g| g.plugin_id == pid)
            .or_else(|| self.groups.values().find(|g| g.name == fallback_name))
            .map(|g| g.group_id);
        let Some(gid) = gid else {
            return false;
        };
        let geoms = self.compute_geometries();
        let matching: Vec<&BoxGeometry> = geoms.iter().filter(|g| g.key.group_id == gid).collect();
        if matching.is_empty() {
            return false;
        }
        self.selected_boxes.clear();
        let mut union = matching[0].rect;
        for g in &matching {
            self.selected_boxes.insert(g.key);
            union = union.union(g.rect);
        }
        self.pan = union.center().to_vec2();
        true
    }

    /// Render the current patchbay scene to an RGBA image buffer for Save Image / Copy Clipboard.
    pub fn render_to_rgba(&self, scale: f32) -> (u32, u32, Vec<u8>) {
        let geoms = self.compute_geometries();
        let bounds = self
            .bounding_rect()
            .unwrap_or_else(|| Rect::from_min_size(Pos2::ZERO, Vec2::new(640.0, 480.0)))
            .expand(48.0);
        let scale = scale.clamp(0.5, 4.0);
        let w = ((bounds.width() * scale).ceil() as u32).clamp(64, 8192);
        let h = ((bounds.height() * scale).ceil() as u32).clamp(64, 8192);

        let mut img: ImageBuffer<Rgba<u8>, Vec<u8>> =
            ImageBuffer::from_pixel(w, h, Rgba([24, 26, 32, 255]));

        let to_px = |p: Pos2| -> (f32, f32) {
            ((p.x - bounds.min.x) * scale, (p.y - bounds.min.y) * scale)
        };

        let draw_rect_filled = |img: &mut ImageBuffer<Rgba<u8>, Vec<u8>>, r: Rect, c: Color32| {
            let (x0, y0) = to_px(r.min);
            let (x1, y1) = to_px(r.max);
            let ix0 = (x0.floor() as i32).max(0) as u32;
            let iy0 = (y0.floor() as i32).max(0) as u32;
            let ix1 = (x1.ceil() as i32).min(w as i32) as u32;
            let iy1 = (y1.ceil() as i32).min(h as i32) as u32;
            let px = Rgba([c.r(), c.g(), c.b(), c.a()]);
            for y in iy0..iy1 {
                for x in ix0..ix1 {
                    img.put_pixel(x, y, px);
                }
            }
        };

        let draw_rect_border = |img: &mut ImageBuffer<Rgba<u8>, Vec<u8>>, r: Rect, c: Color32| {
            let (x0, y0) = to_px(r.min);
            let (x1, y1) = to_px(r.max);
            let ix0 = (x0.floor() as i32).max(0) as u32;
            let iy0 = (y0.floor() as i32).max(0) as u32;
            let ix1 = (x1.ceil() as i32).min(w as i32 - 1).max(0) as u32;
            let iy1 = (y1.ceil() as i32).min(h as i32 - 1).max(0) as u32;
            let px = Rgba([c.r(), c.g(), c.b(), 255]);
            for x in ix0..=ix1 {
                img.put_pixel(x, iy0, px);
                img.put_pixel(x, iy1, px);
            }
            for y in iy0..=iy1 {
                img.put_pixel(ix0, y, px);
                img.put_pixel(ix1, y, px);
            }
        };

        let draw_thick_segment =
            |img: &mut ImageBuffer<Rgba<u8>, Vec<u8>>, a: Pos2, b: Pos2, c: Color32| {
                let (x0, y0) = to_px(a);
                let (x1, y1) = to_px(b);
                let steps = ((x1 - x0).hypot(y1 - y0).ceil() as usize).max(1);
                let px = Rgba([c.r(), c.g(), c.b(), 255]);
                let rad = (scale * 1.1).round() as i32;
                for i in 0..=steps {
                    let t = i as f32 / steps as f32;
                    let cx = (x0 + (x1 - x0) * t).round() as i32;
                    let cy = (y0 + (y1 - y0) * t).round() as i32;
                    for dy in -rad..=rad {
                        for dx in -rad..=rad {
                            let xx = cx + dx;
                            let yy = cy + dy;
                            if xx >= 0 && yy >= 0 && (xx as u32) < w && (yy as u32) < h {
                                img.put_pixel(xx as u32, yy as u32, px);
                            }
                        }
                    }
                }
            };

        // Draw boxes and ports
        for g in &geoms {
            draw_rect_filled(&mut img, g.rect, Color32::from_rgb(38, 41, 49));
            let hdr = Rect::from_min_size(g.rect.min, Vec2::new(g.rect.width(), HEADER_HEIGHT));
            draw_rect_filled(&mut img, hdr, Color32::from_rgb(52, 56, 66));
            draw_rect_border(&mut img, g.rect, Color32::from_rgb(90, 96, 110));

            for &(pid, r) in &g.in_ports {
                let col = self
                    .ports
                    .iter()
                    .find(|p| p.group_id == g.key.group_id && p.port_id == pid)
                    .map_or(Color32::from_rgb(78, 122, 168), |p| p.color());
                draw_rect_filled(&mut img, r, col);
            }
            for &(pid, r) in &g.out_ports {
                let col = self
                    .ports
                    .iter()
                    .find(|p| p.group_id == g.key.group_id && p.port_id == pid)
                    .map_or(Color32::from_rgb(78, 122, 168), |p| p.color());
                draw_rect_filled(&mut img, r, col);
            }
        }

        // Draw cables on top
        for c in &self.connections {
            let Some(p1) = self.port_anchor(&geoms, c.group_out, c.port_out, false) else {
                continue;
            };
            let Some(p2) = self.port_anchor(&geoms, c.group_in, c.port_in, true) else {
                continue;
            };
            let col = self
                .ports
                .iter()
                .find(|p| p.group_id == c.group_out && p.port_id == c.port_out)
                .map_or(Color32::from_rgb(96, 165, 250), |p| p.cable_color());

            let pts = sample_cable_points(p1, p2, self.use_bezier, 28);
            for pair in pts.windows(2) {
                draw_thick_segment(&mut img, pair[0], pair[1], col);
            }
        }

        (w, h, img.into_raw())
    }

    pub fn save_png(&self, path: &Path, scale: f32) -> Result<(), String> {
        let (w, h, rgba) = self.render_to_rgba(scale);
        let img: ImageBuffer<Rgba<u8>, Vec<u8>> =
            ImageBuffer::from_raw(w, h, rgba).ok_or("Failed to construct image buffer")?;
        img.save(path).map_err(|e| e.to_string())
    }
}

pub fn sample_cable_points(p1: Pos2, p2: Pos2, use_bezier: bool, steps: usize) -> Vec<Pos2> {
    if !use_bezier {
        return vec![p1, p2];
    }
    let dx = (p2.x - p1.x).abs();
    let offset = (dx * 0.5).max(36.0).min(240.0);
    let c1 = Pos2::new(p1.x + offset, p1.y);
    let c2 = Pos2::new(p2.x - offset, p2.y);

    let mut pts = Vec::with_capacity(steps + 1);
    for i in 0..=steps {
        let t = i as f32 / steps as f32;
        let u = 1.0 - t;
        let x = u * u * u * p1.x
            + 3.0 * u * u * t * c1.x
            + 3.0 * u * t * t * c2.x
            + t * t * t * p2.x;
        let y = u * u * u * p1.y
            + 3.0 * u * u * t * c1.y
            + 3.0 * u * t * t * c2.y
            + t * t * t * p2.y;
        pts.push(Pos2::new(x, y));
    }
    pts
}

pub fn segments_intersect(a: Pos2, b: Pos2, c: Pos2, d: Pos2) -> bool {
    fn orient(p: Pos2, q: Pos2, r: Pos2) -> f32 {
        (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x)
    }
    let o1 = orient(a, b, c);
    let o2 = orient(a, b, d);
    let o3 = orient(c, d, a);
    let o4 = orient(c, d, b);
    (o1 * o2 < 0.0) && (o3 * o4 < 0.0)
}
