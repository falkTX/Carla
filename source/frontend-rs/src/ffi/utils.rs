// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::ffi::{CString, c_char, c_int, c_uint, c_void};
use std::path::Path;

use libloading::Library;
use serde::{Deserialize, Serialize};

use super::host::{cstr_to_string, read_string_list};
use super::types::*;

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct DiscoveredPlugin {
    pub build: i32,
    pub ptype: i32,
    pub hints: u32,
    pub category: i32,
    pub filename: String,
    pub name: String,
    pub label: String,
    pub maker: String,
    pub unique_id: u64,
    pub audio_ins: u32,
    pub audio_outs: u32,
    pub cv_ins: u32,
    pub cv_outs: u32,
    pub midi_ins: u32,
    pub midi_outs: u32,
    pub parameter_ins: u32,
    pub parameter_outs: u32,
}

impl DiscoveredPlugin {
    pub fn unique_key(&self) -> String {
        format!(
            "{}:{}:{}:{}:{}",
            self.build, self.ptype, self.filename, self.label, self.unique_id
        )
    }

    pub fn is_synth(&self) -> bool {
        (self.hints & PLUGIN_IS_SYNTH) != 0 || self.category == PLUGIN_CATEGORY_SYNTH
    }

    pub fn is_midi_only(&self) -> bool {
        self.audio_ins == 0 && self.audio_outs == 0 && (self.midi_ins > 0 || self.midi_outs > 0)
    }

    pub fn is_effect(&self) -> bool {
        !self.is_synth() && self.audio_ins > 0 && self.audio_outs > 0
    }
}

/// Unpack Carla's `carla_get_cached_plugin_info(PLUGIN_LV2)` label format
/// (`"<bundle_dir>/<lv2_uri>"` or `"/<lv2_uri>"`) into `(bundle_filename, uri)`.
/// If `label` is already a valid URI (has `scheme:` before any path separator),
/// it is returned unchanged.
pub fn normalize_lv2_bundle_and_uri(filename: &str, label: &str) -> (String, String) {
    let sep_pos = label.find(['/', '\\']);
    let colon_pos = label.find(':');
    let has_bundle_prefix = match (sep_pos, colon_pos) {
        (Some(s), Some(c)) => s < c,
        (Some(0), None) => true,
        _ => false,
    };
    if has_bundle_prefix {
        if let Some(s) = sep_pos {
            let bundle = &label[..s];
            let uri = &label[s + 1..];
            let resolved_file = if filename.is_empty() {
                bundle.to_string()
            } else {
                filename.to_string()
            };
            return (resolved_file, uri.to_string());
        }
    }
    (filename.to_string(), label.to_string())
}

pub struct CarlaUtils {
    lib: Library,
}

impl CarlaUtils {
    pub fn load(lib_path: &Path) -> Result<Self, String> {
        let lib = unsafe { Library::new(lib_path) }
            .map_err(|e| format!("Failed to load {}: {e}", lib_path.display()))?;
        Ok(Self { lib })
    }

