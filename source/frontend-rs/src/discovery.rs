// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::{HashMap, VecDeque};
use std::ffi::{c_char, c_void};
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};

use serde::{Deserialize, Serialize};

use crate::ffi::dirs_home;
use crate::ffi::host::cstr_to_string;
use crate::ffi::types::*;
use crate::ffi::utils::{CarlaUtils, DiscoveredPlugin};
use crate::settings::FrancaSettings;

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct PluginDatabase {
    pub plugins: Vec<DiscoveredPlugin>,
    /// Map from `btype:ptype:filename` -> (sha1, Vec<DiscoveredPlugin>)
    pub file_cache: HashMap<String, (String, Vec<DiscoveredPlugin>)>,
}

impl PluginDatabase {
    pub fn cache_path() -> PathBuf {
        let cache_home = std::env::var_os("XDG_CACHE_HOME")
            .map(PathBuf::from)
            .unwrap_or_else(|| dirs_home().join(".cache"));
        cache_home.join("franca").join("plugins.json")
    }

    pub fn load() -> Self {
        let path = Self::cache_path();
        if let Ok(raw) = fs::read_to_string(&path) {
            if let Ok(db) = serde_json::from_str::<PluginDatabase>(&raw) {
                return db;
            }
        }
        Self::default()
    }

    pub fn save(&self) -> Result<(), String> {
        let path = Self::cache_path();
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent).map_err(|e| e.to_string())?;
        }
        let json = serde_json::to_string(self).map_err(|e| e.to_string())?;
        fs::write(&path, json).map_err(|e| e.to_string())
    }

    pub fn ensure_cached_builtins(&mut self, utils: &CarlaUtils, settings: &FrancaSettings) {
        if !self.plugins.is_empty() {
            return;
        }
        self.refresh_instant_cached(utils, settings);
    }

    pub fn refresh_instant_cached(&mut self, utils: &CarlaUtils, settings: &FrancaSettings) {
        self.plugins.retain(|p| {
            p.ptype != PLUGIN_INTERNAL && p.ptype != PLUGIN_LV2 && p.ptype != PLUGIN_SFZ
        });
        for ptype in [PLUGIN_INTERNAL, PLUGIN_LV2, PLUGIN_SFZ] {
            let path = settings.plugin_path_joined(ptype);
            let found = utils.get_cached_plugins(ptype, &path);
            self.plugins.extend(found);
        }
        self.sort_and_dedup();
    }

    pub fn sort_and_dedup(&mut self) {
        self.plugins.sort_by(|a, b| {
            a.name
                .to_lowercase()
                .cmp(&b.name.to_lowercase())
                .then_with(|| a.ptype.cmp(&b.ptype))
                .then_with(|| a.label.cmp(&b.label))
        });
        let mut seen = std::collections::HashSet::new();
        self.plugins.retain(|p| seen.insert(p.unique_key()));
    }
}

#[derive(Clone, Debug)]
struct DiscoveryTask {
    tool: String,
    btype: i32,
    ptype: i32,
    plugin_path: String,
    label: String,
}

struct SharedDiscoveryState {
    discovered: Vec<DiscoveredPlugin>,
    cache_updates: Vec<(String, String, DiscoveredPlugin)>,
    cache_lookup: HashMap<String, (String, Vec<DiscoveredPlugin>)>,
    current_file: String,
}

unsafe extern "C" fn discovery_callback_trampoline(
    ptr: *mut c_void,
    info: *const RawCarlaPluginDiscoveryInfo,
    sha1: *const c_char,
) {
    if ptr.is_null() {
        return;
    }
    let state = unsafe { &*(ptr as *const Mutex<SharedDiscoveryState>) };
    let Ok(mut guard) = state.lock() else {
        return;
    };
    let sha = unsafe { cstr_to_string(sha1) };
    if info.is_null() {
        return;
    }
    let r = unsafe { &*info };
    let filename = unsafe { cstr_to_string(r.filename) };
    let name = unsafe { cstr_to_string(r.metadata.name) };
    guard.current_file = if !name.is_empty() {
        name.clone()
    } else {
        filename.clone()
    };
    let plugin = DiscoveredPlugin {
        build: r.btype,
        ptype: r.ptype,
        hints: r.metadata.hints,
        category: r.metadata.category,
        filename: filename.clone(),
        name,
        label: unsafe { cstr_to_string(r.label) },
        maker: unsafe { cstr_to_string(r.metadata.maker) },
        unique_id: r.unique_id,
        audio_ins: r.io.audio_ins,
        audio_outs: r.io.audio_outs,
        cv_ins: r.io.cv_ins,
        cv_outs: r.io.cv_outs,
        midi_ins: r.io.midi_ins,
        midi_outs: r.io.midi_outs,
        parameter_ins: r.io.parameter_ins,
        parameter_outs: r.io.parameter_outs,
    };
    let cache_key = format!("{}:{}:{}", plugin.build, plugin.ptype, filename);
    guard.cache_updates.push((cache_key, sha, plugin.clone()));
    guard.discovered.push(plugin);
}

