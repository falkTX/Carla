// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::VecDeque;
use std::time::Instant;

pub const CLIP_THRESHOLD: f32 = 0.999;
pub const CLIP_HOLD_SECONDS: f64 = 0.6;
pub const CLIP_WARNING_HITS: usize = 4;
pub const CLIP_WARNING_WINDOW_SECONDS: f64 = 2.5;
pub const CLIP_WARNING_CLEAR_SECONDS: f64 = 5.0;

#[derive(Clone, Debug)]
pub struct PeakMeterState {
    pub channels: usize,
    pub smoothed: [f32; 2],
    pub raw: [f32; 2],
    pub clip_until: [f64; 2],
    pub smooth_multiplier: f32,
}

impl PeakMeterState {
    pub fn new(channels: usize) -> Self {
        Self {
            channels: channels.min(2),
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
}

pub fn monotonic_secs(epoch: Instant) -> f64 {
    epoch.elapsed().as_secs_f64()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_peak_meter_clip_hold() {
        let mut m = PeakMeterState::new(2);
        m.update_channel(0, 0.4, 1.0);
        assert!(!m.is_channel_clipping(0, 1.0));
        m.update_channel(0, 1.0, 1.1);
        assert!(m.is_channel_clipping(0, 1.1));
        m.update_channel(0, 0.2, 1.3);
        assert!(m.is_channel_clipping(0, 1.3));
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
        w.record_peak(0.0, 13.6);
        assert!(w.is_active());
        w.record_peak(0.0, 15.7);
        assert!(!w.is_active());
    }
}
