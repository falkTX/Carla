// SPDX-License-Identifier: GPL-2.0-or-later

mod controller;
mod discovery;
mod ffi;
mod history;
mod logging;
mod model;
mod patchbay;
mod project;
mod qt_bridge;
mod settings;

use std::path::PathBuf;
use std::sync::atomic::Ordering;

use crate::controller::{INITIAL_PROJECT, SIG_QUIT_REQUESTED, SIG_SAVE_REQUESTED};

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

fn parse_cli_args() -> Option<PathBuf> {
    let mut project = None;
    for arg in std::env::args().skip(1) {
        match arg.as_str() {
            "-h" | "--help" => {
                println!("Franca — Native Rust + KDE Kirigami Audio Plugin Host");
                println!("Usage: franca [OPTIONS] [PROJECT.carxp]");
                println!("  -h, --help       Show this help message");
                println!("  -v, --version    Show version information");
                std::process::exit(0);
            }
            "-v" | "--version" => {
                println!(
                    "Franca {} (Rust + KDE Qt6/Kirigami UI)",
                    crate::ffi::types::CARLA_VERSION_STRING
                );
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

fn main() {
    #[cfg(unix)]
    install_signal_handlers();

    let initial_project = parse_cli_args();
    if let Ok(mut g) = INITIAL_PROJECT.lock() {
        *g = initial_project;
    }

    let _log_file = logging::init_logging(None);
    cxx_qt::init_qml_module!("org.franca.app");

    let code = qt_bridge::qobject::run_qt_app();
    std::process::exit(code);
}
