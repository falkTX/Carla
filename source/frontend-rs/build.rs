// SPDX-License-Identifier: GPL-2.0-or-later

use cxx_qt_build::{CxxQtBuilder, QmlModule};

fn main() {
    println!("cargo::rerun-if-changed=cpp/qt_boot.h");
    println!("cargo::rerun-if-changed=cpp/qt_boot.cpp");
    println!("cargo::rerun-if-changed=qml/resources.qrc");
    println!("cargo::rerun-if-changed=../../resources/256x256/franca.png");

    let qml_files = [
        "qml/Main.qml",
        "qml/RackView.qml",
        "qml/PatchbayView.qml",
        "qml/Minimap.qml",
        "qml/PeakMeter.qml",
        "qml/ClipWarning.qml",
        "qml/PianoKeyboard.qml",
        "qml/PluginEditWindow.qml",
        "qml/AddPluginDialog.qml",
        "qml/SettingsDialog.qml",
        "qml/RuntimeDriverDialog.qml",
        "qml/SidePanel.qml",
        "qml/LogsTab.qml",
        "qml/Dialogs.qml",
    ];
    for f in &qml_files {
        println!("cargo::rerun-if-changed={f}");
    }

    let qml_module = QmlModule::new("org.franca.app")
        .version(1, 0)
        .qml_files(qml_files);

    CxxQtBuilder::new_qml_module(qml_module)
        .qt_module("Gui")
        .qt_module("Widgets")
        .qt_module("Quick")
        .qt_module("QuickControls2")
        .qrc("qml/resources.qrc")
        .cpp_file("cpp/qt_boot.cpp")
        .files(["src/qt_bridge.rs"])
        .build();
}
