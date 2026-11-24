// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::BTreeMap;

use eframe::egui;

use crate::ffi::host::{CarlaHost, MidiProgramInfo, ParameterInfo, PluginInfo, ScalePointInfo};
use crate::ffi::types::*;
use crate::widgets::keyboard::{NoteEvent, PianoKeyboardState};

pub const MIDI_CC_ITEMS: &[(i16, &str)] = &[
    (CONTROL_INDEX_NONE, "None"),
    (CONTROL_INDEX_MIDI_LEARN, "MIDI Learn..."),
    (CONTROL_INDEX_MIDI_PITCHBEND, "Pitchbend"),
    (CONTROL_INDEX_CV, "CV"),
    (1, "01 [0x01] Modulation"),
    (2, "02 [0x02] Breath"),
    (4, "04 [0x04] Foot"),
    (5, "05 [0x05] Portamento"),
    (7, "07 [0x07] Volume"),
    (8, "08 [0x08] Balance"),
    (10, "10 [0x0A] Pan"),
    (11, "11 [0x0B] Expression"),
    (12, "12 [0x0C] FX Control 1"),
    (13, "13 [0x0D] FX Control 2"),
    (16, "16 [0x10] General Purpose 1"),
    (17, "17 [0x11] General Purpose 2"),
    (18, "18 [0x12] General Purpose 3"),
    (19, "19 [0x13] General Purpose 4"),
    (70, "70 [0x46] Control 1 [Variation]"),
    (71, "71 [0x47] Control 2 [Timbre]"),
    (72, "72 [0x48] Control 3 [Release]"),
    (73, "73 [0x49] Control 4 [Attack]"),
    (74, "74 [0x4A] Control 5 [Brightness]"),
    (75, "75 [0x4B] Control 6 [Decay]"),
    (76, "76 [0x4C] Control 7 [Vib Rate]"),
    (77, "77 [0x4D] Control 8 [Vib Depth]"),
    (78, "78 [0x4E] Control 9 [Vib Delay]"),
    (79, "79 [0x4F] Control 10"),
    (80, "80 [0x50] General Purpose 5"),
    (81, "81 [0x51] General Purpose 6"),
    (82, "82 [0x52] General Purpose 7"),
    (83, "83 [0x53] General Purpose 8"),
    (84, "84 [0x54] Portamento Control"),
    (91, "91 [0x5B] FX 1 Depth [Reverb]"),
    (92, "92 [0x5C] FX 2 Depth [Tremolo]"),
    (93, "93 [0x5D] FX 3 Depth [Chorus]"),
    (94, "94 [0x5E] FX 4 Depth [Detune]"),
    (95, "95 [0x5F] FX 5 Depth [Phaser]"),
];

