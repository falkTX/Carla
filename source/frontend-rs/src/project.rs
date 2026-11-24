// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::fs;
use std::path::{Path, PathBuf};

use tempfile::TempDir;

use crate::ffi::host::CarlaHost;

/// Captures a single plugin's state as a temporary single-plugin `.carxp` project
/// so Undo Remove / Redo Add can restore the plugin with all parameters, chunks, and custom data.
#[derive(Debug)]
pub struct PluginSnapshot {
    _dir: TempDir,
    carxp_path: PathBuf,
}

impl PluginSnapshot {
    pub fn capture(host: &CarlaHost, plugin_id: u32) -> Result<Self, String> {
        let dir = TempDir::with_prefix("franca-undo-").map_err(|e| e.to_string())?;
        let carxs_path = dir.path().join("effect.carxs");
        let carxs_str = carxs_path.to_string_lossy().into_owned();
        if !host.save_plugin_state(plugin_id, &carxs_str) {
            let err = host.get_last_error();
            return Err(if err.is_empty() {
                "Could not capture the effect state.".into()
            } else {
                err
            });
        }

        let preset_xml = fs::read_to_string(&carxs_path).map_err(|e| e.to_string())?;
        let wrapped = wrap_carxs_as_carxp(&preset_xml)?;
        let carxp_path = dir.path().join("effect.carxp");
        fs::write(&carxp_path, wrapped).map_err(|e| e.to_string())?;
        let _ = fs::remove_file(&carxs_path);

        Ok(Self {
            _dir: dir,
            carxp_path,
        })
    }

    pub fn carxp_path(&self) -> &Path {
        &self.carxp_path
    }
}

/// Wraps a `<CARLA-PRESET>...</CARLA-PRESET>` XML document inside
/// `<CARLA-PROJECT VERSION="2.6"><Plugin>...</Plugin></CARLA-PROJECT>`.
pub fn wrap_carxs_as_carxp(carxs_xml: &str) -> Result<String, String> {
    let start_tag_pos = carxs_xml
        .find("<CARLA-PRESET")
        .ok_or_else(|| "Invalid .carxs preset: missing <CARLA-PRESET>".to_string())?;
    let body_start = carxs_xml[start_tag_pos..]
        .find('>')
        .map(|i| start_tag_pos + i + 1)
        .ok_or_else(|| "Invalid .carxs preset: malformed opening tag".to_string())?;
    let body_end = carxs_xml
        .rfind("</CARLA-PRESET>")
        .ok_or_else(|| "Invalid .carxs preset: missing </CARLA-PRESET>".to_string())?;
    if body_end < body_start {
        return Err("Invalid .carxs preset: empty body".into());
    }
    let inner = &carxs_xml[body_start..body_end];
    Ok(format!(
        "<?xml version='1.0' encoding='UTF-8'?>\n<!DOCTYPE CARLA-PROJECT>\n<CARLA-PROJECT VERSION='2.6'>\n <Plugin>{inner}</Plugin>\n</CARLA-PROJECT>\n"
    ))
}

pub fn is_project_file(path: &Path) -> bool {
    path.extension()
        .and_then(|e| e.to_str())
        .is_some_and(|ext| ext.eq_ignore_ascii_case("carxp"))
}

pub fn is_supported_file(path: &Path, supported_exts: &[String]) -> bool {
    let Some(ext) = path.extension().and_then(|e| e.to_str()) else {
        return false;
    };
    let lower = ext.to_ascii_lowercase();
    lower == "carxp"
        || lower == "carxs"
        || supported_exts
            .iter()
            .any(|s| s.trim_start_matches('.').eq_ignore_ascii_case(&lower))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_wrap_carxs_as_carxp() {
        let sample = "<?xml version='1.0' encoding='UTF-8'?>\n<!DOCTYPE CARLA-PRESET>\n<CARLA-PRESET VERSION='2.6'>\n <Info><Type>INTERNAL</Type><Name>Test</Name></Info>\n</CARLA-PRESET>\n";
        let wrapped = wrap_carxs_as_carxp(sample).unwrap();
        assert!(wrapped.contains("<CARLA-PROJECT VERSION='2.6'>"));
        assert!(wrapped.contains("<Plugin>"));
        assert!(wrapped.contains("<Type>INTERNAL</Type>"));
        assert!(wrapped.contains("</Plugin>"));
    }
}