unsafe extern "C" fn check_cache_trampoline(
    ptr: *mut c_void,
    filename: *const c_char,
    sha1: *const c_char,
) -> bool {
    if ptr.is_null() || filename.is_null() || sha1.is_null() {
        return false;
    }
    let state = unsafe { &*(ptr as *const Mutex<SharedDiscoveryState>) };
    let Ok(mut guard) = state.lock() else {
        return false;
    };
    let fname = unsafe { cstr_to_string(filename) };
    let sha = unsafe { cstr_to_string(sha1) };
    guard.current_file = fname.clone();
    // Check if any matching file entry has the same sha1
    let mut matched: Option<Vec<DiscoveredPlugin>> = None;
    for (k, (cached_sha, list)) in &guard.cache_lookup {
        if k.ends_with(&format!(":{fname}")) && cached_sha == &sha {
            matched = Some(list.clone());
            break;
        }
    }
    if let Some(list) = matched {
        guard.discovered.extend(list);
        true
    } else {
        false
    }
}

#[derive(Clone, Debug)]
pub struct DiscoveryOptions {
    pub check_internal: bool,
    pub check_ladspa: bool,
    pub check_dssi: bool,
    pub check_lv2: bool,
    pub check_vst2: bool,
    pub check_vst3: bool,
    pub check_clap: bool,
    pub check_sf2: bool,
    pub check_sfz: bool,
    pub check_jsfx: bool,
    pub check_invalid: bool,
}

impl Default for DiscoveryOptions {
    fn default() -> Self {
        Self {
            check_internal: true,
            check_ladspa: true,
            check_dssi: true,
            check_lv2: true,
            check_vst2: true,
            check_vst3: true,
            check_clap: true,
            check_sf2: true,
            check_sfz: true,
            check_jsfx: true,
            check_invalid: false,
        }
    }
}

pub struct DiscoveryRunner {
    tasks: VecDeque<DiscoveryTask>,
    total_tasks: usize,
    completed_tasks: usize,
    active_handle: CarlaPluginDiscoveryHandle,
    active_label: String,
    state: Arc<Mutex<SharedDiscoveryState>>,
    pub finished: bool,
    pub cancelled: bool,
    pub discovered_count: usize,
}

impl DiscoveryRunner {
    pub fn start(
        utils: &CarlaUtils,
        bin_dir: &Path,
        settings: &FrancaSettings,
        existing_cache: &HashMap<String, (String, Vec<DiscoveredPlugin>)>,
        opts: &DiscoveryOptions,
    ) -> Self {
        let mut tasks = VecDeque::new();
        let mut initial_plugins = Vec::new();

        // Instant cached formats (Internal, LV2, SFZ)
        for (enabled, ptype) in [
            (opts.check_internal, PLUGIN_INTERNAL),
            (opts.check_lv2, PLUGIN_LV2),
            (opts.check_sfz, PLUGIN_SFZ),
        ] {
            if enabled {
                let p = settings.plugin_path_joined(ptype);
                initial_plugins.extend(utils.get_cached_plugins(ptype, &p));
            }
        }

        let native_tool = bin_dir
            .join("carla-discovery-native")
            .to_string_lossy()
            .into_owned();
        if Path::new(&native_tool).exists() {
            for (enabled, ptype, name) in [
                (opts.check_ladspa, PLUGIN_LADSPA, "LADSPA"),
                (opts.check_dssi, PLUGIN_DSSI, "DSSI"),
                (opts.check_vst2, PLUGIN_VST2, "VST2"),
                (opts.check_vst3, PLUGIN_VST3, "VST3"),
                (opts.check_clap, PLUGIN_CLAP, "CLAP"),
                (opts.check_sf2, PLUGIN_SF2, "SF2"),
                (opts.check_jsfx, PLUGIN_JSFX, "JSFX"),
            ] {
                if enabled {
                    let plugin_path = settings.plugin_path_joined(ptype);
                    if !plugin_path.is_empty() {
                        tasks.push_back(DiscoveryTask {
                            tool: native_tool.clone(),
                            btype: BINARY_NATIVE,
                            ptype,
                            plugin_path,
                            label: format!("Scanning {name} plugins..."),
                        });
                    }
                }
            }
        }

        // Optional Wine bridges
        if settings.experimental && settings.exp_wine_bridges {
            for (exe, btype, arch) in [
                ("carla-discovery-win32.exe", BINARY_WIN32, "Win32"),
                ("carla-discovery-win64.exe", BINARY_WIN64, "Win64"),
            ] {
                let tool_path = bin_dir.join(exe);
                if tool_path.exists() {
                    let tool_str = tool_path.to_string_lossy().into_owned();
                    for (enabled, ptype, name) in [
                        (opts.check_vst2, PLUGIN_VST2, "VST2"),
                        (opts.check_vst3, PLUGIN_VST3, "VST3"),
                        (opts.check_clap, PLUGIN_CLAP, "CLAP"),
                    ] {
                        if enabled {
                            let plugin_path = settings.plugin_path_joined(ptype);
                            if !plugin_path.is_empty() {
                                tasks.push_back(DiscoveryTask {
                                    tool: tool_str.clone(),
                                    btype,
                                    ptype,
                                    plugin_path,
                                    label: format!("Scanning {name} ({arch}) plugins..."),
                                });
                            }
                        }
                    }
                }
            }
        }

        let total_tasks = tasks.len();
        let discovered_count = initial_plugins.len();
        let state = Arc::new(Mutex::new(SharedDiscoveryState {
            discovered: initial_plugins,
            cache_updates: Vec::new(),
            cache_lookup: if opts.check_invalid {
                HashMap::new()
            } else {
                existing_cache.clone()
            },
            current_file: String::new(),
        }));

        Self {
            tasks,
            total_tasks,
            completed_tasks: 0,
            active_handle: std::ptr::null_mut(),
            active_label: "Preparing plugin scan...".into(),
            state,
            finished: total_tasks == 0,
            cancelled: false,
            discovered_count,
        }
    }

