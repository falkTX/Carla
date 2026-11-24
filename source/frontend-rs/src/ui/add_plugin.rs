// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::HashSet;

use eframe::egui;

use crate::discovery::{DiscoveryOptions, DiscoveryRunner, PluginDatabase};
use crate::ffi::types::*;
use crate::ffi::utils::{CarlaUtils, DiscoveredPlugin};
use crate::widgets::category::{FRANCA_PLUGIN_CATEGORIES, category_label};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SortColumn {
    Name,
    Format,
    Category,
    Maker,
}

pub struct AddPluginDialogState {
    pub open: bool,
    pub replace_plugin_id: Option<u32>,
    pub search: String,
    pub favorites_only: bool,

    // Format filters
    pub fmt_internal: bool,
    pub fmt_ladspa: bool,
    pub fmt_dssi: bool,
    pub fmt_lv2: bool,
    pub fmt_vst2: bool,
    pub fmt_vst3: bool,
    pub fmt_clap: bool,
    pub fmt_sf2: bool,
    pub fmt_sfz: bool,
    pub fmt_jsfx: bool,

    // Type filters
    pub type_effect: bool,
    pub type_instrument: bool,
    pub type_midi: bool,
    pub type_other: bool,

    // Category filter (None = All)
    pub category_filter: Option<i32>,

    pub sort_col: SortColumn,
    pub sort_asc: bool,
    pub selected_key: Option<String>,

    // Refresh options & runner
    pub show_refresh_opts: bool,
    pub refresh_opts: DiscoveryOptions,
    pub runner: Option<DiscoveryRunner>,
}

impl Default for AddPluginDialogState {
    fn default() -> Self {
        Self {
            open: false,
            replace_plugin_id: None,
            search: String::new(),
            favorites_only: false,
            fmt_internal: true,
            fmt_ladspa: true,
            fmt_dssi: true,
            fmt_lv2: true,
            fmt_vst2: true,
            fmt_vst3: true,
            fmt_clap: true,
            fmt_sf2: true,
            fmt_sfz: true,
            fmt_jsfx: true,
            type_effect: true,
            type_instrument: true,
            type_midi: true,
            type_other: true,
            category_filter: None,
            sort_col: SortColumn::Name,
            sort_asc: true,
            selected_key: None,
            show_refresh_opts: false,
            refresh_opts: DiscoveryOptions::default(),
            runner: None,
        }
    }
}

#[derive(Clone, Debug)]
pub enum AddPluginAction {
    AddPlugin(DiscoveredPlugin),
    ReplacePlugin(u32, DiscoveredPlugin),
    ToggleFavorite(String),
    StartRefresh(DiscoveryOptions),
}

impl AddPluginDialogState {
    pub fn open_for_add(&mut self) {
        self.open = true;
        self.replace_plugin_id = None;
    }

    pub fn open_for_replace(&mut self, plugin_id: u32) {
        self.open = true;
        self.replace_plugin_id = Some(plugin_id);
    }

    pub fn matches_filters(&self, p: &DiscoveredPlugin, favorites: &HashSet<String>) -> bool {
        if self.favorites_only && !favorites.contains(&p.unique_key()) {
            return false;
        }
        let fmt_ok = match p.ptype {
            PLUGIN_INTERNAL => self.fmt_internal,
            PLUGIN_LADSPA => self.fmt_ladspa,
            PLUGIN_DSSI => self.fmt_dssi,
            PLUGIN_LV2 => self.fmt_lv2,
            PLUGIN_VST2 => self.fmt_vst2,
            PLUGIN_VST3 => self.fmt_vst3,
            PLUGIN_CLAP => self.fmt_clap,
            PLUGIN_SF2 => self.fmt_sf2,
            PLUGIN_SFZ => self.fmt_sfz,
            PLUGIN_JSFX => self.fmt_jsfx,
            _ => true,
        };
        if !fmt_ok {
            return false;
        }
        if let Some(cat) = self.category_filter {
            if p.category != cat {
                return false;
            }
        }
        let type_ok = if p.is_synth() {
            self.type_instrument
        } else if p.is_midi_only() {
            self.type_midi
        } else if p.is_effect() {
            self.type_effect
        } else {
            self.type_other
        };
        if !type_ok {
            return false;
        }
        let q = self.search.trim().to_ascii_lowercase();
        if !q.is_empty() {
            let hay = format!(
                "{} {} {} {}",
                p.name.to_ascii_lowercase(),
                p.maker.to_ascii_lowercase(),
                p.label.to_ascii_lowercase(),
                plugin_type_label(p.ptype).to_ascii_lowercase()
            );
            for term in q.split_whitespace() {
                if !hay.contains(term) {
                    return false;
                }
            }
        }
        true
    }
}