#[derive(Clone, Debug)]
pub struct ParamItem {
    pub index: u32,
    pub info: ParameterInfo,
    pub data: ParameterData,
    pub ranges: ParameterRanges,
    pub scale_points: Vec<ScalePointInfo>,
    pub value: f32,
    pub custom_text: String,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
pub enum EditTab {
    Info,
    #[default]
    Parameters,
    Programs,
    Keyboard,
}

#[derive(Clone, Debug, Default)]
pub struct PluginEditState {
    pub active_tab: EditTab,
    pub param_search: String,
    pub drywet: f32,
    pub volume: f32,
    pub balance_left: f32,
    pub balance_right: f32,
    pub panning: f32,
    pub ctrl_channel: i8,
    pub audio_ports: CarlaPortCountInfo,
    pub midi_ports: CarlaPortCountInfo,
    pub param_counts: CarlaPortCountInfo,
    pub latency: u32,
    pub params: Vec<ParamItem>,
    pub programs: Vec<String>,
    pub current_program: i32,
    pub midi_programs: Vec<MidiProgramInfo>,
    pub current_midi_program: i32,
    pub keyboard: PianoKeyboardState,
    pub midi_cc_popup_param: Option<u32>,
}

#[derive(Clone, Debug)]
pub enum PluginEditAction {
    SetActive(u32, bool),
    SetInternalParam(u32, i32, f32),
    SetCtrlChannel(u32, i8),
    SetOption(u32, u32, bool),
    SetParam(u32, u32, f32),
    SetParamMidiChannel(u32, u32, u8),
    SetParamMappedControl(u32, u32, i16),
    SetParamMappedRange(u32, u32, f32, f32),
    SetProgram(u32, u32),
    SetMidiProgram(u32, u32),
    ResetParameters(u32),
    RandomizeParameters(u32),
    LoadState(u32),
    SaveState(u32),
    ReloadAll(u32),
    SendNote(u32, NoteEvent),
}

impl PluginEditState {
    pub fn reload_all(&mut self, host: &CarlaHost, plugin_id: u32) {
        self.drywet = host.get_internal_parameter_value(plugin_id, PARAMETER_DRYWET);
        self.volume = host.get_internal_parameter_value(plugin_id, PARAMETER_VOLUME);
        self.balance_left = host.get_internal_parameter_value(plugin_id, PARAMETER_BALANCE_LEFT);
        self.balance_right = host.get_internal_parameter_value(plugin_id, PARAMETER_BALANCE_RIGHT);
        self.panning = host.get_internal_parameter_value(plugin_id, PARAMETER_PANNING);
        self.ctrl_channel =
            host.get_internal_parameter_value(plugin_id, PARAMETER_CTRL_CHANNEL) as i8;

        self.audio_ports = host.get_audio_port_count_info(plugin_id);
        self.midi_ports = host.get_midi_port_count_info(plugin_id);
        self.param_counts = host.get_parameter_count_info(plugin_id);
        self.latency = host.get_plugin_latency(plugin_id);

        self.reload_parameters(host, plugin_id);
        self.reload_programs(host, plugin_id);
    }

    pub fn reload_parameters(&mut self, host: &CarlaHost, plugin_id: u32) {
        self.params.clear();
        let count = host.get_parameter_count(plugin_id).min(512);
        for i in 0..count {
            let info = host.get_parameter_info(plugin_id, i);
            let data = host.get_parameter_data(plugin_id, i);
            let ranges = host.get_parameter_ranges(plugin_id, i);
            let value = host.get_current_parameter_value(plugin_id, i);
            let mut scale_points = Vec::with_capacity(info.scale_point_count as usize);
            for sp in 0..info.scale_point_count {
                scale_points.push(host.get_parameter_scalepoint_info(plugin_id, i, sp));
            }
            let custom_text = if (data.hints & PARAMETER_USES_CUSTOM_TEXT) != 0 {
                host.get_parameter_text(plugin_id, i)
            } else {
                String::new()
            };
            self.params.push(ParamItem {
                index: i,
                info,
                data,
                ranges,
                scale_points,
                value,
                custom_text,
            });
        }
    }

    pub fn reload_programs(&mut self, host: &CarlaHost, plugin_id: u32) {
        self.programs.clear();
        let pcount = host.get_program_count(plugin_id);
        for i in 0..pcount {
            self.programs.push(host.get_program_name(plugin_id, i));
        }
        self.current_program = host.get_current_program_index(plugin_id);

        self.midi_programs.clear();
        let mcount = host.get_midi_program_count(plugin_id);
        for i in 0..mcount {
            self.midi_programs
                .push(host.get_midi_program_data(plugin_id, i));
        }
        self.current_midi_program = host.get_current_midi_program_index(plugin_id);
    }

    pub fn refresh_live_values(&mut self, host: &CarlaHost, plugin_id: u32) {
        for p in &mut self.params {
            if p.data.ptype == PARAMETER_OUTPUT {
                p.value = host.get_current_parameter_value(plugin_id, p.index);
                if (p.data.hints & PARAMETER_USES_CUSTOM_TEXT) != 0 {
                    p.custom_text = host.get_parameter_text(plugin_id, p.index);
                }
            }
        }
    }

