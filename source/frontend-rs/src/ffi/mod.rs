// SPDX-License-Identifier: GPL-2.0-or-later

pub mod host;
pub mod types;
pub mod utils;

use std::path::{Path, PathBuf};

/// Resolve the directories containing `libcarla_standalone2.so` / `libcarla_utils.so`
/// and `resources/` for both source checkouts (`bin/`) and installed layouts.
pub fn resolve_carla_paths() -> (PathBuf, PathBuf) {
    // 1. Explicit environment override
    if let Ok(dir) = std::env::var("FRANCA_LIBDIR") {
        let bin_dir = PathBuf::from(dir);
        let res_dir = std::env::var("FRANCA_DATADIR")
            .map(|d| PathBuf::from(d).join("resources"))
            .unwrap_or_else(|_| bin_dir.join("resources"));
        if bin_dir.join(lib_filename("carla_standalone2")).exists() {
            return (bin_dir, res_dir);
        }
    }

    // 2. Next to the running executable (e.g. bin/ or ~/.local/lib/franca/)
    if let Ok(exe) = std::env::current_exe() {
        if let Some(exe_dir) = exe.parent() {
            if exe_dir.join(lib_filename("carla_standalone2")).exists() {
                return (exe_dir.to_path_buf(), exe_dir.join("resources"));
            }
            // Typical Linux install: <prefix>/bin/franca -> <prefix>/lib/franca
            if let Some(prefix) = exe_dir.parent() {
                for lib_sub in ["lib/franca", "lib64/franca", "lib/carla"] {
                    let cand = prefix.join(lib_sub);
                    if cand.join(lib_filename("carla_standalone2")).exists() {
                        let res = prefix.join("share/franca/resources");
                        let res = if res.exists() { res } else { cand.join("resources") };
                        return (cand, res);
                    }
                }
            }
            // Cargo target dir inside source/frontend-rs/target/{debug,release}/
            let mut cur: Option<&Path> = Some(exe_dir);
            for _ in 0..6 {
                if let Some(p) = cur {
                    let cand = p.join("bin");
                    if cand.join(lib_filename("carla_standalone2")).exists() {
                        return (cand.clone(), cand.join("resources"));
                    }
                    cur = p.parent();
                }
            }
        }
    }

    // 3. Relative to current working directory
    if let Ok(cwd) = std::env::current_dir() {
        for cand in [cwd.join("bin"), cwd.join("../bin"), cwd.join("../../bin")] {
            if cand.join(lib_filename("carla_standalone2")).exists() {
                let canon = cand.canonicalize().unwrap_or(cand);
                return (canon.clone(), canon.join("resources"));
            }
        }
    }

    // 4. Standard system locations
    for prefix in [
        dirs_home().join(".local"),
        PathBuf::from("/usr/local"),
        PathBuf::from("/usr"),
    ] {
        for lib_sub in ["lib/franca", "lib64/franca", "lib/carla"] {
            let cand = prefix.join(lib_sub);
            if cand.join(lib_filename("carla_standalone2")).exists() {
                let res = prefix.join("share/franca/resources");
                let res = if res.exists() { res } else { cand.join("resources") };
                return (cand, res);
            }
        }
    }

    (PathBuf::from("bin"), PathBuf::from("bin/resources"))
}

pub fn lib_filename(stem: &str) -> String {
    #[cfg(target_os = "macos")]
    {
        format!("lib{stem}.dylib")
    }
    #[cfg(target_os = "windows")]
    {
        format!("lib{stem}.dll")
    }
    #[cfg(not(any(target_os = "macos", target_os = "windows")))]
    {
        format!("lib{stem}.so")
    }
}

