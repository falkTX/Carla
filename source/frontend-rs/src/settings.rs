// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::collections::{BTreeMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::ffi::dirs_home;
use crate::ffi::host::CarlaHost;
use crate::ffi::types::*;

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
#[serde(default)]
pub struct DriverDeviceSettings {
    pub device: String,
    pub buffer_size: u32,
    pub sample_rate: u32,
    pub triple_buffer: bool,
}

impl Default for DriverDeviceSettings {
    fn default() -> Self {
        Self {
            device: String::new(),
            buffer_size: 512,
            sample_rate: 44100,
            triple_buffer: false,
        }
    }
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(default)]
pub struct FrancaSettings {
    // Main
    pub project_folder: String,
    pub refresh_interval_ms: u32,
    pub confirm_exit: bool,
    pub show_logs: bool,
    pub experimental: bool,
    pub dark_theme: bool,

    // Canvas
    pub canvas_theme: String,
    pub canvas_width: u32,
    pub canvas_height: u32,
    pub canvas_use_bezier: bool,
    pub canvas_auto_hide_groups: bool,
    pub canvas_auto_select_items: bool,
    pub canvas_eye_candy: bool,
    pub canvas_inline_displays: bool,

    // Engine
    pub audio_driver: String,
    pub process_mode: i32,
    pub transport_mode: i32,
    pub transport_extra: String,
    pub force_stereo: bool,
    pub prefer_plugin_bridges: bool,
    pub prefer_ui_bridges: bool,
    pub manage_uis: bool,
    pub uis_always_on_top: bool,
    pub max_parameters: u32,
    pub reset_xruns: bool,
    pub ui_bridges_timeout: u32,
    pub driver_settings: BTreeMap<String, DriverDeviceSettings>,

    // OSC
    pub osc_enabled: bool,
    pub osc_tcp_enabled: bool,
    pub osc_tcp_port: i32,
    pub osc_tcp_random: bool,
    pub osc_udp_enabled: bool,
    pub osc_udp_port: i32,
    pub osc_udp_random: bool,

    // Paths
    pub paths_audio: Vec<String>,
    pub paths_midi: Vec<String>,
    pub paths_ladspa: Vec<String>,
    pub paths_dssi: Vec<String>,
    pub paths_lv2: Vec<String>,
    pub paths_vst2: Vec<String>,
    pub paths_vst3: Vec<String>,
    pub paths_clap: Vec<String>,
    pub paths_sf2: Vec<String>,
    pub paths_sfz: Vec<String>,
    pub paths_jsfx: Vec<String>,

    // Wine
    pub wine_executable: String,
    pub wine_auto_prefix: bool,
    pub wine_fallback_prefix: String,
    pub wine_rt_prio_enabled: bool,
    pub wine_base_rt_prio: i32,
    pub wine_server_rt_prio: i32,

    // Experimental
    pub exp_plugin_bridges: bool,
    pub exp_wine_bridges: bool,
    pub exp_jack_apps: bool,
    pub exp_export_lv2: bool,
    pub exp_prevent_bad_behaviour: bool,
    pub exp_load_lib_global: bool,

    // UI layout & bookmarks
    pub show_toolbar: bool,
    pub show_side_panel: bool,
    pub show_meters: bool,
    pub show_keyboard: bool,
    pub disk_folders: Vec<String>,
    pub favorite_plugins: HashSet<String>,
}

fn default_audio_driver() -> String {
    if Path::new("/usr/bin/jackd").exists()
        || Path::new("/usr/bin/jackdbus").exists()
        || Path::new("/usr/bin/pw-jack").exists()
    {
        "JACK".to_string()
    } else {
        "PulseAudio".to_string()
    }
}

fn config_home() -> PathBuf {
    std::env::var_os("XDG_CONFIG_HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|| dirs_home().join(".config"))
}

pub fn default_ladspa_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.ladspa"),
        "/usr/lib/ladspa".into(),
        "/usr/local/lib/ladspa".into(),
    ]
}

pub fn default_dssi_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.dssi"),
        "/usr/lib/dssi".into(),
        "/usr/local/lib/dssi".into(),
    ]
}

pub fn default_lv2_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.lv2"),
        "/usr/lib/lv2".into(),
        "/usr/local/lib/lv2".into(),
    ]
}