    pub fn set_parameter_value_cached(&mut self, param_id: i32, value: f32, host: &CarlaHost, plugin_id: u32) {
        match param_id {
            PARAMETER_DRYWET => self.drywet = value,
            PARAMETER_VOLUME => self.volume = value,
            PARAMETER_BALANCE_LEFT => self.balance_left = value,
            PARAMETER_BALANCE_RIGHT => self.balance_right = value,
            PARAMETER_PANNING => self.panning = value,
            PARAMETER_CTRL_CHANNEL => self.ctrl_channel = value as i8,
            idx if idx >= 0 => {
                if let Some(p) = self.params.get_mut(idx as usize) {
                    p.value = value;
                    if (p.data.hints & PARAMETER_USES_CUSTOM_TEXT) != 0 {
                        p.custom_text = host.get_parameter_text(plugin_id, idx as u32);
                    }
                }
            }
            _ => {}
        }
    }
}

pub fn show_plugin_edit_window(
    ctx: &egui::Context,
    plugin_id: u32,
    info: &PluginInfo,
    active: bool,
    open: &mut bool,
    state: &mut PluginEditState,
) -> Vec<PluginEditAction> {
    let mut actions = Vec::new();
    let title = format!("{} — Parameters", info.name);

    egui::Window::new(title)
        .id(egui::Id::unique(("plugin_edit_win", plugin_id)))
        .open(open)
        .default_size([560.0, 520.0])
        .resizable(true)
        .show(ctx, |ui| {
            // Top bar: tabs + state actions
            ui.horizontal(|ui| {
                ui.selectable_value(&mut state.active_tab, EditTab::Parameters, "Parameters");
                ui.selectable_value(&mut state.active_tab, EditTab::Info, "Info & Options");
                if !state.programs.is_empty() || !state.midi_programs.is_empty() {
                    ui.selectable_value(&mut state.active_tab, EditTab::Programs, "Programs");
                }
                ui.selectable_value(&mut state.active_tab, EditTab::Keyboard, "Keyboard");

                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    if ui.small_button("Save State").clicked() {
                        actions.push(PluginEditAction::SaveState(plugin_id));
                    }
                    if ui.small_button("Load State").clicked() {
                        actions.push(PluginEditAction::LoadState(plugin_id));
                    }
                    if ui.small_button("Randomize").clicked() {
                        actions.push(PluginEditAction::RandomizeParameters(plugin_id));
                    }
                    if ui.small_button("Reset").clicked() {
                        actions.push(PluginEditAction::ResetParameters(plugin_id));
                    }
                });
            });

            ui.separator();

            // Internal controls bar (Active, Dry/Wet, Volume, Balance/Pan)
            ui.horizontal_wrapped(|ui| {
                let mut act = active;
                if ui.checkbox(&mut act, "Active").changed() {
                    actions.push(PluginEditAction::SetActive(plugin_id, act));
                }
                if (info.hints & PLUGIN_CAN_DRYWET) != 0 {
                    ui.label("Dry/Wet:");
                    let mut v = state.drywet;
                    if ui
                        .add(egui::Slider::new(&mut v, 0.0..=1.0).fixed_decimals(2))
                        .changed()
                    {
                        state.drywet = v;
                        actions.push(PluginEditAction::SetInternalParam(
                            plugin_id,
                            PARAMETER_DRYWET,
                            v,
                        ));
                    }
                }
                if (info.hints & PLUGIN_CAN_VOLUME) != 0 {
                    ui.label("Volume:");
                    let mut v = state.volume;
                    if ui
                        .add(egui::Slider::new(&mut v, 0.0..=1.27).fixed_decimals(2))
                        .changed()
                    {
                        state.volume = v;
                        actions.push(PluginEditAction::SetInternalParam(
                            plugin_id,
                            PARAMETER_VOLUME,
                            v,
                        ));
                    }
                }
                if (info.hints & PLUGIN_CAN_BALANCE) != 0 {
                    ui.label("Bal L:");
                    let mut l = state.balance_left;
                    if ui
                        .add(egui::Slider::new(&mut l, -1.0..=1.0).fixed_decimals(2))
                        .changed()
                    {
                        state.balance_left = l;
                        actions.push(PluginEditAction::SetInternalParam(
                            plugin_id,
                            PARAMETER_BALANCE_LEFT,
                            l,
                        ));
                    }
                    ui.label("Bal R:");
                    let mut r = state.balance_right;
                    if ui
                        .add(egui::Slider::new(&mut r, -1.0..=1.0).fixed_decimals(2))
                        .changed()
                    {
                        state.balance_right = r;
                        actions.push(PluginEditAction::SetInternalParam(
                            plugin_id,
                            PARAMETER_BALANCE_RIGHT,
                            r,
                        ));
                    }
                } else if (info.hints & PLUGIN_CAN_PANNING) != 0 {
                    ui.label("Pan:");
                    let mut p = state.panning;
                    if ui
                        .add(egui::Slider::new(&mut p, -1.0..=1.0).fixed_decimals(2))
                        .changed()
                    {
                        state.panning = p;
                        actions.push(PluginEditAction::SetInternalParam(
                            plugin_id,
                            PARAMETER_PANNING,
                            p,
                        ));
                    }
                }
            });

            ui.separator();

            match state.active_tab {
                EditTab::Info => show_info_tab(ui, plugin_id, info, state, &mut actions),
                EditTab::Parameters => show_parameters_tab(ui, plugin_id, state, &mut actions),
                EditTab::Programs => show_programs_tab(ui, plugin_id, state, &mut actions),
                EditTab::Keyboard => {
                    let note_evs = state.keyboard.show(ui, active);
                    for ev in note_evs {
                        actions.push(PluginEditAction::SendNote(plugin_id, ev));
                    }
                }
            }
        });

    actions
}