    pub fn progress(&self) -> f32 {
        if self.total_tasks == 0 {
            1.0
        } else {
            (self.completed_tasks as f32 / self.total_tasks as f32).clamp(0.0, 1.0)
        }
    }

    pub fn status_text(&self) -> String {
        let cur = self
            .state
            .lock()
            .map(|g| g.current_file.clone())
            .unwrap_or_default();
        if cur.is_empty() {
            self.active_label.clone()
        } else {
            format!("{} ({})", self.active_label, cur)
        }
    }

    /// Step discovery slightly. Returns `true` when finished.
    pub fn step(&mut self, utils: &CarlaUtils) -> bool {
        if self.finished || self.cancelled {
            return true;
        }

        if self.active_handle.is_null() {
            if let Some(task) = self.tasks.pop_front() {
                self.active_label = task.label;
                let state_ptr = Arc::as_ptr(&self.state) as *mut c_void;
                self.active_handle = utils.discovery_start(
                    &task.tool,
                    task.btype,
                    task.ptype,
                    &task.plugin_path,
                    discovery_callback_trampoline,
                    Some(check_cache_trampoline),
                    state_ptr,
                );
                if self.active_handle.is_null() {
                    self.completed_tasks += 1;
                }
            } else {
                self.finished = true;
                return true;
            }
        }

        if !self.active_handle.is_null() {
            let keep_going = utils.discovery_idle(self.active_handle);
            if let Ok(g) = self.state.lock() {
                self.discovered_count = g.discovered.len();
            }
            if !keep_going {
                utils.discovery_stop(self.active_handle);
                self.active_handle = std::ptr::null_mut();
                self.completed_tasks += 1;
                if self.tasks.is_empty() {
                    self.finished = true;
                    return true;
                }
            }
        }

        false
    }

    pub fn skip_current(&mut self, utils: &CarlaUtils) {
        if !self.active_handle.is_null() {
            utils.discovery_skip(self.active_handle);
        }
    }

    pub fn cancel(&mut self, utils: &CarlaUtils) {
        if !self.active_handle.is_null() {
            utils.discovery_stop(self.active_handle);
            self.active_handle = std::ptr::null_mut();
        }
        self.tasks.clear();
        self.cancelled = true;
        self.finished = true;
    }

    pub fn apply_to_database(&self, db: &mut PluginDatabase) {
        if let Ok(guard) = self.state.lock() {
            db.plugins = guard.discovered.clone();
            let mut grouped: HashMap<String, (String, Vec<DiscoveredPlugin>)> = HashMap::new();
            for (key, sha, plugin) in &guard.cache_updates {
                grouped
                    .entry(key.clone())
                    .or_insert_with(|| (sha.clone(), Vec::new()))
                    .1
                    .push(plugin.clone());
            }
            for (k, v) in grouped {
                db.file_cache.insert(k, v);
            }
        }
        db.sort_and_dedup();
        let _ = db.save();
    }
}
