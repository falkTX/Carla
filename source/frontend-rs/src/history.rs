// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::time::{Duration, Instant};

use crate::project::PluginSnapshot;

pub const HISTORY_LIMIT: usize = 50;
pub const MERGE_WINDOW: Duration = Duration::from_millis(400);

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PatchbayEndpoint {
    pub group_name: String,
    pub port_name: String,
    pub is_input: bool,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct GroupPosSnapshot {
    pub group_name: String,
    pub split: bool,
    pub pos1: (i32, i32),
    pub pos2: (i32, i32),
}

#[derive(Debug)]
pub enum HistoryAction {
    MovePlugin {
        from: u32,
        to: u32,
    },
    RenamePlugin {
        plugin_id: u32,
        before: String,
        after: String,
    },
    SetActive {
        plugin_id: u32,
        before: bool,
        after: bool,
    },
    SetInternalParam {
        plugin_id: u32,
        param_id: i32,
        before: f32,
        after: f32,
    },
    SetParam {
        plugin_id: u32,
        param_id: u32,
        before: f32,
        after: f32,
    },
    SetParamsBatch {
        plugin_id: u32,
        before: Vec<(u32, f32)>,
        after: Vec<(u32, f32)>,
    },
    SetCategory {
        plugin_id: u32,
        before: i32,
        after: i32,
    },
    AddPlugin {
        plugin_id: u32,
        snapshot: Option<PluginSnapshot>,
    },
    RemovePlugin {
        plugin_id: u32,
        snapshot: Option<PluginSnapshot>,
    },
    PatchbayLink {
        external: bool,
        output: PatchbayEndpoint,
        input: PatchbayEndpoint,
        connected: bool,
    },
    PatchbayArrange {
        external: bool,
        before: Vec<GroupPosSnapshot>,
        after: Vec<GroupPosSnapshot>,
    },
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum MergeKey {
    Param(u32, i32),
}

#[derive(Debug)]
pub struct HistoryEntry {
    pub label: String,
    pub action: HistoryAction,
    pub merge_key: Option<MergeKey>,
    pub timestamp: Instant,
}

pub struct EditHistory {
    entries: Vec<HistoryEntry>,
    index: usize,
    pub busy: bool,
    pub blocked: bool,
}

impl Default for EditHistory {
    fn default() -> Self {
        Self::new()
    }
}

impl EditHistory {
    pub fn new() -> Self {
        Self {
            entries: Vec::new(),
            index: 0,
            busy: false,
            blocked: false,
        }
    }

    pub fn clear(&mut self) {
        self.entries.clear();
        self.index = 0;
    }

    pub fn set_blocked(&mut self, blocked: bool) {
        self.blocked = blocked;
    }

    pub fn can_undo(&self) -> bool {
        !self.busy && !self.blocked && self.index > 0
    }

    pub fn can_redo(&self) -> bool {
        !self.busy && !self.blocked && self.index < self.entries.len()
    }

    pub fn undo_label(&self) -> String {
        if self.index > 0 {
            format!("Undo {}", self.entries[self.index - 1].label)
        } else {
            "Undo".to_string()
        }
    }

    pub fn redo_label(&self) -> String {
        if self.index < self.entries.len() {
            format!("Redo {}", self.entries[self.index].label)
        } else {
            "Redo".to_string()
        }
    }

    pub fn record(&mut self, label: impl Into<String>, action: HistoryAction, merge_key: Option<MergeKey>) {
        if self.busy || self.blocked {
            return;
        }
        let now = Instant::now();
        let label = label.into();

        // Coalesce continuous parameter drags within 400ms
        if let Some(ref key) = merge_key {
            if self.index > 0 && self.index == self.entries.len() {
                let prev = &mut self.entries[self.index - 1];
                if prev.merge_key.as_ref() == Some(key)
                    && now.duration_since(prev.timestamp) <= MERGE_WINDOW
                {
                    match (&mut prev.action, &action) {
                        (
                            HistoryAction::SetParam { after: prev_after, .. },
                            HistoryAction::SetParam { after: new_after, .. },
                        ) => {
                            *prev_after = *new_after;
                            prev.timestamp = now;
                            return;
                        }
                        (
                            HistoryAction::SetInternalParam { after: prev_after, .. },
                            HistoryAction::SetInternalParam { after: new_after, .. },
                        ) => {
                            *prev_after = *new_after;
                            prev.timestamp = now;
                            return;
                        }
                        _ => {}
                    }
                }
            }
        }

        self.entries.truncate(self.index);
        self.entries.push(HistoryEntry {
            label,
            action,
            merge_key,
            timestamp: now,
        });
        if self.entries.len() > HISTORY_LIMIT {
            let excess = self.entries.len() - HISTORY_LIMIT;
            self.entries.drain(0..excess);
        }
        self.index = self.entries.len();
    }

    pub fn take_undo(&mut self) -> Option<&mut HistoryEntry> {
        if !self.can_undo() {
            return None;
        }
        self.index -= 1;
        self.entries.get_mut(self.index)
    }

    pub fn revert_undo_index(&mut self) {
        if self.index < self.entries.len() {
            self.index += 1;
        }
    }

    pub fn take_redo(&mut self) -> Option<&mut HistoryEntry> {
        if !self.can_redo() {
            return None;
        }
        let idx = self.index;
        self.index += 1;
        self.entries.get_mut(idx)
    }

    pub fn revert_redo_index(&mut self) {
        if self.index > 0 {
            self.index -= 1;
        }
    }

    pub fn len(&self) -> usize {
        self.entries.len()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_history_undo_redo_and_coalesce() {
        let mut h = EditHistory::new();
        assert!(!h.can_undo());
        assert!(!h.can_redo());

        h.record(
            "Change Parameter",
            HistoryAction::SetParam {
                plugin_id: 0,
                param_id: 1,
                before: 0.2,
                after: 0.5,
            },
            Some(MergeKey::Param(0, 1)),
        );
        h.record(
            "Change Parameter",
            HistoryAction::SetParam {
                plugin_id: 0,
                param_id: 1,
                before: 0.5,
                after: 0.9,
            },
            Some(MergeKey::Param(0, 1)),
        );
        // Should coalesce into a single entry with before=0.2, after=0.9
        assert_eq!(h.len(), 1);
        assert_eq!(h.undo_label(), "Undo Change Parameter");

        let entry = h.take_undo().unwrap();
        if let HistoryAction::SetParam { before, after, .. } = entry.action {
            assert!((before - 0.2).abs() < 1e-6);
            assert!((after - 0.9).abs() < 1e-6);
        } else {
            panic!("unexpected variant");
        }
        assert!(!h.can_undo());
        assert!(h.can_redo());
        assert_eq!(h.redo_label(), "Redo Change Parameter");
    }

    #[test]
    fn test_history_limit() {
        let mut h = EditHistory::new();
        for i in 0..80 {
            h.record(
                format!("Step {i}"),
                HistoryAction::SetCategory {
                    plugin_id: 0,
                    before: 0,
                    after: i % 9,
                },
                None,
            );
        }
        assert_eq!(h.len(), HISTORY_LIMIT);
    }
}