fn show_info_tab(
    ui: &mut egui::Ui,
    plugin_id: u32,
    info: &PluginInfo,
    state: &mut PluginEditState,
    actions: &mut Vec<PluginEditAction>,
) {
    egui::ScrollArea::vertical().show(ui, |ui| {
        egui::Grid::new(("plugin_info_grid", plugin_id))
            .num_columns(2)
            .striped(true)
            .spacing([12.0, 4.0])
            .show(ui, |ui| {
                ui.label("Format:");
                ui.label(plugin_type_label(info.ptype));
                ui.end_row();
                ui.label("Name:");
                ui.label(&info.name);
                ui.end_row();
                ui.label("Label:");
                ui.label(&info.label);
                ui.end_row();
                ui.label("Maker:");
                ui.label(&info.maker);
                ui.end_row();
                ui.label("Copyright:");
                ui.label(&info.copyright);
                ui.end_row();
                ui.label("Unique ID:");
                ui.label(format!("{}", info.unique_id));
                ui.end_row();
                ui.label("Filename:");
                ui.label(&info.filename);
                ui.end_row();
                ui.label("Audio I/O:");
                ui.label(format!("{} in / {} out", state.audio_ports.ins, state.audio_ports.outs));
                ui.end_row();
                ui.label("MIDI I/O:");
                ui.label(format!("{} in / {} out", state.midi_ports.ins, state.midi_ports.outs));
                ui.end_row();
                ui.label("Parameters:");
                ui.label(format!("{} in / {} out", state.param_counts.ins, state.param_counts.outs));
                ui.end_row();
                ui.label("Latency:");
                ui.label(format!("{} frames", state.latency));
                ui.end_row();
                ui.label("Control Channel:");
                let mut ch = state.ctrl_channel;
                let ch_lbl = if ch < 0 {
                    "None".to_string()
                } else {
                    format!("{}", ch + 1)
                };
                egui::ComboBox::from_id_salt(("ctrl_ch", plugin_id))
                    .selected_text(ch_lbl)
                    .show_ui(ui, |ui| {
                        if ui.selectable_value(&mut ch, -1, "None").clicked() {
                            state.ctrl_channel = -1;
                            actions.push(PluginEditAction::SetCtrlChannel(plugin_id, -1));
                        }
                        for c in 0..16i8 {
                            if ui.selectable_value(&mut ch, c, format!("{}", c + 1)).clicked() {
                                state.ctrl_channel = c;
                                actions.push(PluginEditAction::SetCtrlChannel(plugin_id, c));
                            }
                        }
                    });
                ui.end_row();
            });

        ui.add_space(10.0);
        ui.heading("Plugin Options");
        let option_defs: &[(u32, &str)] = &[
            (PLUGIN_OPTION_FIXED_BUFFERS, "Use fixed buffers"),
            (PLUGIN_OPTION_FORCE_STEREO, "Force stereo"),
            (PLUGIN_OPTION_MAP_PROGRAM_CHANGES, "Map program changes"),
            (PLUGIN_OPTION_USE_CHUNKS, "Use chunks"),
            (PLUGIN_OPTION_SEND_CONTROL_CHANGES, "Send control changes"),
            (PLUGIN_OPTION_SEND_CHANNEL_PRESSURE, "Send channel pressure"),
            (PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH, "Send note aftertouch"),
            (PLUGIN_OPTION_SEND_PITCHBEND, "Send pitchbend"),
            (PLUGIN_OPTION_SEND_ALL_SOUND_OFF, "Send all sound off"),
            (PLUGIN_OPTION_SEND_PROGRAM_CHANGES, "Send program changes"),
            (PLUGIN_OPTION_SKIP_SENDING_NOTES, "Skip sending notes"),
        ];
        for &(bit, label) in option_defs {
            if (info.options_available & bit) != 0 {
                let mut enabled = (info.options_enabled & bit) != 0;
                if ui.checkbox(&mut enabled, label).changed() {
                    actions.push(PluginEditAction::SetOption(plugin_id, bit, enabled));
                }
            }
        }
        ui.add_space(8.0);
        if ui.button("Reload Info & Parameters").clicked() {
            actions.push(PluginEditAction::ReloadAll(plugin_id));
        }
    });
}

