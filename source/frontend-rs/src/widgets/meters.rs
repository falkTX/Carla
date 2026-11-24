// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::VecDeque;
use std::time::Instant;

use eframe::egui::{self, Color32, CornerRadius, Pos2, Rect, Stroke, Vec2};

pub const CLIP_THRESHOLD: f32 = 0.999;
pub const CLIP_HOLD_SECONDS: f64 = 0.6;
pub const CLIP_WARNING_HITS: usize = 4;
pub const CLIP_WARNING_WINDOW_SECONDS: f64 = 2.5;
pub const CLIP_WARNING_CLEAR_SECONDS: f64 = 5.0;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MeterColor {
    Green,
    Blue,
}

#[derive(Clone, Debug)]
pub struct PeakMeterState {
    pub channels: usize,
    pub color: MeterColor,
    pub smoothed: [f32; 2],
    pub raw: [f32; 2],
    pub clip_until: [f64; 2],
    pub smooth_multiplier: f32,
}

impl PeakMeterState {
    pub fn new(channels: usize, color: MeterColor) -> Self {
        Self {
            channels: channels.min(2),
            color,
            smoothed: [0.0; 2],
            raw: [0.0; 2],
            clip_until: [0.0; 2],
            smooth_multiplier: 2.0,
        }
    }

    pub fn set_channels(&mut self, channels: usize) {
        self.channels = channels.min(2);
    }

    pub fn clear(&mut self) {
        self.smoothed = [0.0; 2];
        self.raw = [0.0; 2];
        self.clip_until = [0.0; 2];
    }

    pub fn update_channel(&mut self, ch: usize, raw_level: f32, now_secs: f64) {
        if ch >= 2 {
            return;
        }
        let level = raw_level.clamp(0.0, 1.5);
        self.raw[ch] = level;

        let clamped = level.clamp(0.0, 1.0);
        if level >= CLIP_THRESHOLD {
            self.clip_until[ch] = now_secs + CLIP_HOLD_SECONDS;
            self.smoothed[ch] = 1.0;
        } else if clamped >= self.smoothed[ch] {
            self.smoothed[ch] = clamped;
        } else if self.smooth_multiplier > 0.0 {
            self.smoothed[ch] =
                (self.smoothed[ch] * self.smooth_multiplier + clamped) / (self.smooth_multiplier + 1.0);
        } else {
            self.smoothed[ch] = clamped;
        }
        if self.smoothed[ch] < 0.001 {
            self.smoothed[ch] = 0.0;
        }
    }

    pub fn is_channel_clipping(&self, ch: usize, now_secs: f64) -> bool {
        if ch >= 2 {
            return false;
        }
        self.raw[ch] >= CLIP_THRESHOLD || now_secs < self.clip_until[ch]
    }

    pub fn max_raw(&self) -> f32 {
        match self.channels {
            0 => 0.0,
            1 => self.raw[0],
            _ => self.raw[0].max(self.raw[1]),
        }
    }