pub fn dirs_home() -> PathBuf {
    std::env::var_os("HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from("."))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ffi::host::CarlaHost;
    use crate::ffi::types::*;
    use crate::ffi::utils::CarlaUtils;
    use crate::project::PluginSnapshot;

    #[test]
    fn test_carla_utils_and_dummy_engine_roundtrip() {
        let (bin_dir, res_dir) = resolve_carla_paths();
        let standalone = bin_dir.join(lib_filename("carla_standalone2"));
        let utils_lib = bin_dir.join(lib_filename("carla_utils"));
        assert!(standalone.exists(), "missing {}", standalone.display());
        assert!(utils_lib.exists(), "missing {}", utils_lib.display());

        // Verify CarlaUtils
        let utils = CarlaUtils::load(&utils_lib).expect("load libcarla_utils");
        let features = utils.get_supported_features();
        assert!(!features.is_empty());
        let exts = utils.get_supported_file_extensions();
        assert!(!exts.is_empty());
        let internals = utils.get_cached_plugins(PLUGIN_INTERNAL, "");
        assert!(!internals.is_empty(), "expected internal Carla plugins");

        // Verify CarlaHost Dummy engine lifecycle + plugin + custom category + snapshot + project save/load
        let host = CarlaHost::load(&standalone).expect("load libcarla_standalone2");
        host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, &bin_dir.to_string_lossy());
        host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, &res_dir.to_string_lossy());
        host.set_engine_option(
            ENGINE_OPTION_PROCESS_MODE,
            ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
            "",
        );
        assert!(
            host.engine_init("Dummy", "FrancaRustTest"),
            "engine_init failed: {}",
            host.get_last_error()
        );
        assert!(host.is_engine_running());

        // Add internal Audio Gain plugin
        let gain = internals
            .iter()
            .find(|p| p.label == "audiogain_s" || p.label == "audiogain")
            .unwrap_or(&internals[0]);
        assert!(
            host.add_plugin(
                BINARY_NATIVE,
                PLUGIN_INTERNAL,
                "",
                Some("Test Gain"),
                &gain.label,
                0,
                PLUGIN_OPTIONS_NULL,
            ),
            "add_plugin failed: {}",
            host.get_last_error()
        );
        assert_eq!(host.get_current_plugin_count(), 1);
        assert_eq!(host.get_plugin_info(0).name, "Test Gain");

        // Toggle bypass/active
        host.set_active(0, false);
        assert!(host.get_internal_parameter_value(0, PARAMETER_ACTIVE) < 0.5);
        host.set_active(0, true);
        assert!(host.get_internal_parameter_value(0, PARAMETER_ACTIVE) >= 0.5);

        // Persist FrancaCategory custom data and verify snapshot + project save/load round-trip
        host.set_custom_data(0, CUSTOM_DATA_TYPE_PROPERTY, "FrancaCategory", "dynamics");
        assert_eq!(
            host.get_custom_data_value(0, CUSTOM_DATA_TYPE_PROPERTY, "FrancaCategory"),
            "dynamics"
        );

        let snap = PluginSnapshot::capture(&host, 0).expect("snapshot plugin");
        assert!(snap.carxp_path().exists());

        let tmp = tempfile::tempdir().unwrap();
        let proj = tmp.path().join("roundtrip.carxp");
        assert!(host.save_project(&proj.to_string_lossy()));
        assert!(host.remove_plugin(0));
        assert_eq!(host.get_current_plugin_count(), 0);

        // Restore via snapshot (Undo Remove flow)
        assert!(host.load_file(&snap.carxp_path().to_string_lossy()));
        assert_eq!(host.get_current_plugin_count(), 1);
        assert_eq!(
            host.get_custom_data_value(0, CUSTOM_DATA_TYPE_PROPERTY, "FrancaCategory"),
            "dynamics"
        );

        // Full project reload
        assert!(host.remove_all_plugins());
        assert_eq!(host.get_current_plugin_count(), 0);
        assert!(host.load_project(&proj.to_string_lossy()));
        assert_eq!(host.get_current_plugin_count(), 1);
        assert_eq!(host.get_plugin_info(0).name, "Test Gain");
        assert_eq!(
            host.get_custom_data_value(0, CUSTOM_DATA_TYPE_PROPERTY, "FrancaCategory"),
            "dynamics"
        );

        assert!(host.set_engine_about_to_close());
        assert!(host.engine_close());
    }
}