    pub fn get_complete_license_text(&self) -> String {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn() -> *const c_char> =
                self.lib.get(b"carla_get_complete_license_text\0").unwrap();
            cstr_to_string(f())
        }
    }

    pub fn get_supported_file_extensions(&self) -> Vec<String> {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn() -> *const *const c_char> = self
                .lib
                .get(b"carla_get_supported_file_extensions\0")
                .unwrap();
            read_string_list(f())
        }
    }

    pub fn get_supported_features(&self) -> Vec<String> {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn() -> *const *const c_char> =
                self.lib.get(b"carla_get_supported_features\0").unwrap();
            read_string_list(f())
        }
    }

    pub fn get_cached_plugins(&self, ptype: c_int, plugin_path: &str) -> Vec<DiscoveredPlugin> {
        let Ok(cpath) = CString::new(plugin_path) else {
            return Vec::new();
        };
        let mut out = Vec::new();
        unsafe {
            let count_fn: libloading::Symbol<unsafe extern "C" fn(c_int, *const c_char) -> c_uint> =
                self.lib.get(b"carla_get_cached_plugin_count\0").unwrap();
            let info_fn: libloading::Symbol<
                unsafe extern "C" fn(c_int, c_uint) -> *const RawCarlaCachedPluginInfo,
            > = self.lib.get(b"carla_get_cached_plugin_info\0").unwrap();

            let count = count_fn(ptype, cpath.as_ptr());
            for i in 0..count {
                let ptr = info_fn(ptype, i);
                if ptr.is_null() {
                    continue;
                }
                let r = &*ptr;
                if !r.valid {
                    continue;
                }
                let name = cstr_to_string(r.name);
                let raw_label = cstr_to_string(r.label);
                let (filename, label) = match ptype {
                    PLUGIN_LV2 => normalize_lv2_bundle_and_uri("", &raw_label),
                    PLUGIN_SFZ => (raw_label, name.clone()),
                    _ => (String::new(), raw_label),
                };
                out.push(DiscoveredPlugin {
                    build: BINARY_NATIVE,
                    ptype,
                    hints: r.hints,
                    category: r.category,
                    filename,
                    name,
                    label,
                    maker: cstr_to_string(r.maker),
                    unique_id: 0,
                    audio_ins: r.audio_ins,
                    audio_outs: r.audio_outs,
                    cv_ins: r.cv_ins,
                    cv_outs: r.cv_outs,
                    midi_ins: r.midi_ins,
                    midi_outs: r.midi_outs,
                    parameter_ins: r.parameter_ins,
                    parameter_outs: r.parameter_outs,
                });
            }
        }
        out
    }

    pub fn set_discovery_option(&self, option: c_int, value: i32, value_str: &str) {
        let cstr = CString::new(value_str).unwrap_or_else(|_| CString::new("").unwrap());
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(c_int, c_int, *const c_char)> = self
                .lib
                .get(b"carla_plugin_discovery_set_option\0")
                .unwrap();
            f(option, value, cstr.as_ptr());
        }
    }

    pub fn discovery_start(
        &self,
        tool: &str,
        btype: c_int,
        ptype: c_int,
        plugin_path: &str,
        discovery_cb: CarlaPluginDiscoveryCallback,
        check_cache_cb: Option<CarlaPluginCheckCacheCallback>,
        callback_ptr: *mut c_void,
    ) -> CarlaPluginDiscoveryHandle {
        let Ok(ctool) = CString::new(tool) else {
            return std::ptr::null_mut();
        };
        let Ok(cpath) = CString::new(plugin_path) else {
            return std::ptr::null_mut();
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(
                    *const c_char,
                    c_int,
                    c_int,
                    *const c_char,
                    CarlaPluginDiscoveryCallback,
                    Option<CarlaPluginCheckCacheCallback>,
                    *mut c_void,
                ) -> CarlaPluginDiscoveryHandle,
            > = self.lib.get(b"carla_plugin_discovery_start\0").unwrap();
            f(
                ctool.as_ptr(),
                btype,
                ptype,
                cpath.as_ptr(),
                discovery_cb,
                check_cache_cb,
                callback_ptr,
            )
        }
    }

    pub fn discovery_idle(&self, handle: CarlaPluginDiscoveryHandle) -> bool {
        if handle.is_null() {
            return false;
        }
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaPluginDiscoveryHandle) -> bool> =
                self.lib.get(b"carla_plugin_discovery_idle\0").unwrap();
            f(handle)
        }
    }

    pub fn discovery_skip(&self, handle: CarlaPluginDiscoveryHandle) {
        if handle.is_null() {
            return;
        }
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaPluginDiscoveryHandle)> =
                self.lib.get(b"carla_plugin_discovery_skip\0").unwrap();
            f(handle);
        }
    }

    pub fn discovery_stop(&self, handle: CarlaPluginDiscoveryHandle) {
        if handle.is_null() {
            return;
        }
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaPluginDiscoveryHandle)> =
                self.lib.get(b"carla_plugin_discovery_stop\0").unwrap();
            f(handle);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lv2_bundle_uri_normalization() {
        assert_eq!(
            normalize_lv2_bundle_and_uri("", "ZL Equalizer 2.lv2/https://github.com/ZL-Audio/ZLEqualizer2"),
            (
                "ZL Equalizer 2.lv2".to_string(),
                "https://github.com/ZL-Audio/ZLEqualizer2".to_string()
            )
        );
        assert_eq!(
            normalize_lv2_bundle_and_uri("", "/http://calf.sourceforge.net/plugins/Reverb"),
            (
                "".to_string(),
                "http://calf.sourceforge.net/plugins/Reverb".to_string()
            )
        );
        assert_eq!(
            normalize_lv2_bundle_and_uri("", "foo.lv2/urn:ardour:a-eq"),
            ("foo.lv2".to_string(), "urn:ardour:a-eq".to_string())
        );
        assert_eq!(
            normalize_lv2_bundle_and_uri(
                "/usr/lib/lv2/foo.lv2",
                "https://github.com/ZL-Audio/ZLEqualizer2"
            ),
            (
                "/usr/lib/lv2/foo.lv2".to_string(),
                "https://github.com/ZL-Audio/ZLEqualizer2".to_string()
            )
        );
    }
}
