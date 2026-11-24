// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use crate::ffi::types::*;

pub const FRANCA_CATEGORY_PROPERTY: &str = "FrancaCategory";

#[derive(Clone, Copy, Debug)]
pub struct CategoryDef {
    pub id: i32,
    pub key: &'static str,
    pub label: &'static str,
    pub hex_color: &'static str, // empty string for None
}

pub const FRANCA_PLUGIN_CATEGORIES: &[CategoryDef] = &[
    CategoryDef {
        id: PLUGIN_CATEGORY_SYNTH,
        key: "synth",
        label: "Synth",
        hex_color: "#8B5CF6",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_DELAY,
        key: "delay",
        label: "Delay",
        hex_color: "#06B6D4",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_EQ,
        key: "eq",
        label: "EQ",
        hex_color: "#10B981",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_FILTER,
        key: "filter",
        label: "Filter",
        hex_color: "#3B82F6",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_DISTORTION,
        key: "distortion",
        label: "Distortion",
        hex_color: "#F97316",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_DYNAMICS,
        key: "dynamics",
        label: "Dynamics",
        hex_color: "#EAB308",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_MODULATOR,
        key: "modulator",
        label: "Modulator",
        hex_color: "#EC4899",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_UTILITY,
        key: "utility",
        label: "Utility",
        hex_color: "#64748B",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_OTHER,
        key: "other",
        label: "Other",
        hex_color: "#14B8A6",
    },
    CategoryDef {
        id: PLUGIN_CATEGORY_NONE,
        key: "none",
        label: "None",
        hex_color: "",
    },
];

pub fn category_def(id: i32) -> &'static CategoryDef {
    FRANCA_PLUGIN_CATEGORIES
        .iter()
        .find(|c| c.id == id)
        .unwrap_or(&FRANCA_PLUGIN_CATEGORIES[FRANCA_PLUGIN_CATEGORIES.len() - 1])
}

pub fn category_hex(id: i32) -> &'static str {
    category_def(id).hex_color
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
