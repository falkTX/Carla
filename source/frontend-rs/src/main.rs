// SPDX-License-Identifier: GPL-2.0-or-later

mod discovery;
mod ffi;
mod history;
mod logging;
mod patchbay;
mod project;
mod settings;
mod ui;
mod widgets;

use std::path::PathBuf;
use std::sync::atomic::Ordering;

use eframe::egui;

use crate::ffi::host::CarlaHost;
use crate::ffi::utils::CarlaUtils;
use crate::ffi::{lib_filename, resolve_carla_paths};
use crate::ui::app::{FrancaApp, SIG_QUIT_REQUESTED, SIG_SAVE_REQUESTED};

#[cfg(unix)]
extern "C" fn handle_unix_signal(sig: libc::c_int) {
    match sig {
        libc::SIGINT | libc::SIGTERM => {
            SIG_QUIT_REQUESTED.store(true, Ordering::SeqCst);
        }
        libc::SIGUSR1 => {
            SIG_SAVE_REQUESTED.store(true, Ordering::SeqCst);
        }
        _ => {}
    }
}

#[cfg(unix)]
fn install_signal_handlers() {
    unsafe {
        let handler = handle_unix_signal as *const () as libc::sighandler_t;
        libc::signal(libc::SIGINT, handler);
        libc::signal(libc::SIGTERM, handler);
        libc::signal(libc::SIGUSR1, handler);
    }
}

fn load_app_icon() -> Option<egui::IconData> {
    let icon_bytes = include_bytes!("../../../resources/256x256/franca.png");
    let img = image::load_from_memory(icon_bytes).ok()?.to_rgba8();
    let (width, height) = img.dimensions();
    Some(egui::IconData {
        rgba: img.into_raw(),
        width,
        height,
    })
}

fn parse_cli_args() -> Option<PathBuf> {
    let mut project = None;
    for arg in std::env::args().skip(1) {
        match arg.as_str() {
            "-h" | "--help" => {
                println!("Franca — Native Rust Audio Plugin Host");
                println!("Usage: franca [OPTIONS] [PROJECT.carxp]");
                println!("  -h, --help       Show this help message");
                println!("  -v, --version    Show version information");
                std::process::exit(0);
            }
            "-v" | "--version" => {
                println!("Franca {} (Rust UI)", crate::ffi::types::CARLA_VERSION_STRING);
                std::process::exit(0);
            }
            s if s.starts_with("--with-appname=") || s.starts_with("--with-libprefix=") => {}
            s if s.starts_with('-') => {}
            s => {
                if project.is_none() {
                    project = Some(PathBuf::from(s));
                }
            }
        }
    }
    project
}

fn main() -> eframe::Result<()> {
    #[cfg(unix)]
    install_signal_handlers();

    let initial_project = parse_cli_args();
    let _log_file = logging::init_logging(None);
    let (bin_dir, res_dir) = resolve_carla_paths();
    logging::log_info(&format!(
        "Resolved Carla paths: bin={}, resources={}",
        bin_dir.display(),
        res_dir.display()
    ));

    let standalone_path = bin_dir.join(lib_filename("carla_standalone2"));
    let host = match CarlaHost::load(&standalone_path) {
        Ok(h) => h,
        Err(e) => {
            logging::log_error(&e);
            eprintln!("Franca fatal error: {e}");
            std::process::exit(1);
        }
    };

    let utils_path = bin_dir.join(lib_filename("carla_utils"));
    let utils = match CarlaUtils::load(&utils_path) {
        Ok(u) => Some(u),
        Err(e) => {
            logging::log_warn(&format!("Could not load carla_utils: {e}"));
            None
        }
    };

    let mut viewport = egui::ViewportBuilder::default()
        .with_title("Franca")
        .with_inner_size([1180.0, 760.0])
        .with_min_inner_size([720.0, 480.0]);
    if let Some(icon) = load_app_icon() {
        viewport = viewport.with_icon(icon);
    }

    let options = eframe::NativeOptions {
        viewport,
        ..Default::default()
    };

    eframe::run_native(
        "Franca",
        options,
        Box::new(move |_cc| {
            Ok(Box::new(FrancaApp::new(
                host,
                utils,
                bin_dir,
                res_dir,
                initial_project,
            )))
        }),
    )
}