    pub fn draw_horizontal(&self, ui: &mut egui::Ui, size: Vec2, now_secs: f64, tooltip: &str) {
        let (rect, response) = ui.allocate_exact_size(size, egui::Sense::hover());
        response.on_hover_text(tooltip);

        let painter = ui.painter();
        painter.rect_filled(rect, CornerRadius::same(2), Color32::from_rgb(14, 16, 20));
        painter.rect_stroke(
            rect,
            CornerRadius::same(2),
            Stroke::new(0.8, Color32::from_rgb(45, 50, 58)),
            egui::StrokeKind::Inside,
        );

        if self.channels == 0 {
            return;
        }

        let inner = rect.shrink(1.0);
        let ch_count = self.channels.max(1);
        let gap = if ch_count > 1 { 1.0 } else { 0.0 };
        let ch_h = (inner.height() - gap * (ch_count - 1) as f32) / ch_count as f32;
        let base_color = match self.color {
            MeterColor::Green => Color32::from_rgb(74, 222, 128),
            MeterColor::Blue => Color32::from_rgb(56, 189, 248),
        };
        let hot_color = Color32::from_rgb(250, 204, 21);
        let clip_color = Color32::from_rgb(220, 38, 38);

        for ch in 0..ch_count {
            let y0 = inner.min.y + ch as f32 * (ch_h + gap);
            let y1 = y0 + ch_h;
            let level = self.smoothed[ch].clamp(0.0, 1.0);
            let clipping = self.is_channel_clipping(ch, now_secs);

            if clipping {
                // Entire active bar turns red on 0 dB clip + bright red cap
                let fill_w = (inner.width() * level.max(0.95)).max(3.0);
                let fill_rect = Rect::from_min_max(
                    Pos2::new(inner.min.x, y0),
                    Pos2::new(inner.min.x + fill_w, y1),
                );
                painter.rect_filled(fill_rect, CornerRadius::ZERO, clip_color);
                let cap_rect = Rect::from_min_max(
                    Pos2::new((inner.max.x - 3.0).max(inner.min.x), y0),
                    Pos2::new(inner.max.x, y1),
                );
                painter.rect_filled(cap_rect, CornerRadius::ZERO, Color32::from_rgb(239, 68, 68));
            } else if level > 0.001 {
                // Visual perceptual curve (sqrt) matching DigitalPeakMeter
                let visual = level.sqrt().clamp(0.0, 1.0);
                let fill_w = inner.width() * visual;
                let fill_rect = Rect::from_min_max(
                    Pos2::new(inner.min.x, y0),
                    Pos2::new(inner.min.x + fill_w, y1),
                );
                let col = if level >= 0.85 { hot_color } else { base_color };
                painter.rect_filled(fill_rect, CornerRadius::ZERO, col);
            }
        }
    }

    pub fn draw_vertical(&self, ui: &mut egui::Ui, size: Vec2, now_secs: f64, tooltip: &str) {
        let (rect, response) = ui.allocate_exact_size(size, egui::Sense::hover());
        response.on_hover_text(tooltip);

        let painter = ui.painter();
        painter.rect_filled(rect, CornerRadius::same(2), Color32::from_rgb(14, 16, 20));
        painter.rect_stroke(
            rect,
            CornerRadius::same(2),
            Stroke::new(0.8, Color32::from_rgb(45, 50, 58)),
            egui::StrokeKind::Inside,
        );

        let ch_count = self.channels.max(1);
        let inner = rect.shrink(1.5);
        let gap = if ch_count > 1 { 2.0 } else { 0.0 };
        let ch_w = (inner.width() - gap * (ch_count - 1) as f32) / ch_count as f32;

        let base_color = match self.color {
            MeterColor::Green => Color32::from_rgb(74, 222, 128),
            MeterColor::Blue => Color32::from_rgb(56, 189, 248),
        };
        let clip_color = Color32::from_rgb(220, 38, 38);

        for ch in 0..ch_count {
            let x0 = inner.min.x + ch as f32 * (ch_w + gap);
            let x1 = x0 + ch_w;
            let level = self.smoothed[ch].clamp(0.0, 1.0);
            let clipping = self.is_channel_clipping(ch, now_secs);

            if clipping {
                let fill_h = (inner.height() * level.max(0.95)).max(4.0);
                let fill_rect = Rect::from_min_max(
                    Pos2::new(x0, inner.max.y - fill_h),
                    Pos2::new(x1, inner.max.y),
                );
                painter.rect_filled(fill_rect, CornerRadius::ZERO, clip_color);
                let cap_rect = Rect::from_min_max(
                    Pos2::new(x0, inner.min.y),
                    Pos2::new(x1, (inner.min.y + 3.0).min(inner.max.y)),
                );
                painter.rect_filled(cap_rect, CornerRadius::ZERO, Color32::from_rgb(239, 68, 68));
            } else if level > 0.001 {
                let visual = level.sqrt().clamp(0.0, 1.0);
                let fill_h = inner.height() * visual;
                let fill_rect = Rect::from_min_max(
                    Pos2::new(x0, inner.max.y - fill_h),
                    Pos2::new(x1, inner.max.y),
                );
                painter.rect_filled(fill_rect, CornerRadius::ZERO, base_color);
            }
        }
    }
}