fn show_parameters_tab(
    ui: &mut egui::Ui,
    plugin_id: u32,
    state: &mut PluginEditState,
    actions: &mut Vec<PluginEditAction>,
) {
    ui.horizontal(|ui| {
        ui.label("Filter:");
        ui.text_edit_singleline(&mut state.param_search);
        if !state.param_search.is_empty() && ui.small_button("✕").clicked() {
            state.param_search.clear();
        }
    });
    ui.add_space(4.0);

    let filter = state.param_search.trim().to_ascii_lowercase();

    // Group indices by group_name (preserving input vs output)
    let mut input_groups: BTreeMap<String, Vec<usize>> = BTreeMap::new();
    let mut output_indices: Vec<usize> = Vec::new();

    for (idx, p) in state.params.iter().enumerate() {
        if (p.data.hints & PARAMETER_IS_ENABLED) == 0 {
            continue;
        }
        if !filter.is_empty()
            && !p.info.name.to_ascii_lowercase().contains(&filter)
            && !p.info.group_name.to_ascii_lowercase().contains(&filter)
        {
            continue;
        }
        if p.data.ptype == PARAMETER_OUTPUT {
            output_indices.push(idx);
        } else {
            let gname = if p.info.group_name.is_empty() {
                "Parameters".to_string()
            } else {
                p.info.group_name.clone()
            };
            input_groups.entry(gname).or_default().push(idx);
        }
    }

    egui::ScrollArea::vertical()
        .id_salt(("param_scroll", plugin_id))
        .auto_shrink([false, false])
        .show(ui, |ui| {
            if input_groups.is_empty() && output_indices.is_empty() {
                ui.label(egui::RichText::new("No parameters to display.").weak());
                return;
            }

            let multi_group = input_groups.len() > 1;
            for (group_name, indices) in input_groups {
                if multi_group {
                    ui.collapsing(egui::RichText::new(&group_name).strong(), |ui| {
                        render_param_rows(ui, plugin_id, &indices, state, actions, false);
                    });
                } else {
                    render_param_rows(ui, plugin_id, &indices, state, actions, false);
                }
            }

            if !output_indices.is_empty() {
                ui.add_space(8.0);
                ui.collapsing(egui::RichText::new("Output Parameters (Read-Only)").strong(), |ui| {
                    render_param_rows(ui, plugin_id, &output_indices, state, actions, true);
                });
            }
        });
}