pub fn default_vst2_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.vst"),
        "/usr/lib/vst".into(),
        "/usr/local/lib/vst".into(),
        format!("{home}/.lxvst"),
        "/usr/lib/lxvst".into(),
        "/usr/local/lib/lxvst".into(),
    ]
}

pub fn default_vst3_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.vst3"),
        "/usr/lib/vst3".into(),
        "/usr/local/lib/vst3".into(),
    ]
}

pub fn default_clap_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.clap"),
        "/usr/lib/clap".into(),
        "/usr/local/lib/clap".into(),
    ]
}

pub fn default_sf2_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.sounds/sf2"),
        format!("{home}/.sounds/sf3"),
        "/usr/share/sounds/sf2".into(),
        "/usr/share/sounds/sf3".into(),
        "/usr/share/soundfonts".into(),
    ]
}

pub fn default_sfz_paths() -> Vec<String> {
    let home = dirs_home().to_string_lossy().into_owned();
    vec![
        format!("{home}/.sounds/sfz"),
        "/usr/share/sounds/sfz".into(),
    ]
}

pub fn default_jsfx_paths() -> Vec<String> {
    let cfg = config_home().to_string_lossy().into_owned();
    vec![format!("{cfg}/REAPER/Effects")]
}

impl Default for FrancaSettings {
    fn default() -> Self {
        let home = dirs_home().to_string_lossy().into_owned();
        let driver = default_audio_driver();
        let (process_mode, transport_mode) = if driver == "JACK" {
            (
                ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
                ENGINE_TRANSPORT_MODE_JACK,
            )
        } else {
            (
                ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
                ENGINE_TRANSPORT_MODE_INTERNAL,
            )
        };
        Self {
            project_folder: home.clone(),
            refresh_interval_ms: 20,
            confirm_exit: true,
            show_logs: true,
            experimental: false,
            dark_theme: true,

            canvas_theme: "Modern Dark".into(),
            canvas_width: 3100,
            canvas_height: 2400,
            canvas_use_bezier: true,
            canvas_auto_hide_groups: true,
            canvas_auto_select_items: false,
            canvas_eye_candy: true,
            canvas_inline_displays: false,

            audio_driver: driver,
            process_mode,
            transport_mode,
            transport_extra: String::new(),
            force_stereo: false,
            prefer_plugin_bridges: false,
            prefer_ui_bridges: true,
            manage_uis: true,
            uis_always_on_top: false,
            max_parameters: MAX_DEFAULT_PARAMETERS,
            reset_xruns: false,
            ui_bridges_timeout: 4000,
            driver_settings: BTreeMap::new(),

            osc_enabled: false,
            osc_tcp_enabled: true,
            osc_tcp_port: 22752,
            osc_tcp_random: false,
            osc_udp_enabled: true,
            osc_udp_port: 22752,
            osc_udp_random: false,

            paths_audio: Vec::new(),
            paths_midi: Vec::new(),
            paths_ladspa: default_ladspa_paths(),
            paths_dssi: default_dssi_paths(),
            paths_lv2: default_lv2_paths(),
            paths_vst2: default_vst2_paths(),
            paths_vst3: default_vst3_paths(),
            paths_clap: default_clap_paths(),
            paths_sf2: default_sf2_paths(),
            paths_sfz: default_sfz_paths(),
            paths_jsfx: default_jsfx_paths(),

            wine_executable: "wine".into(),
            wine_auto_prefix: true,
            wine_fallback_prefix: format!("{home}/.wine"),
            wine_rt_prio_enabled: true,
            wine_base_rt_prio: 15,
            wine_server_rt_prio: 10,

            exp_plugin_bridges: false,
            exp_wine_bridges: false,
            exp_jack_apps: false,
            exp_export_lv2: false,
            exp_prevent_bad_behaviour: false,
            exp_load_lib_global: false,

            show_toolbar: true,
            show_side_panel: false,
            show_meters: false,
            show_keyboard: false,
            disk_folders: Vec::new(),
            favorite_plugins: HashSet::new(),
        }
    }
}

impl FrancaSettings {
    pub fn config_path() -> PathBuf {
        config_home().join("franca").join("settings.json")
    }

