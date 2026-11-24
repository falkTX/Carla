// SPDX-License-Identifier: GPL-2.0-or-later

use std::fs::{self, File, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::{Mutex, OnceLock};

use chrono::Local;

pub const DEFAULT_MAX_BYTES: u64 = 2 * 1024 * 1024; // 2 MiB per file
pub const DEFAULT_BACKUP_COUNT: usize = 5;          // ~12 MiB total cap

pub struct RotatingLogger {
    log_dir: PathBuf,
    log_file: PathBuf,
    max_bytes: u64,
    backup_count: usize,
    file: Option<File>,
    current_size: u64,
    ui_lines: Vec<String>,
}

impl RotatingLogger {
    pub fn new(log_dir: PathBuf, max_bytes: u64, backup_count: usize) -> Self {
        let _ = fs::create_dir_all(&log_dir);
        let log_file = log_dir.join("franca.log");
        let current_size = fs::metadata(&log_file).map(|m| m.len()).unwrap_or(0);
        let file = OpenOptions::new()
            .create(true)
            .append(true)
            .open(&log_file)
            .ok();
        Self {
            log_dir,
            log_file,
            max_bytes,
            backup_count,
            file,
            current_size,
            ui_lines: Vec::new(),
        }
    }

    pub fn log_dir(&self) -> &Path {
        &self.log_dir
    }

    pub fn log_file(&self) -> &Path {
        &self.log_file
    }

    pub fn ui_lines(&self) -> &[String] {
        &self.ui_lines
    }

    pub fn clear_ui_lines(&mut self) {
        self.ui_lines.clear();
        self.append_ui("======= Logs cleared ========");
    }

    pub fn write_line(&mut self, level: &str, msg: &str, mirror_to_ui: bool) {
        let ts = Local::now().format("%Y-%m-%d %H:%M:%S%.3f");
        for line in msg.lines() {
            let trimmed = line.trim_end();
            if trimmed.is_empty() {
                continue;
            }
            let formatted = format!("{ts} [{level}] {trimmed}\n");
            let bytes = formatted.as_bytes();
            if self.current_size + bytes.len() as u64 > self.max_bytes {
                self.rotate();
            }
            if let Some(ref mut f) = self.file {
                if f.write_all(bytes).is_ok() {
                    let _ = f.flush();
                    self.current_size += bytes.len() as u64;
                }
            }
            if mirror_to_ui {
                self.ui_lines.push(trimmed.to_string());
                if self.ui_lines.len() > 5000 {
                    let drain = self.ui_lines.len() - 4000;
                    self.ui_lines.drain(0..drain);
                }
            }
        }
    }

    pub fn append_ui(&mut self, msg: &str) {
        self.write_line("INFO", msg, true);
    }

    fn rotate(&mut self) {
        self.file = None;
        if self.backup_count > 0 {
            let oldest = self.log_dir.join(format!("franca.log.{}", self.backup_count));
            let _ = fs::remove_file(oldest);
            for i in (1..self.backup_count).rev() {
                let src = self.log_dir.join(format!("franca.log.{i}"));
                let dst = self.log_dir.join(format!("franca.log.{}", i + 1));
                if src.exists() {
                    let _ = fs::rename(src, dst);
                }
            }
            if self.log_file.exists() {
                let first_backup = self.log_dir.join("franca.log.1");
                let _ = fs::rename(&self.log_file, first_backup);
            }
        } else {
            let _ = fs::remove_file(&self.log_file);
        }
        self.file = OpenOptions::new()
            .create(true)
            .write(true)
            .truncate(true)
            .open(&self.log_file)
            .ok();
        self.current_size = 0;
    }
}

static GLOBAL_LOGGER: OnceLock<Mutex<RotatingLogger>> = OnceLock::new();

pub fn default_log_dir() -> PathBuf {
    if let Ok(env_dir) = std::env::var("FRANCA_LOG_DIR") {
        if !env_dir.is_empty() {
            return PathBuf::from(env_dir);
        }
    }
    let state_base = std::env::var_os("XDG_STATE_HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|| crate::ffi::dirs_home().join(".local").join("state"));
    state_base.join("franca").join("logs")
}

pub fn init_logging(custom_dir: Option<PathBuf>) -> PathBuf {
    let dir = custom_dir.unwrap_or_else(default_log_dir);
    let logger = RotatingLogger::new(dir.clone(), DEFAULT_MAX_BYTES, DEFAULT_BACKUP_COUNT);
    let log_file = logger.log_file().to_path_buf();
    let _ = GLOBAL_LOGGER.set(Mutex::new(logger));

    let crash_dir = dir.clone();
    let prev_hook = std::panic::take_hook();
    std::panic::set_hook(Box::new(move |info| {
        let ts = Local::now().format("%Y-%m-%d %H:%M:%S%.3f");
        let msg = format!("{ts} [PANIC] {info}\n");
        log_error(&format!("PANIC: {info}"));
        let crash_path = crash_dir.join("franca-crash.log");
        if let Ok(mut f) = OpenOptions::new().create(true).append(true).open(crash_path) {
            let _ = f.write_all(msg.as_bytes());
        }
        prev_hook(info);
    }));

    log_info(&format!(
        "======= Franca Rust UI started (pid={}, os={}/{}) =======",
        std::process::id(),
        std::env::consts::OS,
        std::env::consts::ARCH
    ));
    log_ui(&format!("Session log file: {}", log_file.display()));
    log_file
}

pub fn log_info(msg: &str) {
    if let Some(m) = GLOBAL_LOGGER.get() {
        if let Ok(mut g) = m.lock() {
            g.write_line("INFO", msg, false);
        }
    }
}

pub fn log_warn(msg: &str) {
    if let Some(m) = GLOBAL_LOGGER.get() {
        if let Ok(mut g) = m.lock() {
            g.write_line("WARN", msg, false);
        }
    }
}

pub fn log_error(msg: &str) {
    if let Some(m) = GLOBAL_LOGGER.get() {
        if let Ok(mut g) = m.lock() {
            g.write_line("ERROR", msg, true);
        }
    }
}

pub fn log_ui(msg: &str) {
    if let Some(m) = GLOBAL_LOGGER.get() {
        if let Ok(mut g) = m.lock() {
            g.append_ui(msg);
        }
    }
}

pub fn ui_lines_snapshot() -> Vec<String> {
    GLOBAL_LOGGER
        .get()
        .and_then(|m| m.lock().ok())
        .map(|g| g.ui_lines().to_vec())
        .unwrap_or_default()
}

pub fn clear_ui_logs() {
    if let Some(m) = GLOBAL_LOGGER.get() {
        if let Ok(mut g) = m.lock() {
            g.clear_ui_lines();
        }
    }
}

pub fn current_log_dir() -> PathBuf {
    GLOBAL_LOGGER
        .get()
        .and_then(|m| m.lock().ok())
        .map(|g| g.log_dir().to_path_buf())
        .unwrap_or_else(default_log_dir)
}

pub fn log_shutdown() {
    log_info(&format!(
        "======= Franca Rust UI ended (pid={}) =======",
        std::process::id()
    ));
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_rotating_logger_caps_size() {
        let dir = tempfile::tempdir().unwrap();
        let mut logger = RotatingLogger::new(dir.path().to_path_buf(), 1024, 2);
        for i in 0..120 {
            logger.write_line("INFO", &format!("Entry #{i} {}", "x".repeat(40)), false);
        }
        let entries: Vec<_> = fs::read_dir(dir.path())
            .unwrap()
            .filter_map(|e| e.ok())
            .collect();
        assert!(entries.len() >= 2 && entries.len() <= 3);
        let total: u64 = entries
            .iter()
            .map(|e| e.metadata().unwrap().len())
            .sum();
        assert!(total < 6 * 1024);
    }
}
