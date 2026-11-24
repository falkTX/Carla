// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::HashSet;

use eframe::egui::{self, Color32, CornerRadius, Pos2, Rect, Stroke, Vec2};

#[derive(Clone, Debug)]
pub struct PianoKeyboardState {
    pub base_octave: i32,
    pub num_octaves: i32,
    pub channel: u8,
    pub velocity: u8,
    pub mouse_note: Option<u8>,
    pub active_notes: HashSet<u8>,
}

impl Default for PianoKeyboardState {
    fn default() -> Self {
        Self {
            base_octave: 2,
            num_octaves: 5,
            channel: 0,
            velocity: 100,
            mouse_note: None,
            active_notes: HashSet::new(),
        }
    }
}

#[derive(Clone, Copy, Debug)]
pub enum NoteEvent {
    On { channel: u8, note: u8, velocity: u8 },
    Off { channel: u8, note: u8 },
}

fn is_black_key(semitone: u8) -> bool {
    matches!(semitone % 12, 1 | 3 | 6 | 8 | 10)
}

impl PianoKeyboardState {
    pub fn note_on_external(&mut self, note: u8) {
        self.active_notes.insert(note);
    }

    pub fn note_off_external(&mut self, note: u8) {
        self.active_notes.remove(&note);
    }

    pub fn all_notes_off(&mut self) {
        self.active_notes.clear();
        self.mouse_note = None;
    }

    pub fn show(&mut self, ui: &mut egui::Ui, enabled: bool) -> Vec<NoteEvent> {
        let mut events = Vec::new();

        ui.horizontal(|ui| {
            ui.label("Octave:");
            if ui.small_button("−").clicked() && self.base_octave > 0 {
                self.base_octave -= 1;
            }
            ui.label(format!("C{}", self.base_octave));
            if ui.small_button("+").clicked() && self.base_octave < 6 {
                self.base_octave += 1;
            }
            ui.separator();
            ui.label("Channel:");
            let mut ch_disp = self.channel + 1;
            if ui
                .add(egui::DragValue::new(&mut ch_disp).range(1..=16))
                .changed()
            {
                self.channel = ch_disp - 1;
            }
        });

        let white_count = (self.num_octaves * 7) as usize;
        let avail_w = ui.available_width().max(320.0);
        let key_h = 68.0;
        let (rect, response) =
            ui.allocate_exact_size(Vec2::new(avail_w, key_h), egui::Sense::click_and_drag());

        let white_w = rect.width() / white_count as f32;
        let black_w = white_w * 0.62;
        let black_h = key_h * 0.60;

        let mut white_keys: Vec<(u8, Rect)> = Vec::with_capacity(white_count);
        let mut black_keys: Vec<(u8, Rect)> = Vec::new();

        let start_midi = ((self.base_octave + 1) * 12).clamp(0, 108) as u8;
        let total_semitones = (self.num_octaves * 12) as u8;

        let mut white_idx = 0usize;
        for offset in 0..total_semitones {
            let note = start_midi.saturating_add(offset);
            if note > 127 {
                break;
            }
            if !is_black_key(note) {
                let x = rect.min.x + white_idx as f32 * white_w;
                let r = Rect::from_min_size(Pos2::new(x, rect.min.y), Vec2::new(white_w, key_h));
                white_keys.push((note, r));
                white_idx += 1;
            } else {
                let x = rect.min.x + white_idx as f32 * white_w - black_w * 0.5;
                let r = Rect::from_min_size(Pos2::new(x, rect.min.y), Vec2::new(black_w, black_h));
                black_keys.push((note, r));
            }
        }

        // Interaction
        if enabled {
            let pointer_down = response.is_pointer_button_down_on();
            let hit_note = if pointer_down {
                response.interact_pointer_pos().and_then(|pos| {
                    black_keys
                        .iter()
                        .find(|(_, r)| r.contains(pos))
                        .or_else(|| white_keys.iter().find(|(_, r)| r.contains(pos)))
                        .map(|(n, _)| *n)
                })
            } else {
                None
            };

            if hit_note != self.mouse_note {
                if let Some(prev) = self.mouse_note.take() {
                    self.active_notes.remove(&prev);
                    events.push(NoteEvent::Off {
                        channel: self.channel,
                        note: prev,
                    });
                }
                if let Some(next) = hit_note {
                    self.mouse_note = Some(next);
                    self.active_notes.insert(next);
                    events.push(NoteEvent::On {
                        channel: self.channel,
                        note: next,
                        velocity: self.velocity,
                    });
                }
            }
        } else if let Some(prev) = self.mouse_note.take() {
            self.active_notes.remove(&prev);
            events.push(NoteEvent::Off {
                channel: self.channel,
                note: prev,
            });
        }

        // Render white keys first, then black keys on top
        let painter = ui.painter();
        let active_col = Color32::from_rgb(59, 130, 246);
        let active_black = Color32::from_rgb(37, 99, 235);

        for (note, r) in &white_keys {
            let fill = if !enabled {
                Color32::from_rgb(160, 160, 165)
            } else if self.active_notes.contains(note) {
                active_col
            } else {
                Color32::from_rgb(240, 240, 244)
            };
            painter.rect_filled(*r, CornerRadius::ZERO, fill);
            painter.rect_stroke(
                *r,
                CornerRadius::ZERO,
                Stroke::new(0.8, Color32::from_rgb(45, 45, 50)),
                egui::StrokeKind::Inside,
            );
            if note % 12 == 0 {
                let oct = (*note as i32 / 12) - 1;
                painter.text(
                    Pos2::new(r.center().x, r.max.y - 10.0),
                    egui::Align2::CENTER_CENTER,
                    format!("C{oct}"),
                    egui::FontId::proportional(9.5),
                    Color32::from_rgb(90, 90, 100),
                );
            }
        }

        for (note, r) in &black_keys {
            let fill = if !enabled {
                Color32::from_rgb(70, 70, 76)
            } else if self.active_notes.contains(note) {
                active_black
            } else {
                Color32::from_rgb(24, 26, 32)
            };
            painter.rect_filled(*r, CornerRadius::same(1), fill);
            painter.rect_stroke(
                *r,
                CornerRadius::same(1),
                Stroke::new(0.8, Color32::BLACK),
                egui::StrokeKind::Inside,
            );
        }

        events
    }
}