    pub fn load() -> Self {
        let path = Self::config_path();
        if let Ok(raw) = fs::read_to_string(&path) {
            if let Ok(settings) = serde_json::from_str::<FrancaSettings>(&raw) {
                return settings;
            }
        }
        let mut settings = Self::default();
        // Best-effort one-time import from legacy Carla2.conf
        let legacy = config_home().join("falkTX").join("Carla2.conf");
        if legacy.exists() {
            settings.import_carla2_conf(&legacy);
        }
        settings
    }

    pub fn save(&self) -> Result<(), String> {
        let path = Self::config_path();
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent).map_err(|e| e.to_string())?;
        }
        let json = serde_json::to_string_pretty(self).map_err(|e| e.to_string())?;
        fs::write(&path, json).map_err(|e| e.to_string())
    }

    pub fn driver_config(&self, driver: &str) -> DriverDeviceSettings {
        self.driver_settings
            .get(driver)
            .cloned()
            .unwrap_or_default()
    }

    pub fn driver_config_mut(&mut self, driver: &str) -> &mut DriverDeviceSettings {
        self.driver_settings
            .entry(driver.to_string())
            .or_default()
    }

    pub fn plugin_path_joined(&self, ptype: i32) -> String {
        let sep = if cfg!(windows) { ";" } else { ":" };
        let list: &[String] = match ptype {
            PLUGIN_LADSPA => &self.paths_ladspa,
            PLUGIN_DSSI => &self.paths_dssi,
            PLUGIN_LV2 => &self.paths_lv2,
            PLUGIN_VST2 => &self.paths_vst2,
            PLUGIN_VST3 => &self.paths_vst3,
            PLUGIN_CLAP => &self.paths_clap,
            PLUGIN_SF2 => &self.paths_sf2,
            PLUGIN_SFZ => &self.paths_sfz,
            PLUGIN_JSFX => &self.paths_jsfx,
            _ => return String::new(),
        };
        list.join(sep)
    }

    pub fn apply_to_host(&self, host: &CarlaHost, bin_dir: &Path, res_dir: &Path) {
        let sep = if cfg!(windows) { ";" } else { ":" };

        host.set_engine_option(
            ENGINE_OPTION_PATH_BINARIES,
            0,
            &bin_dir.to_string_lossy(),
        );
        host.set_engine_option(
            ENGINE_OPTION_PATH_RESOURCES,
            0,
            &res_dir.to_string_lossy(),
        );
        host.set_engine_option(ENGINE_OPTION_PROCESS_MODE, self.process_mode, "");
        let effective_transport = if self.audio_driver != "JACK"
            && self.transport_mode == ENGINE_TRANSPORT_MODE_JACK
        {
            ENGINE_TRANSPORT_MODE_INTERNAL
        } else {
            self.transport_mode
        };
        host.set_engine_option(
            ENGINE_OPTION_TRANSPORT_MODE,
            effective_transport,
            &self.transport_extra,
        );
        host.set_engine_option(
            ENGINE_OPTION_FORCE_STEREO,
            i32::from(self.force_stereo),
            "",
        );
        host.set_engine_option(
            ENGINE_OPTION_PREFER_PLUGIN_BRIDGES,
            i32::from(self.prefer_plugin_bridges && self.exp_plugin_bridges),
            "",
        );
        host.set_engine_option(
            ENGINE_OPTION_PREFER_UI_BRIDGES,
            i32::from(self.prefer_ui_bridges),
            "",
        );
        host.set_engine_option(
            ENGINE_OPTION_UIS_ALWAYS_ON_TOP,
            i32::from(self.uis_always_on_top),
            "",
        );
        host.set_engine_option(ENGINE_OPTION_MAX_PARAMETERS, self.max_parameters as i32, "");
        host.set_engine_option(ENGINE_OPTION_RESET_XRUNS, i32::from(self.reset_xruns), "");
        host.set_engine_option(
            ENGINE_OPTION_UI_BRIDGES_TIMEOUT,
            self.ui_bridges_timeout as i32,
            "",
        );

        let drv = self.driver_config(&self.audio_driver);
        host.set_engine_option(ENGINE_OPTION_AUDIO_DRIVER, 0, &self.audio_driver);
        host.set_engine_option(ENGINE_OPTION_AUDIO_DEVICE, 0, &drv.device);
        host.set_engine_option(ENGINE_OPTION_AUDIO_BUFFER_SIZE, drv.buffer_size as i32, "");
        host.set_engine_option(ENGINE_OPTION_AUDIO_SAMPLE_RATE, drv.sample_rate as i32, "");
        host.set_engine_option(
            ENGINE_OPTION_AUDIO_TRIPLE_BUFFER,
            i32::from(drv.triple_buffer),
            "",
        );

        host.set_engine_option(ENGINE_OPTION_OSC_ENABLED, i32::from(self.osc_enabled), "");
        if self.osc_enabled {
            let tcp = if !self.osc_tcp_enabled {
                -1
            } else if self.osc_tcp_random {
                0
            } else {
                self.osc_tcp_port
            };
            let udp = if !self.osc_udp_enabled {
                -1
            } else if self.osc_udp_random {
                0
            } else {
                self.osc_udp_port
            };
            host.set_engine_option(ENGINE_OPTION_OSC_PORT_TCP, tcp, "");
            host.set_engine_option(ENGINE_OPTION_OSC_PORT_UDP, udp, "");
        }

        host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_AUDIO, &self.paths_audio.join(sep));
        host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_MIDI, &self.paths_midi.join(sep));
        for ptype in [
            PLUGIN_LADSPA,
            PLUGIN_DSSI,
            PLUGIN_LV2,
            PLUGIN_VST2,
            PLUGIN_VST3,
            PLUGIN_CLAP,
            PLUGIN_SF2,
            PLUGIN_SFZ,
            PLUGIN_JSFX,
        ] {
            host.set_engine_option(
                ENGINE_OPTION_PLUGIN_PATH,
                ptype,
                &self.plugin_path_joined(ptype),
            );
        }

        host.set_engine_option(
            ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR,
            i32::from(self.experimental && self.exp_prevent_bad_behaviour),
            "",
        );
        host.set_engine_option(ENGINE_OPTION_WINE_EXECUTABLE, 0, &self.wine_executable);
        host.set_engine_option(
            ENGINE_OPTION_WINE_AUTO_PREFIX,
            i32::from(self.wine_auto_prefix),
            "",
        );
        host.set_engine_option(
            ENGINE_OPTION_WINE_FALLBACK_PREFIX,
            0,
            &self.wine_fallback_prefix,
        );
        host.set_engine_option(
            ENGINE_OPTION_WINE_RT_PRIO_ENABLED,
            i32::from(self.wine_rt_prio_enabled),
            "",
        );
        host.set_engine_option(ENGINE_OPTION_WINE_BASE_RT_PRIO, self.wine_base_rt_prio, "");
        host.set_engine_option(
            ENGINE_OPTION_WINE_SERVER_RT_PRIO,
            self.wine_server_rt_prio,
            "",
        );
    }

    pub fn import_carla2_conf(&mut self, path: &Path) {
        let Ok(content) = fs::read_to_string(path) else {
            return;
        };
        let mut section = String::new();
        for raw_line in content.lines() {
            let line = raw_line.trim();
            if line.is_empty() || line.starts_with('#') || line.starts_with(';') {
                continue;
            }
            if line.starts_with('[') && line.ends_with(']') {
                section = line[1..line.len() - 1].to_string();
                continue;
            }
            let Some((k, v)) = line.split_once('=') else {
                continue;
            };
            let key = k.trim();
            let val = v.trim();
            match (section.as_str(), key) {
                ("Main", "ProjectFolder") if !val.is_empty() => {
                    self.project_folder = val.to_string()
                }
                ("Main", "ConfirmExit") => self.confirm_exit = val.eq_ignore_ascii_case("true"),
                ("Main", "ShowLogs") => self.show_logs = val.eq_ignore_ascii_case("true"),
                ("Main", "Experimental") => self.experimental = val.eq_ignore_ascii_case("true"),
                ("Main", "RefreshInterval") => {
                    if let Ok(n) = val.parse::<u32>() {
                        self.refresh_interval_ms = n.clamp(10, 200);
                    }
                }
                ("Canvas", "UseBezierLines") => {
                    self.canvas_use_bezier = val.eq_ignore_ascii_case("true")
                }
                ("Canvas", "AutoHideGroups") => {
                    self.canvas_auto_hide_groups = val.eq_ignore_ascii_case("true")
                }
                ("Canvas", "AutoSelectItems") => {
                    self.canvas_auto_select_items = val.eq_ignore_ascii_case("true")
                }
                ("Canvas", "Size") => {
                    if let Some((w, h)) = val.split_once('x') {
                        if let (Ok(w), Ok(h)) = (w.parse::<u32>(), h.parse::<u32>()) {
                            self.canvas_width = w.clamp(800, 10000);
                            self.canvas_height = h.clamp(600, 10000);
                        }
                    }
                }
                ("Engine", "AudioDriver") if !val.is_empty() => {
                    self.audio_driver = val.to_string()
                }
                ("Engine", "ProcessMode") => {
                    if let Ok(n) = val.parse::<i32>() {
                        self.process_mode = n;
                    }
                }
                ("Engine", "TransportMode") => {
                    if let Ok(n) = val.parse::<i32>() {
                        self.transport_mode = n;
                    }
                }
                ("Engine", "ForceStereo") => self.force_stereo = val.eq_ignore_ascii_case("true"),
                ("Engine", "PreferUiBridges") => {
                    self.prefer_ui_bridges = val.eq_ignore_ascii_case("true")
                }
                ("Engine", "MaxParameters") => {
                    if let Ok(n) = val.parse::<u32>() {
                        self.max_parameters = n.clamp(16, 4096);
                    }
                }
                ("Paths", "LADSPA") => self.paths_ladspa = split_qsettings_list(val),
                ("Paths", "DSSI") => self.paths_dssi = split_qsettings_list(val),
                ("Paths", "LV2") => self.paths_lv2 = split_qsettings_list(val),
                ("Paths", "VST2") => self.paths_vst2 = split_qsettings_list(val),
                ("Paths", "VST3") => self.paths_vst3 = split_qsettings_list(val),
                ("Paths", "CLAP") => self.paths_clap = split_qsettings_list(val),
                ("Paths", "SF2") => self.paths_sf2 = split_qsettings_list(val),
                ("Paths", "SFZ") => self.paths_sfz = split_qsettings_list(val),
                ("Paths", "JSFX") => self.paths_jsfx = split_qsettings_list(val),
                _ => {
                    if section == "Engine" && key.starts_with("Driver-") {
                        let rest = &key["Driver-".len()..];
                        if let Some((drv, field)) = rest.rsplit_once('/') {
                            let cfg = self.driver_config_mut(drv);
                            match field {
                                "Device" => cfg.device = val.to_string(),
                                "BufferSize" => {
                                    if let Ok(n) = val.parse::<u32>() {
                                        cfg.buffer_size = n;
                                    }
                                }
                                "SampleRate" => {
                                    if let Ok(n) = val.parse::<u32>() {
                                        cfg.sample_rate = n;
                                    }
                                }
                                "TripleBuffer" => {
                                    cfg.triple_buffer = val.eq_ignore_ascii_case("true");
                                }
                                _ => {}
                            }
                        }
                    }
                }
            }
        }
    }
}

fn split_qsettings_list(val: &str) -> Vec<String> {
    val.split(',')
        .map(|s| s.trim().trim_matches('"').to_string())
        .filter(|s| !s.is_empty())
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_import_carla2_conf() {
        let dir = tempfile::tempdir().unwrap();
        let conf = dir.path().join("Carla2.conf");
        fs::write(
            &conf,
            "[Main]\nConfirmExit=false\nRefreshInterval=30\n[Engine]\nAudioDriver=Dummy\nProcessMode=2\nDriver-Dummy/BufferSize=256\nDriver-Dummy/SampleRate=48000\n[Paths]\nVST3=/a/vst3, /b/vst3\n",
        )
        .unwrap();
        let mut s = FrancaSettings::default();
        s.import_carla2_conf(&conf);
        assert!(!s.confirm_exit);
        assert_eq!(s.refresh_interval_ms, 30);
        assert_eq!(s.audio_driver, "Dummy");
        assert_eq!(s.process_mode, 2);
        assert_eq!(s.driver_config("Dummy").buffer_size, 256);
        assert_eq!(s.driver_config("Dummy").sample_rate, 48000);
        assert_eq!(s.paths_vst3, vec!["/a/vst3", "/b/vst3"]);
    }
}