#[derive(Clone, Debug, Default)]
pub struct ClipWarningState {
    hits: VecDeque<f64>,
    last_hit: f64,
    active: bool,
}

impl ClipWarningState {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn is_active(&self) -> bool {
        self.active
    }

    pub fn clear(&mut self) {
        self.hits.clear();
        self.last_hit = 0.0;
        self.active = false;
    }

    pub fn record_peak(&mut self, peak: f32, now_secs: f64) {
        if peak >= CLIP_THRESHOLD {
            self.last_hit = now_secs;
            self.hits.push_back(now_secs);
            let cutoff = now_secs - CLIP_WARNING_WINDOW_SECONDS;
            while self.hits.front().is_some_and(|&t| t < cutoff) {
                self.hits.pop_front();
            }
            if !self.active && self.hits.len() >= CLIP_WARNING_HITS {
                self.active = true;
            }
        } else if self.active && (now_secs - self.last_hit) >= CLIP_WARNING_CLEAR_SECONDS {
            self.clear();
        }
    }

    pub fn draw(&mut self, ui: &mut egui::Ui) {
        let (rect, response) = ui.allocate_exact_size(Vec2::splat(18.0), egui::Sense::click());
        if !self.active {
            return;
        }
        let response = response
            .on_hover_cursor(egui::CursorIcon::PointingHand)
            .on_hover_text("Audio is repeatedly hitting 0 dB (clipping). Click to dismiss.");
        if response.clicked() {
            self.clear();
            return;
        }

        let painter = ui.painter();
        let w = rect.width();
        let top = Pos2::new(rect.min.x + w * 0.5, rect.min.y + 1.5);
        let br = Pos2::new(rect.max.x - 1.5, rect.max.y - 1.5);
        let bl = Pos2::new(rect.min.x + 1.5, rect.max.y - 1.5);

        painter.add(egui::Shape::convex_polygon(
            vec![top, br, bl],
            Color32::from_rgb(0xFA, 0xCC, 0x15), // #FACC15
            Stroke::new(1.3, Color32::from_rgb(0x92, 0x40, 0x0E)), // #92400E
        ));
        let cx = rect.min.x + w * 0.5;
        painter.line_segment(
            [Pos2::new(cx, rect.min.y + 6.0), Pos2::new(cx, rect.max.y - 6.2)],
            Stroke::new(1.8, Color32::from_rgb(0x1F, 0x29, 0x37)),
        );
        painter.circle_filled(
            Pos2::new(cx, rect.max.y - 4.0),
            1.15,
            Color32::from_rgb(0x1F, 0x29, 0x37),
        );
    }
}

pub fn monotonic_secs(epoch: Instant) -> f64 {
    epoch.elapsed().as_secs_f64()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_peak_meter_clip_hold() {
        let mut m = PeakMeterState::new(2, MeterColor::Green);
        m.update_channel(0, 0.4, 1.0);
        assert!(!m.is_channel_clipping(0, 1.0));
        m.update_channel(0, 1.0, 1.1);
        assert!(m.is_channel_clipping(0, 1.1));
        m.update_channel(0, 0.2, 1.3);
        assert!(m.is_channel_clipping(0, 1.3)); // still in 0.6s hold
        m.update_channel(0, 0.2, 2.0);
        assert!(!m.is_channel_clipping(0, 2.0));
    }

    #[test]
    fn test_clip_warning_repeated_hits_and_auto_clear() {
        let mut w = ClipWarningState::new();
        w.record_peak(1.0, 10.0);
        assert!(!w.is_active());
        w.record_peak(1.0, 10.2);
        w.record_peak(1.0, 10.4);
        assert!(!w.is_active());
        w.record_peak(1.0, 10.6);
        assert!(w.is_active());
        // Silence for 3s still active
        w.record_peak(0.0, 13.6);
        assert!(w.is_active());
        // Silence past 5s auto-clears
        w.record_peak(0.0, 15.7);
        assert!(!w.is_active());
    }
}
