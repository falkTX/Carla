// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

pub mod arrange;

use std::collections::{BTreeMap, HashMap, HashSet};
use std::path::Path;

use image::{ImageBuffer, Rgba};
use serde::Serialize;

use crate::ffi::types::*;
use crate::history::{GroupPosSnapshot, PatchbayEndpoint};
use crate::patchbay::arrange::{LayoutEdge, LayoutNode, NodeKey};

pub const HEADER_HEIGHT: f32 = 26.0;
pub const PORT_HEIGHT: f32 = 18.0;
pub const PORT_GAP: f32 = 4.0;
pub const BOX_PAD_BOTTOM: f32 = 6.0;
pub const PORT_TEXT_PAD: f32 = 18.0;

#[derive(Clone, Copy, Debug, Default, Serialize)]
pub struct PointF {
    pub x: f32,
    pub y: f32,
}

impl PointF {
    pub const fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
}

#[derive(Clone, Copy, Debug, Default, Serialize)]
pub struct RectF {
    pub x: f32,
    pub y: f32,
    pub w: f32,
    pub h: f32,
}

impl RectF {
    pub const fn new(x: f32, y: f32, w: f32, h: f32) -> Self {
        Self { x, y, w, h }
    }

    pub fn right(&self) -> f32 {
        self.x + self.w
    }

    pub fn bottom(&self) -> f32 {
        self.y + self.h
    }

    pub fn center_y(&self) -> f32 {
        self.y + self.h * 0.5
    }

    pub fn union(&self, other: &RectF) -> RectF {
        let min_x = self.x.min(other.x);
        let min_y = self.y.min(other.y);
        let max_x = self.right().max(other.right());
        let max_y = self.bottom().max(other.bottom());
        RectF::new(min_x, min_y, max_x - min_x, max_y - min_y)
    }