fn render_param_rows(
    ui: &mut egui::Ui,
    plugin_id: u32,
    indices: &[usize],
    state: &mut PluginEditState,
    actions: &mut Vec<PluginEditAction>,
    read_only_section: bool,
) {
    for &idx in indices {
        let p = &mut state.params[idx];
        let pidx = p.index;
        let ro = read_only_section || (p.data.hints & PARAMETER_IS_READ_ONLY) != 0;
        let is_bool = (p.data.hints & PARAMETER_IS_BOOLEAN) != 0;
        let is_int = (p.data.hints & PARAMETER_IS_INTEGER) != 0;
        let is_log = (p.data.hints & PARAMETER_IS_LOGARITHMIC) != 0;
        let uses_sp = (p.data.hints & PARAMETER_USES_SCALEPOINTS) != 0 && !p.scale_points.is_empty();

        ui.horizontal(|ui| {
            let name_lbl = if p.info.unit.is_empty() {
                p.info.name.clone()
            } else {
                format!("{} ({})", p.info.name, p.info.unit)
            };
            let lbl = ui.add_sized([170.0, 20.0], egui::Label::new(name_lbl).truncate());
            if !p.info.comment.is_empty() {
                lbl.on_hover_text(&p.info.comment);
            }

            if is_bool {
                let mut checked = p.value >= 0.5 * (p.ranges.min + p.ranges.max);
                if ui.add_enabled(!ro, egui::Checkbox::without_text(&mut checked)).changed() {
                    let nv = if checked { p.ranges.max } else { p.ranges.min };
                    p.value = nv;
                    actions.push(PluginEditAction::SetParam(plugin_id, pidx, nv));
                }
            } else if uses_sp && !ro {
                let cur_label = p
                    .scale_points
                    .iter()
                    .min_by(|a, b| {
                        (a.value - p.value)
                            .abs()
                            .total_cmp(&(b.value - p.value).abs())
                    })
                    .map(|sp| sp.label.clone())
                    .unwrap_or_else(|| format!("{:.2}", p.value));
                egui::ComboBox::from_id_salt(("sp", plugin_id, pidx))
                    .selected_text(cur_label)
                    .width(200.0)
                    .show_ui(ui, |ui| {
                        for sp in &p.scale_points {
                            if ui
                                .selectable_label((p.value - sp.value).abs() < 1e-4, &sp.label)
                                .clicked()
                            {
                                p.value = sp.value;
                                actions.push(PluginEditAction::SetParam(plugin_id, pidx, sp.value));
                            }
                        }
                    });
            } else {
                let min_v = p.ranges.min;
                let max_v = if p.ranges.max > p.ranges.min {
                    p.ranges.max
                } else {
                    p.ranges.min + 1.0
                };
                let mut v = p.value.clamp(min_v, max_v);
                let mut slider = egui::Slider::new(&mut v, min_v..=max_v);
                if is_log && min_v > 0.0 {
                    slider = slider.logarithmic(true);
                }
                if is_int {
                    slider = slider.integer();
                } else {
                    slider = slider.fixed_decimals(3);
                }
                if !p.custom_text.is_empty() {
                    slider = slider.show_value(false);
                }
                let resp = ui.add_enabled(!ro, slider);
                if resp.changed() {
                    p.value = v;
                    actions.push(PluginEditAction::SetParam(plugin_id, pidx, v));
                }
                if !p.custom_text.is_empty() {
                    ui.label(egui::RichText::new(&p.custom_text).monospace().small());
                }
            }

            // Reset to default & MIDI CC mapping button for input parameters
            if !ro {
                if ui.small_button("↺").on_hover_text("Reset to default").clicked() {
                    p.value = p.ranges.def;
                    actions.push(PluginEditAction::SetParam(plugin_id, pidx, p.ranges.def));
                }
                if (p.data.hints & PARAMETER_IS_AUTOMATABLE) != 0 {
                    let cc_label = match p.data.mapped_control_index {
                        CONTROL_INDEX_NONE => "CC".to_string(),
                        CONTROL_INDEX_CV => "CV".to_string(),
                        CONTROL_INDEX_MIDI_PITCHBEND => "PB".to_string(),
                        CONTROL_INDEX_MIDI_LEARN => "Learn".to_string(),
                        cc => format!("CC{cc}"),
                    };
                    let cc_id = ui.make_persistent_id(("cc_pop", plugin_id, pidx));
                    let btn = ui
                        .small_button(cc_label)
                        .on_hover_text("MIDI CC / Automation mapping");
                    if btn.clicked() {
                        egui::Popup::toggle_id(ui.ctx(), cc_id);
                    }
                    egui::Popup::menu(&btn).id(cc_id).show(|ui| {
                        ui.label(egui::RichText::new("MIDI / CV Mapping").strong());
                        let mut cur_cc = p.data.mapped_control_index;
                        egui::ComboBox::from_id_salt(("cc_sel", plugin_id, pidx))
                            .selected_text(
                                MIDI_CC_ITEMS
                                    .iter()
                                    .find(|(id, _)| *id == cur_cc)
                                    .map(|(_, l)| *l)
                                    .unwrap_or("Custom"),
                            )
                            .show_ui(ui, |ui| {
                                for &(cc_val, lbl) in MIDI_CC_ITEMS {
                                    if ui.selectable_value(&mut cur_cc, cc_val, lbl).clicked() {
                                        p.data.mapped_control_index = cc_val;
                                        actions.push(PluginEditAction::SetParamMappedControl(
                                            plugin_id, pidx, cc_val,
                                        ));
                                    }
                                }
                            });

                        ui.horizontal(|ui| {
                            ui.label("MIDI Ch:");
                            let mut ch = p.data.midi_channel + 1;
                            if ui.add(egui::DragValue::new(&mut ch).range(1..=16)).changed() {
                                p.data.midi_channel = ch - 1;
                                actions.push(PluginEditAction::SetParamMidiChannel(
                                    plugin_id,
                                    pidx,
                                    ch - 1,
                                ));
                            }
                        });
                        ui.horizontal(|ui| {
                            ui.label("Range:");
                            let mut rmin = p.data.mapped_minimum;
                            let mut rmax = p.data.mapped_maximum;
                            let c1 = ui
                                .add(
                                    egui::DragValue::new(&mut rmin)
                                        .speed(0.01)
                                        .range(p.ranges.min..=p.ranges.max),
                                )
                                .changed();
                            let c2 = ui
                                .add(
                                    egui::DragValue::new(&mut rmax)
                                        .speed(0.01)
                                        .range(p.ranges.min..=p.ranges.max),
                                )
                                .changed();
                            if c1 || c2 {
                                p.data.mapped_minimum = rmin;
                                p.data.mapped_maximum = rmax;
                                actions.push(PluginEditAction::SetParamMappedRange(
                                    plugin_id, pidx, rmin, rmax,
                                ));
                            }
                        });
                    });
                }
            }
        });
    }
}