pub fn show_add_plugin_dialog(
    ctx: &egui::Context,
    state: &mut AddPluginDialogState,
    db: &mut PluginDatabase,
    favorites: &HashSet<String>,
    utils: Option<&CarlaUtils>,
) -> Vec<AddPluginAction> {
    let mut actions = Vec::new();

    // Step active discovery runner every frame
    if let (Some(runner), Some(utils)) = (state.runner.as_mut(), utils) {
        if !runner.finished {
            for _ in 0..8 {
                if runner.step(utils) {
                    break;
                }
            }
            ctx.request_repaint();
        }
        if runner.finished && !runner.cancelled {
            runner.apply_to_database(db);
            state.runner = None;
        } else if runner.cancelled {
            state.runner = None;
        }
    }

    if !state.open {
        return actions;
    }

    let title = if state.replace_plugin_id.is_some() {
        "Replace Plugin"
    } else {
        "Add Plugin"
    };
    let mut open = state.open;
    let mut close_now = false;

    egui::Window::new(title)
        .id(egui::Id::unique("franca_add_plugin_dialog"))
        .open(&mut open)
        .default_size([860.0, 580.0])
        .resizable(true)
        .show(ctx, |ui| {
            // Search & action bar
            ui.horizontal(|ui| {
                ui.label("Search:");
                ui.add(
                    egui::TextEdit::singleline(&mut state.search)
                        .hint_text("Filter by name, maker, format...")
                        .desired_width(280.0),
                );
                if !state.search.is_empty() && ui.small_button("✕").clicked() {
                    state.search.clear();
                }
                ui.checkbox(&mut state.favorites_only, "★ Favorites");

                ui.label("Category:");
                let cat_lbl = state
                    .category_filter
                    .map(category_label)
                    .unwrap_or("All");
                egui::ComboBox::from_id_salt("add_plugin_cat_filter")
                    .selected_text(cat_lbl)
                    .show_ui(ui, |ui| {
                        ui.selectable_value(&mut state.category_filter, None, "All");
                        for c in FRANCA_PLUGIN_CATEGORIES {
                            ui.selectable_value(&mut state.category_filter, Some(c.id), c.label);
                        }
                    });

                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    if ui
                        .add_enabled(state.runner.is_none(), egui::Button::new("↻ Refresh..."))
                        .clicked()
                    {
                        state.show_refresh_opts = true;
                    }
                });
            });

            // Format & Type checkboxes
            ui.horizontal_wrapped(|ui| {
                ui.label(egui::RichText::new("Format:").weak().small());
                ui.checkbox(&mut state.fmt_internal, "Internal");
                ui.checkbox(&mut state.fmt_ladspa, "LADSPA");
                ui.checkbox(&mut state.fmt_dssi, "DSSI");
                ui.checkbox(&mut state.fmt_lv2, "LV2");
                ui.checkbox(&mut state.fmt_vst2, "VST2");
                ui.checkbox(&mut state.fmt_vst3, "VST3");
                ui.checkbox(&mut state.fmt_clap, "CLAP");
                ui.checkbox(&mut state.fmt_sf2, "SF2");
                ui.checkbox(&mut state.fmt_sfz, "SFZ");
                ui.checkbox(&mut state.fmt_jsfx, "JSFX");
            });

            ui.horizontal_wrapped(|ui| {
                ui.label(egui::RichText::new("Type:").weak().small());
                ui.checkbox(&mut state.type_effect, "Effect");
                ui.checkbox(&mut state.type_instrument, "Instrument");
                ui.checkbox(&mut state.type_midi, "MIDI");
                ui.checkbox(&mut state.type_other, "Other");
            });

            // Live discovery progress banner
            if let Some(runner) = state.runner.as_mut() {
                ui.separator();
                ui.horizontal(|ui| {
                    ui.add(
                        egui::ProgressBar::new(runner.progress())
                            .desired_width(240.0)
                            .show_percentage(),
                    );
                    ui.label(format!(
                        "{} ({} found)",
                        runner.status_text(),
                        runner.discovered_count
                    ));
                    if let Some(u) = utils {
                        if ui.small_button("Skip").clicked() {
                            runner.skip_current(u);
                        }
                        if ui.small_button("Cancel").clicked() {
                            runner.cancel(u);
                        }
                    }
                });
            }

            ui.separator();

            // Filtered & sorted indices
            let mut filtered: Vec<usize> = db
                .plugins
                .iter()
                .enumerate()
                .filter(|(_, p)| state.matches_filters(p, favorites))
                .map(|(i, _)| i)
                .collect();

            filtered.sort_by(|&a, &b| {
                let pa = &db.plugins[a];
                let pb = &db.plugins[b];
                let ord = match state.sort_col {
                    SortColumn::Name => pa.name.to_lowercase().cmp(&pb.name.to_lowercase()),
                    SortColumn::Format => plugin_type_label(pa.ptype).cmp(plugin_type_label(pb.ptype)),
                    SortColumn::Category => {
                        category_label(pa.category).cmp(category_label(pb.category))
                    }
                    SortColumn::Maker => pa.maker.to_lowercase().cmp(&pb.maker.to_lowercase()),
                };
                if state.sort_asc { ord } else { ord.reverse() }
            });

            // Header row with sort buttons
            ui.horizontal(|ui| {
                ui.add_space(24.0);
                let mut hdr_btn = |col: SortColumn, text: &str, w: f32| {
                    let arrow = if state.sort_col == col {
                        if state.sort_asc { " ▲" } else { " ▼" }
                    } else {
                        ""
                    };
                    if ui
                        .add_sized([w, 20.0], egui::Button::new(format!("{text}{arrow}")).frame(false))
                        .clicked()
                    {
                        if state.sort_col == col {
                            state.sort_asc = !state.sort_asc;
                        } else {
                            state.sort_col = col;
                            state.sort_asc = true;
                        }
                    }
                };
                hdr_btn(SortColumn::Name, "Name", 240.0);
                hdr_btn(SortColumn::Format, "Format", 72.0);
                hdr_btn(SortColumn::Category, "Category", 96.0);
                hdr_btn(SortColumn::Maker, "Maker", 160.0);
                ui.label(egui::RichText::new("I/O").weak());
            });
            ui.separator();

            // Virtualized list of filtered plugins
            let list_h = (ui.available_height() - 125.0).max(160.0);
            let mut commit_plugin: Option<DiscoveredPlugin> = None;

            egui::ScrollArea::vertical()
                .id_salt("add_plugin_list_scroll")
                .max_height(list_h)
                .auto_shrink([false, false])
                .show_rows(ui, 22.0, filtered.len(), |ui, row_range| {
                    for row_idx in row_range {
                        let p = &db.plugins[filtered[row_idx]];
                        let key = p.unique_key();
                        let is_fav = favorites.contains(&key);
                        let is_sel = state.selected_key.as_deref() == Some(key.as_str());

                        ui.horizontal(|ui| {
                            if ui
                                .small_button(if is_fav { "★" } else { "☆" })
                                .on_hover_text("Toggle favorite")
                                .clicked()
                            {
                                actions.push(AddPluginAction::ToggleFavorite(key.clone()));
                            }

                            let row_text = format!(
                                "{:<34} {:<8} {:<11} {:<20} A:{}/{} M:{}/{}",
                                truncate_str(&p.name, 33),
                                plugin_type_label(p.ptype),
                                category_label(p.category),
                                truncate_str(&p.maker, 19),
                                p.audio_ins,
                                p.audio_outs,
                                p.midi_ins,
                                p.midi_outs
                            );
                            let resp = ui.selectable_label(
                                is_sel,
                                egui::RichText::new(row_text).monospace().size(11.5),
                            );
                            if resp.clicked() {
                                state.selected_key = Some(key);
                            }
                            if resp.double_clicked() {
                                commit_plugin = Some(p.clone());
                            }
                        });
                    }
                });

            ui.separator();

            // Selected plugin details + action buttons
            let selected_plugin = state
                .selected_key
                .as_ref()
                .and_then(|k| db.plugins.iter().find(|p| p.unique_key() == *k))
                .cloned();

            if let Some(ref p) = selected_plugin {
                ui.horizontal(|ui| {
                    ui.vertical(|ui| {
                        ui.label(
                            egui::RichText::new(format!(
                                "{}  [{} · {}]",
                                p.name,
                                plugin_type_label(p.ptype),
                                binary_type_label(p.build)
                            ))
                            .strong(),
                        );
                        ui.label(
                            egui::RichText::new(format!(
                                "Maker: {}  |  Category: {}  |  Audio: {}/{}  MIDI: {}/{}  Params: {}/{}",
                                if p.maker.is_empty() { "Unknown" } else { &p.maker },
                                category_label(p.category),
                                p.audio_ins,
                                p.audio_outs,
                                p.midi_ins,
                                p.midi_outs,
                                p.parameter_ins,
                                p.parameter_outs
                            ))
                            .small(),
                        );
                        if !p.filename.is_empty() {
                            ui.label(egui::RichText::new(&p.filename).small().weak());
                        }
                    });
                });
            } else {
                ui.label(
                    egui::RichText::new(format!(
                        "Showing {} of {} plugins. Select a plugin or click Refresh to scan your system.",
                        filtered.len(),
                        db.plugins.len()
                    ))
                    .weak(),
                );
            }

            ui.add_space(6.0);
            ui.horizontal(|ui| {
                ui.label(egui::RichText::new(format!("{} matching plugins", filtered.len())).small().weak());
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    if ui.button("Cancel").clicked() {
                        close_now = true;
                    }
                    let btn_lbl = if state.replace_plugin_id.is_some() {
                        "Replace Plugin"
                    } else {
                        "Add Plugin"
                    };
                    if ui
                        .add_enabled(selected_plugin.is_some(), egui::Button::new(btn_lbl))
                        .clicked()
                    {
                        commit_plugin = selected_plugin.clone();
                    }
                });
            });

            if let Some(plugin) = commit_plugin {
                if let Some(rid) = state.replace_plugin_id {
                    actions.push(AddPluginAction::ReplacePlugin(rid, plugin));
                } else {
                    actions.push(AddPluginAction::AddPlugin(plugin));
                }
                close_now = true;
            }
        });

    state.open = open && !close_now;

    // Refresh options sub-modal
    if state.show_refresh_opts {
        let mut ropen = true;
        egui::Window::new("Refresh Plugins")
            .id(egui::Id::unique("franca_refresh_plugins_modal"))
            .collapsible(false)
            .resizable(false)
            .open(&mut ropen)
            .show(ctx, |ui| {
                ui.label("Select plugin formats to scan:");
                ui.horizontal_wrapped(|ui| {
                    ui.checkbox(&mut state.refresh_opts.check_internal, "Internal");
                    ui.checkbox(&mut state.refresh_opts.check_ladspa, "LADSPA");
                    ui.checkbox(&mut state.refresh_opts.check_dssi, "DSSI");
                    ui.checkbox(&mut state.refresh_opts.check_lv2, "LV2");
                    ui.checkbox(&mut state.refresh_opts.check_vst2, "VST2");
                    ui.checkbox(&mut state.refresh_opts.check_vst3, "VST3");
                    ui.checkbox(&mut state.refresh_opts.check_clap, "CLAP");
                    ui.checkbox(&mut state.refresh_opts.check_sf2, "SF2");
                    ui.checkbox(&mut state.refresh_opts.check_sfz, "SFZ");
                    ui.checkbox(&mut state.refresh_opts.check_jsfx, "JSFX");
                });
                ui.checkbox(
                    &mut state.refresh_opts.check_invalid,
                    "Ignore cache and rescan all files",
                );
                ui.separator();
                ui.horizontal(|ui| {
                    if ui.button("Start Scan").clicked() {
                        actions.push(AddPluginAction::StartRefresh(state.refresh_opts.clone()));
                        state.show_refresh_opts = false;
                    }
                    if ui.button("Cancel").clicked() {
                        state.show_refresh_opts = false;
                    }
                });
            });
        if !ropen {
            state.show_refresh_opts = false;
        }
    }

    actions
}

fn truncate_str(s: &str, max_chars: usize) -> String {
    if s.chars().count() <= max_chars {
        s.to_string()
    } else {
        let mut out: String = s.chars().take(max_chars.saturating_sub(1)).collect();
        out.push('…');
        out
    }
}
