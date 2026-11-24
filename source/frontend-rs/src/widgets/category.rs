// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use eframe::egui::{self, Color32, CornerRadius, Rect, Stroke, Vec2};

use crate::ffi::types::*;

pub const FRANCA_CATEGORY_PROPERTY: &str = "FrancaCategory";

#[derive(Clone, Copy, Debug)]
pub struct CategoryDef {
    pub id: i32,
    pub key: &'static str,
    pub label: &'static str,
    pub color: Option<Color32>,
}

pub const FRANCA_PLUGIN_CATEGORIES: &[CategoryDef] = &[
    CategoryDef {
        id: PLUGIN_CATEGORY_SYNTH,
        key: "synth",
        label: "Synth",
        color: Some(Color32::from_rgb(0x8B, 0x5C, 0xF6)), // #8B5CF6
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_DELAY,
        key: "delay",
        label: "Delay",
        color: Some(Color32::from_rgb(0x06, 0xB6, 0xD4)), // #06B6D4
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_EQ,
        key: "eq",
        label: "EQ",
        color: Some(Color32::from_rgb(0x10, 0xB9, 0x81)), // #10B981
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_FILTER,
        key: "filter",
        label: "Filter",
        color: Some(Color32::from_rgb(0x3B, 0x82, 0xF6)), // #3B82F6
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_DISTORTION,
        key: "distortion",
        label: "Distortion",
        color: Some(Color32::from_rgb(0xF9, 0x73, 0x16)), // #F97316
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_DYNAMICS,
        key: "dynamics",
        label: "Dynamics",
        color: Some(Color32::from_rgb(0xEA, 0xB3, 0x08)), // #EAB308
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_MODULATOR,
        key: "modulator",
        label: "Modulator",
        color: Some(Color32::from_rgb(0xEC, 0x48, 0x99)), // #EC4899
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_UTILITY,
        key: "utility",
        label: "Utility",
        color: Some(Color32::from_rgb(0x64, 0x74, 0x8B)), // #64748B
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_OTHER,
        key: "other",
        label: "Other",
        color: Some(Color32::from_rgb(0x14, 0xB8, 0xA6)), // #14B8A6
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_NONE,
        key: "none",
        label: "None",
        color: None,
    },
];

pub fn category_def(id: i32) -> &'static CategoryDef {
    FRANCA_PLUGIN_CATEGORIES
        .iter()
        .find(|c| c.id == id)
        .unwrap_or(&FRANCA_PLUGIN_CATEGORIES[FRANCA_PLUGIN_CATEGORIES.len() - 1])
}

pub fn category_color(id: i32) -> Option<Color32> {
    category_def(id).color
}

pub fn category_key(id: i32) -> &'static str {
    category_def(id).key
}

pub fn category_label(id: i32) -> &'static str {
    category_def(id).label
}

pub fn parse_category_key(raw: &str) -> Option<i32> {
    let key = raw.trim().to_ascii_lowercase();
    if key.is_empty() {
        return None;
    }
    FRANCA_PLUGIN_CATEGORIES
        .iter()
        .find(|c| c.key == key)
        .map(|c| c.id)
}

pub fn draw_category_swatch(ui: &mut egui::Ui, color: Option<Color32>) {
    let (rect, _) = ui.allocate_exact_size(Vec2::splat(12.0), egui::Sense::hover());
    let painter = ui.painter();
    match color {
        Some(c) => {
            painter.rect_filled(rect.shrink(1.0), CornerRadius::same(2), c);
            painter.rect_stroke(
                rect.shrink(1.0),
                CornerRadius::same(2),
                Stroke::new(1.0, Color32::from_rgba_unmultiplied(0, 0, 0, 110)),
                egui::StrokeKind::Inside,
            );
        }
        None => {
            let r = rect.shrink(1.0);
            painter.rect_stroke(
                r,
                CornerRadius::same(2),
                Stroke::new(1.0, Color32::from_gray(130)),
                egui::StrokeKind::Inside,
            );
            painter.line_segment(
                [r.left_bottom(), r.right_top()],
                Stroke::new(1.0, Color32::from_gray(130)),
            );
        }
    }
}

pub fn category_tint(color: Color32) -> Color32 {
    Color32::from_rgba_unmultiplied(color.r(), color.g(), color.b(), 26)
}

pub fn draw_category_accent_bg(
    painter: &egui::Painter,
    row_rect: Rect,
    color: Option<Color32>,
    selected: bool,
) {
    let Some(c) = color else { return };
    if !selected {
        painter.rect_filled(row_rect, CornerRadius::ZERO, category_tint(c));
    }
    let bar = Rect::from_min_max(
        row_rect.min,
        egui::pos2(row_rect.min.x + 5.0, row_rect.max.y),
    );
    painter.rect_filled(bar, CornerRadius::ZERO, c);
}