fn show_programs_tab(
    ui: &mut egui::Ui,
    plugin_id: u32,
    state: &mut PluginEditState,
    actions: &mut Vec<PluginEditAction>,
) {
    if !state.programs.is_empty() {
        ui.heading("Programs");
        let cur_name = state
            .programs
            .get(state.current_program as usize)
            .cloned()
            .unwrap_or_else(|| "None".into());
        egui::ComboBox::from_id_salt(("prog_cb", plugin_id))
            .selected_text(cur_name)
            .width(340.0)
            .show_ui(ui, |ui| {
                for (i, name) in state.programs.iter().enumerate() {
                    if ui
                        .selectable_label(state.current_program == i as i32, format!("{i:03} - {name}"))
                        .clicked()
                    {
                        state.current_program = i as i32;
                        actions.push(PluginEditAction::SetProgram(plugin_id, i as u32));
                    }
                }
            });
        ui.add_space(10.0);
    }

    if !state.midi_programs.is_empty() {
        ui.heading("MIDI Programs");
        let cur_name = state
            .midi_programs
            .get(state.current_midi_program as usize)
            .map(|m| format!("{:03}:{:03} - {}", m.bank, m.program, m.name))
            .unwrap_or_else(|| "None".into());
        egui::ComboBox::from_id_salt(("mprog_cb", plugin_id))
            .selected_text(cur_name)
            .width(340.0)
            .show_ui(ui, |ui| {
                for (i, m) in state.midi_programs.iter().enumerate() {
                    let lbl = format!("{:03}:{:03} - {}", m.bank, m.program, m.name);
                    if ui
                        .selectable_label(state.current_midi_program == i as i32, lbl)
                        .clicked()
                    {
                        state.current_midi_program = i as i32;
                        actions.push(PluginEditAction::SetMidiProgram(plugin_id, i as u32));
                    }
                }
            });
    }
}