    pub fn expand(&self, margin: f32) -> RectF {
        RectF::new(
            self.x - margin,
            self.y - margin,
            self.w + 2.0 * margin,
            self.h + 2.0 * margin,
        )
    }
}

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

    pub fn color_rgb(&self) -> [u8; 3] {
        if self.is_audio() {
            [0x4E, 0x7A, 0xA8]
        } else if self.is_midi() {
            [0xB8, 0x4A, 0x5A]
        } else if self.is_cv() {
            [0x3B, 0x9C, 0x75]
        } else {
            [0x88, 0x70, 0xA8]
        }
    }

    pub fn color_hex(&self) -> &'static str {
        if self.is_audio() {
            "#4E7AA8"
        } else if self.is_midi() {
            "#B84A5A"
        } else if self.is_cv() {
            "#3B9C75"
        } else {
            "#8870A8"
        }
    }

    pub fn cable_rgb(&self) -> [u8; 3] {
        if self.is_audio() {
            [0x60, 0xA5, 0xFA]
        } else if self.is_midi() {
            [0xF8, 0x71, 0x71]
        } else if self.is_cv() {
            [0x34, 0xD3, 0x99]
        } else {
            [0xC0, 0x84, 0xFC]
        }
    }

    pub fn cable_hex(&self) -> &'static str {
        if self.is_audio() {
            "#60A5FA"
        } else if self.is_midi() {
            "#F87171"
        } else if self.is_cv() {
            "#34D399"
        } else {
            "#C084FC"
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
    pub plugin_id: i32,
    pub icon: i32,
    pub name: String,
    pub split: bool,
    pub pos1: PointF,
    pub pos2: PointF,
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

#[derive(Clone, Debug, Serialize)]
pub struct PortGeomDto {
    pub group_id: u32,
    pub port_id: u32,
    pub name: String,
    pub is_input: bool,
    pub hints: u32,
    pub color: String,
    pub rect: RectF,
    pub anchor_x: f32,
    pub anchor_y: f32,
}

#[derive(Clone, Debug, Serialize)]
pub struct BoxGeomDto {
    pub group_id: u32,
    pub split_part: u8,
    pub plugin_id: i32,
    pub icon: i32,
    pub title: String,
    pub split: bool,
    pub can_split: bool,
    pub selected: bool,
    pub rect: RectF,
    pub in_ports: Vec<PortGeomDto>,
    pub out_ports: Vec<PortGeomDto>,
}

#[derive(Clone, Debug, Serialize)]
pub struct ConnectionDto {
    pub connection_id: u32,
    pub group_out: u32,
    pub port_out: u32,
    pub group_in: u32,
    pub port_in: u32,
    pub x1: f32,
    pub y1: f32,
    pub x2: f32,
    pub y2: f32,
    pub color: String,
    pub selected: bool,
}

#[derive(Clone, Debug, Serialize)]
pub struct PatchbaySceneDto {
    pub canvas_width: f32,
    pub canvas_height: f32,
    pub use_bezier: bool,
    pub boxes: Vec<BoxGeomDto>,
    pub connections: Vec<ConnectionDto>,
}

pub struct PatchbayState {
    pub groups: BTreeMap<u32, PatchbayGroup>,
    pub ports: Vec<PatchbayPort>,
    pub connections: Vec<PatchbayConnection>,
    pub canvas_width: f32,
    pub canvas_height: f32,
    pub selected_boxes: HashSet<NodeKey>,
    pub auto_hide_groups: bool,
    pub use_bezier: bool,
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
            canvas_width: w,
            canvas_height: h,
            selected_boxes: HashSet::new(),
            auto_hide_groups: true,
            use_bezier: true,
            next_auto_pos_index: 0,
        }
    }

    pub fn clear(&mut self) {
        self.groups.clear();
        self.ports.clear();
        self.connections.clear();
        self.selected_boxes.clear();
        self.next_auto_pos_index = 0;
    }

    pub fn add_group(&mut self, group_id: u32, plugin_id: i32, icon: i32, name: String) {
        if let Some(existing) = self.groups.get_mut(&group_id) {
            existing.plugin_id = plugin_id;
            existing.icon = icon;
            existing.name = name;
            return;
        }
        let should_split = icon == PATCHBAY_ICON_HARDWARE;
        let idx = self.next_auto_pos_index;
        self.next_auto_pos_index += 1;
        let cx = self.canvas_width * 0.5;
        let cy = self.canvas_height * 0.5;
        let default_pos1 = if should_split {
            PointF::new(cx - 320.0, cy - 180.0 + (idx % 6) as f32 * 95.0)
        } else {
            PointF::new(
                cx - 100.0 + ((idx % 3) as f32 - 1.0) * 230.0,
                cy - 180.0 + (idx / 3) as f32 * 110.0,
            )
        };
        let default_pos2 = PointF::new(cx + 220.0, cy - 180.0 + (idx % 6) as f32 * 95.0);

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
            g.pos1 = PointF::new(x1 as f32, y1 as f32);
            if g.split || (x2 != 0 || y2 != 0) {
                g.pos2 = PointF::new(x2 as f32, y2 as f32);
            }
            g.has_custom_pos = true;
        }
    }

    pub fn move_box(&mut self, group_id: u32, split_part: u8, x: f32, y: f32) {
        if let Some(g) = self.groups.get_mut(&group_id) {
            let p = PointF::new(
                x.clamp(1.0, self.canvas_width - 40.0),
                y.clamp(1.0, self.canvas_height - 40.0),
            );
            if split_part == 1 {
                g.pos2 = p;
            } else {
                g.pos1 = p;
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
        g.pos2 = PointF::new(g.pos1.x + 260.0, g.pos1.y);
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

    fn estimate_title_width(s: &str, icon: i32) -> f32 {
        let text_w: f32 = s
            .chars()
            .map(|c| match c {
                'i' | 'l' | 'I' | '1' | '|' | '!' | '.' | ',' | ':' | ';' | '\'' | ' ' => 4.6,
                'f' | 'j' | 'r' | 't' | '-' | '/' | '(' | ')' => 5.4,
                'm' | 'w' | 'M' | 'W' | '@' | '%' => 9.6,
                c if c.is_ascii_uppercase() => 8.0,
                _ => 6.9,
            })
            .sum();
        let icon_w = if icon == 1 || icon == 2 || icon == 3 {
            16.0
        } else {
            0.0
        };
        text_w + icon_w + 24.0
    }

    fn estimate_port_text_width(s: &str) -> f32 {
        s.chars()
            .map(|c| match c {
                'i' | 'l' | 'I' | '1' | '|' | '!' | '.' | ',' | ':' | ';' | '\'' | ' ' => 4.2,
                'f' | 'j' | 'r' | 't' | '-' | '/' | '(' | ')' => 4.9,
                'm' | 'w' | 'M' | 'W' | '@' | '%' => 8.6,
                c if c.is_ascii_uppercase() => 7.2,
                _ => 6.2,
            })
            .sum()
    }

    fn build_box_dto(
        &self,
        g: &PatchbayGroup,
        split_part: u8,
        pos: PointF,
        ins: &[&PatchbayPort],
        outs: &[&PatchbayPort],
        can_split: bool,
    ) -> BoxGeomDto {
        let max_in_w = ins
            .iter()
            .map(|p| Self::estimate_port_text_width(&p.name) + PORT_TEXT_PAD)
            .fold(0.0_f32, f32::max);
        let max_out_w = outs
            .iter()
            .map(|p| Self::estimate_port_text_width(&p.name) + PORT_TEXT_PAD)
            .fold(0.0_f32, f32::max);
        let title_w = Self::estimate_title_width(&g.name, g.icon);

        let ports_w = if !ins.is_empty() && !outs.is_empty() {
            max_in_w + max_out_w + 18.0
        } else {
            max_in_w.max(max_out_w) + 12.0
        };
        let width = title_w.max(ports_w).max(122.0);
        let rows = ins.len().max(outs.len());
        let height = HEADER_HEIGHT
            + if rows > 0 {
                rows as f32 * (PORT_HEIGHT + PORT_GAP) + BOX_PAD_BOTTOM
            } else {
                12.0
            };

        let rect = RectF::new(pos.x, pos.y, width, height);
        let in_col_w = max_in_w.max(46.0).min((width - 6.0).max(46.0));
        let mut in_ports = Vec::with_capacity(ins.len());
        for (i, p) in ins.iter().enumerate() {
            let y = pos.y + HEADER_HEIGHT + 2.0 + i as f32 * (PORT_HEIGHT + PORT_GAP);
            let pr = RectF::new(pos.x, y, in_col_w, PORT_HEIGHT);
            in_ports.push(PortGeomDto {
                group_id: p.group_id,
                port_id: p.port_id,
                name: p.name.clone(),
                is_input: true,
                hints: p.hints,
                color: p.color_hex().to_string(),
                rect: pr,
                anchor_x: pr.x,
                anchor_y: pr.center_y(),
            });
        }

        let out_col_w = max_out_w.max(46.0).min((width - 6.0).max(46.0));
        let mut out_ports = Vec::with_capacity(outs.len());
        for (i, p) in outs.iter().enumerate() {
            let y = pos.y + HEADER_HEIGHT + 2.0 + i as f32 * (PORT_HEIGHT + PORT_GAP);
            let pr = RectF::new(pos.x + width - out_col_w, y, out_col_w, PORT_HEIGHT);
            out_ports.push(PortGeomDto {
                group_id: p.group_id,
                port_id: p.port_id,
                name: p.name.clone(),
                is_input: false,
                hints: p.hints,
                color: p.color_hex().to_string(),
                rect: pr,
                anchor_x: pr.right(),
                anchor_y: pr.center_y(),
            });
        }

        let key = NodeKey {
            group_id: g.group_id,
            split_part,
        };
        BoxGeomDto {
            group_id: g.group_id,
            split_part,
            plugin_id: g.plugin_id,
            icon: g.icon,
            title: g.name.clone(),
            split: g.split,
            can_split,
            selected: self.selected_boxes.contains(&key),
            rect,
            in_ports,
            out_ports,
        }
    }

    pub fn build_scene_dto(&self) -> PatchbaySceneDto {
        let mut boxes = Vec::new();
        let mut anchors: HashMap<(u32, u32, bool), (f32, f32)> = HashMap::new();

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
            let can_split = !ins.is_empty() && !outs.is_empty();

            if g.split {
                if !outs.is_empty() || !self.auto_hide_groups {
                    let b = self.build_box_dto(g, 0, g.pos1, &[], &outs, can_split);
                    for p in &b.out_ports {
                        anchors.insert((p.group_id, p.port_id, false), (p.anchor_x, p.anchor_y));
                    }
                    boxes.push(b);
                }
                if !ins.is_empty() || !self.auto_hide_groups {
                    let b = self.build_box_dto(g, 1, g.pos2, &ins, &[], can_split);
                    for p in &b.in_ports {
                        anchors.insert((p.group_id, p.port_id, true), (p.anchor_x, p.anchor_y));
                    }
                    boxes.push(b);
                }
            } else if !ins.is_empty() || !outs.is_empty() || !self.auto_hide_groups {
                let b = self.build_box_dto(g, 0, g.pos1, &ins, &outs, can_split);
                for p in &b.in_ports {
                    anchors.insert((p.group_id, p.port_id, true), (p.anchor_x, p.anchor_y));
                }
                for p in &b.out_ports {
                    anchors.insert((p.group_id, p.port_id, false), (p.anchor_x, p.anchor_y));
                }
                boxes.push(b);
            }
        }

        let mut connections = Vec::new();
        for c in &self.connections {
            let Some(&(x1, y1)) = anchors.get(&(c.group_out, c.port_out, false)) else {
                continue;
            };
            let Some(&(x2, y2)) = anchors.get(&(c.group_in, c.port_in, true)) else {
                continue;
            };
            let color = self
                .ports
                .iter()
                .find(|p| p.group_id == c.group_out && p.port_id == c.port_out)
                .map_or("#60A5FA", |p| p.cable_hex())
                .to_string();
            let selected = self
                .selected_boxes
                .iter()
                .any(|k| k.group_id == c.group_out || k.group_id == c.group_in);
            connections.push(ConnectionDto {
                connection_id: c.connection_id,
                group_out: c.group_out,
                port_out: c.port_out,
                group_in: c.group_in,
                port_in: c.port_in,
                x1,
                y1,
                x2,
                y2,
                color,
                selected,
            });
        }

        PatchbaySceneDto {
            canvas_width: self.canvas_width,
            canvas_height: self.canvas_height,
            use_bezier: self.use_bezier,
            boxes,
            connections,
        }
    }

    pub fn arrange_nodes(&mut self) -> Vec<(u32, i32, i32, i32, i32)> {
        let scene = self.build_scene_dto();
        if scene.boxes.is_empty() {
            return Vec::new();
        }
        let mut nodes = Vec::with_capacity(scene.boxes.len());
        let mut port_owner: HashMap<(u32, u32, bool), NodeKey> = HashMap::new();

        for b in &scene.boxes {
            let key = NodeKey {
                group_id: b.group_id,
                split_part: b.split_part,
            };
            let mut in_y = HashMap::new();
            for p in &b.in_ports {
                in_y.insert(p.port_id, (p.anchor_y - b.rect.y) as f64);
                port_owner.insert((b.group_id, p.port_id, true), key);
            }
            let mut out_y = HashMap::new();
            for p in &b.out_ports {
                out_y.insert(p.port_id, (p.anchor_y - b.rect.y) as f64);
                port_owner.insert((b.group_id, p.port_id, false), key);
            }
            nodes.push(LayoutNode {
                key,
                name: b.title.clone(),
                width: b.rect.w as f64,
                height: b.rect.h as f64,
                has_inputs: !b.in_ports.is_empty(),
                has_outputs: !b.out_ports.is_empty(),
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

        let rect = (0.0, 0.0, self.canvas_width as f64, self.canvas_height as f64);
        let placed = arrange::compute_layout(&nodes, &edges, Some(rect), self.use_bezier);

        for (k, (ix, iy)) in &placed {
            if let Some(g) = self.groups.get_mut(&k.group_id) {
                if k.split_part == 1 {
                    g.pos2 = PointF::new(*ix as f32, *iy as f32);
                } else {
                    g.pos1 = PointF::new(*ix as f32, *iy as f32);
                }
                g.has_custom_pos = true;
            }
        }

        self.groups
            .values()
            .map(|g| {
                let (x2, y2) = if g.split {
                    (g.pos2.x.round() as i32, g.pos2.y.round() as i32)
                } else {
                    (0, 0)
                };
                (
                    g.group_id,
                    g.pos1.x.round() as i32,
                    g.pos1.y.round() as i32,
                    x2,
                    y2,
                )
            })
            .collect()
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

    pub fn apply_positions(&mut self, positions: &[GroupPosSnapshot]) -> Vec<(u32, i32, i32, i32, i32)> {
        let mut by_name: HashMap<&str, u32> = HashMap::new();
        for g in self.groups.values() {
            by_name.insert(&g.name, g.group_id);
        }
        let updates: Vec<(u32, (i32, i32), (i32, i32))> = positions
            .iter()
            .filter_map(|s| Some((*by_name.get(s.group_name.as_str())?, s.pos1, s.pos2)))
            .collect();

        let mut out = Vec::new();
        for (gid, (x1, y1), (x2, y2)) in updates {
            if let Some(g) = self.groups.get_mut(&gid) {
                g.pos1 = PointF::new(x1 as f32, y1 as f32);
                if g.split {
                    g.pos2 = PointF::new(x2 as f32, y2 as f32);
                }
                g.has_custom_pos = true;
                out.push((gid, x1, y1, if g.split { x2 } else { 0 }, if g.split { y2 } else { 0 }));
            }
        }
        out
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
        let p = self.ports.iter().find(|p| {
            p.group_id == g.group_id && p.name == ep.port_name && p.is_input() == ep.is_input
        })?;
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
        let scene = self.build_scene_dto();
        self.selected_boxes.clear();
        for b in &scene.boxes {
            if b.group_id == gid {
                self.selected_boxes.insert(NodeKey {
                    group_id: b.group_id,
                    split_part: b.split_part,
                });
            }
        }
        !self.selected_boxes.is_empty()
    }

    pub fn connections_intersecting_segment(&self, x1: f32, y1: f32, x2: f32, y2: f32) -> Vec<u32> {
        let scene = self.build_scene_dto();
        let a = PointF::new(x1, y1);
        let b = PointF::new(x2, y2);
        let mut hit = Vec::new();
        for c in &scene.connections {
            let pts = sample_cable_points(
                PointF::new(c.x1, c.y1),
                PointF::new(c.x2, c.y2),
                self.use_bezier,
                20,
            );
            if pts.windows(2).any(|w| segments_intersect(a, b, w[0], w[1])) {
                hit.push(c.connection_id);
            }
        }
        hit
    }

    pub fn selected_plugin_ids(&self) -> Vec<u32> {
        let mut pids: Vec<u32> = self
            .selected_boxes
            .iter()
            .filter_map(|k| self.groups.get(&k.group_id))
            .filter(|g| g.plugin_id >= 0)
            .map(|g| g.plugin_id as u32)
            .collect();
        pids.sort_unstable();
        pids.dedup();
        pids
    }

    pub fn render_to_rgba(&self, scale: f32) -> (u32, u32, Vec<u8>) {
        let scene = self.build_scene_dto();
        let bounds = if let Some(first) = scene.boxes.first() {
            let mut r = first.rect;
            for b in &scene.boxes[1..] {
                r = r.union(&b.rect);
            }
            r.expand(48.0)
        } else {
            RectF::new(0.0, 0.0, 640.0, 480.0)
        };

        let scale = scale.clamp(0.5, 4.0);
        let w = ((bounds.w * scale).ceil() as u32).clamp(64, 8192);
        let h = ((bounds.h * scale).ceil() as u32).clamp(64, 8192);
        let mut img: ImageBuffer<Rgba<u8>, Vec<u8>> =
            ImageBuffer::from_pixel(w, h, Rgba([27, 30, 32, 255]));

        let to_px = |p: PointF| -> (f32, f32) {
            ((p.x - bounds.x) * scale, (p.y - bounds.y) * scale)
        };

        let draw_rect = |img: &mut ImageBuffer<Rgba<u8>, Vec<u8>>, r: RectF, rgb: [u8; 3]| {
            let (x0, y0) = to_px(PointF::new(r.x, r.y));
            let (x1, y1) = to_px(PointF::new(r.right(), r.bottom()));
            let ix0 = (x0.floor() as i32).max(0) as u32;
            let iy0 = (y0.floor() as i32).max(0) as u32;
            let ix1 = (x1.ceil() as i32).min(w as i32) as u32;
            let iy1 = (y1.ceil() as i32).min(h as i32) as u32;
            let px = Rgba([rgb[0], rgb[1], rgb[2], 255]);
            for y in iy0..iy1 {
                for x in ix0..ix1 {
                    img.put_pixel(x, y, px);
                }
            }
        };

        let draw_seg = |img: &mut ImageBuffer<Rgba<u8>, Vec<u8>>, a: PointF, b: PointF, rgb: [u8; 3]| {
            let (x0, y0) = to_px(a);
            let (x1, y1) = to_px(b);
            let steps = ((x1 - x0).hypot(y1 - y0).ceil() as usize).max(1);
            let px = Rgba([rgb[0], rgb[1], rgb[2], 255]);
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

        for b in &scene.boxes {
            draw_rect(&mut img, b.rect, [42, 46, 50]);
            draw_rect(
                &mut img,
                RectF::new(b.rect.x, b.rect.y, b.rect.w, HEADER_HEIGHT),
                [56, 61, 68],
            );
            for p in b.in_ports.iter().chain(b.out_ports.iter()) {
                let rgb = self
                    .ports
                    .iter()
                    .find(|pp| pp.group_id == p.group_id && pp.port_id == p.port_id)
                    .map_or([78, 122, 168], |pp| pp.color_rgb());
                draw_rect(&mut img, p.rect, rgb);
            }
        }

        for c in &scene.connections {
            let rgb = self
                .ports
                .iter()
                .find(|p| p.group_id == c.group_out && p.port_id == c.port_out)
                .map_or([96, 165, 250], |p| p.cable_rgb());
            let pts = sample_cable_points(
                PointF::new(c.x1, c.y1),
                PointF::new(c.x2, c.y2),
                self.use_bezier,
                28,
            );
            for pair in pts.windows(2) {
                draw_seg(&mut img, pair[0], pair[1], rgb);
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

pub fn sample_cable_points(p1: PointF, p2: PointF, use_bezier: bool, steps: usize) -> Vec<PointF> {
    if !use_bezier {
        return vec![p1, p2];
    }
    let dx = (p2.x - p1.x).abs();
    let offset = (dx * 0.5).clamp(36.0, 240.0);
    let c1 = PointF::new(p1.x + offset, p1.y);
    let c2 = PointF::new(p2.x - offset, p2.y);

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
        pts.push(PointF::new(x, y));
    }
    pts
}

pub fn segments_intersect(a: PointF, b: PointF, c: PointF, d: PointF) -> bool {
    fn orient(p: PointF, q: PointF, r: PointF) -> f32 {
        (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x)
    }
    let o1 = orient(a, b, c);
    let o2 = orient(a, b, d);
    let o3 = orient(c, d, a);
    let o4 = orient(c, d, b);
    (o1 * o2 < 0.0) && (o3 * o4 < 0.0)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn clear_before_refresh_prevents_duplicate_shifted_ports() {
        let mut pb = PatchbayState::default();
        // Initial population (e.g. during engine_init)
        pb.add_group(14, -1, 0, "Virtual Mic".into());
        pb.add_port(14, 45, PATCHBAY_PORT_IS_INPUT | PATCHBAY_PORT_TYPE_AUDIO, "input_FL".into());
        pb.add_port(14, 46, PATCHBAY_PORT_IS_INPUT | PATCHBAY_PORT_TYPE_AUDIO, "input_FR".into());
        pb.add_port(14, 47, PATCHBAY_PORT_TYPE_AUDIO, "capture_FL".into());
        pb.add_port(14, 48, PATCHBAY_PORT_TYPE_AUDIO, "capture_FR".into());

        // Refresh clears state before backend re-emits with shifted port IDs
        pb.clear();
        pb.add_group(14, -1, 0, "Virtual Mic".into());
        pb.add_port(14, 47, PATCHBAY_PORT_IS_INPUT | PATCHBAY_PORT_TYPE_AUDIO, "input_FL".into());
        pb.add_port(14, 48, PATCHBAY_PORT_IS_INPUT | PATCHBAY_PORT_TYPE_AUDIO, "input_FR".into());
        pb.add_port(14, 49, PATCHBAY_PORT_TYPE_AUDIO, "capture_FL".into());
        pb.add_port(14, 50, PATCHBAY_PORT_TYPE_AUDIO, "capture_FR".into());

        let scene = pb.build_scene_dto();
        assert_eq!(scene.boxes.len(), 1);
        let vm = &scene.boxes[0];
        assert_eq!(vm.in_ports.len(), 2);
        assert_eq!(vm.out_ports.len(), 2);
        assert_eq!(vm.in_ports[0].port_id, 47);
        assert_eq!(vm.in_ports[1].port_id, 48);
        assert_eq!(vm.out_ports[0].port_id, 49);
        assert_eq!(vm.out_ports[1].port_id, 50);
    }

    #[test]
    fn re_adding_existing_group_preserves_custom_position() {
        let mut pb = PatchbayState::default();
        pb.add_group(5, -1, 0, "Synth".into());
        pb.set_group_position(5, 420, 310, 0, 0);
        pb.add_group(5, 2, 1, "Synth Renamed".into());
        let g = pb.groups.get(&5).unwrap();
        assert_eq!(g.name, "Synth Renamed");
        assert_eq!(g.plugin_id, 2);
        assert_eq!(g.pos1.x, 420.0);
        assert_eq!(g.pos1.y, 310.0);
        assert!(g.has_custom_pos);
    }

    #[test]
    fn wide_title_does_not_stretch_port_tags() {
        let mut pb = PatchbayState::default();
        pb.add_group(1, -1, 0, "Split SSL 2+ Line Outputs 1/L + 2/R".into());
        pb.add_port(1, 1, PATCHBAY_PORT_TYPE_AUDIO, "output_AUX0".into());
        pb.add_port(1, 2, PATCHBAY_PORT_TYPE_AUDIO, "output_AUX1".into());
        let scene = pb.build_scene_dto();
        let b = &scene.boxes[0];
        assert!(b.rect.w > 240.0);
        for p in &b.out_ports {
            assert!(p.rect.w < 110.0, "port tag too wide: {}", p.rect.w);
            assert!((p.rect.right() - b.rect.right()).abs() < 0.01);
        }
    }
}
