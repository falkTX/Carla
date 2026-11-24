// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code)]

use std::ffi::{CStr, CString, c_char, c_double, c_float, c_int, c_uint, c_void};
use std::path::Path;
use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
use std::sync::mpsc::{self, Receiver, Sender};

use libloading::Library;

use super::types::*;

unsafe extern "C" {
    fn franca_qt_set_cancelable_action(
        started: bool,
        message: *const c_char,
        cancel_fn: Option<unsafe extern "C" fn(*mut c_void)>,
        cancel_ctx: *mut c_void,
    );
    fn franca_qt_update_cancelable_progress(plugins_loaded: c_int);
    fn franca_qt_process_events();
}

#[derive(Clone, Debug)]
pub struct EngineEvent {
    pub action: c_int,
    pub plugin_id: u32,
    pub value1: i32,
    pub value2: i32,
    pub value3: i32,
    pub valuef: f32,
    pub value_str: String,
}

#[derive(Clone, Debug, Default)]
pub struct PluginInfo {
    pub ptype: c_int,
    pub category: c_int,
    pub hints: u32,
    pub options_available: u32,
    pub options_enabled: u32,
    pub filename: String,
    pub name: String,
    pub label: String,
    pub maker: String,
    pub copyright: String,
    pub icon_name: String,
    pub unique_id: i64,
}

#[derive(Clone, Debug, Default)]
pub struct ParameterInfo {
    pub name: String,
    pub symbol: String,
    pub unit: String,
    pub comment: String,
    pub group_name: String,
    pub scale_point_count: u32,
}

#[derive(Clone, Debug)]
pub struct ScalePointInfo {
    pub value: f32,
    pub label: String,
}

#[derive(Clone, Debug)]
pub struct MidiProgramInfo {
    pub bank: u32,
    pub program: u32,
    pub name: String,
}

#[derive(Clone, Debug, Default)]
pub struct DriverDeviceInfo {
    pub hints: u32,
    pub buffer_sizes: Vec<u32>,
    pub sample_rates: Vec<f64>,
}

#[derive(Clone, Debug, Default)]
pub struct RuntimeDriverDeviceInfo {
    pub name: String,
    pub hints: u32,
    pub buffer_size: u32,
    pub buffer_sizes: Vec<u32>,
    pub sample_rate: f64,
    pub sample_rates: Vec<f64>,
}

struct CallbackState {
    tx: Sender<EngineEvent>,
    file_result: std::sync::Mutex<Option<CString>>,
    handle: CarlaHostHandle,
    cancel_engine_fn: Option<unsafe extern "C" fn(CarlaHostHandle)>,
    cancelable_active: AtomicBool,
    cancelable_plugins_loaded: AtomicI32,
}

unsafe extern "C" fn cancel_dialog_trampoline(ctx: *mut c_void) {
    if ctx.is_null() {
        return;
    }
    let state = unsafe { &*(ctx as *const CallbackState) };
    if let Some(cancel_fn) = state.cancel_engine_fn {
        unsafe { cancel_fn(state.handle) };
    }
}

unsafe extern "C" fn engine_callback_trampoline(
    ptr: *mut c_void,
    action: c_int,
    plugin_id: c_uint,
    value1: c_int,
    value2: c_int,
    value3: c_int,
    valuef: c_float,
    value_str: *const c_char,
) {
    if ptr.is_null() {
        return;
    }
    let state = unsafe { &*(ptr as *const CallbackState) };

    // Handle live progress dialog updates & Qt event pumping during synchronous
    // backend operations (such as carla_load_project / carla_add_plugin).
    match action {
        ENGINE_CALLBACK_CANCELABLE_ACTION => {
            let started = value1 != 0;
            state.cancelable_active.store(started, Ordering::SeqCst);
            state.cancelable_plugins_loaded.store(0, Ordering::SeqCst);
            unsafe {
                franca_qt_set_cancelable_action(
                    started,
                    value_str,
                    Some(cancel_dialog_trampoline),
                    ptr,
                );
            }
        }
        ENGINE_CALLBACK_PLUGIN_ADDED => {
            if state.cancelable_active.load(Ordering::SeqCst) {
                let count = state
                    .cancelable_plugins_loaded
                    .fetch_add(1, Ordering::SeqCst)
                    + 1;
                unsafe { franca_qt_update_cancelable_progress(count) };
            }
        }
        ENGINE_CALLBACK_IDLE => {
            if state.cancelable_active.load(Ordering::SeqCst) {
                unsafe { franca_qt_process_events() };
            }
            return;
        }
        _ => {}
    }

    let text = unsafe { cstr_to_string(value_str) };
    let _ = state.tx.send(EngineEvent {
        action,
        plugin_id,
        value1,
        value2,
        value3,
        valuef,
        value_str: text,
    });
}

unsafe extern "C" fn file_callback_trampoline(
    ptr: *mut c_void,
    action: c_int,
    is_dir: bool,
    title: *const c_char,
    _filter: *const c_char,
) -> *const c_char {
    if ptr.is_null() {
        return std::ptr::null();
    }
    let state = unsafe { &*(ptr as *const CallbackState) };
    let title_str = unsafe { cstr_to_string(title) };

    let chosen: Option<String> = match action {
        FILE_CALLBACK_OPEN => {
            if is_dir {
                rfd::FileDialog::new()
                    .set_title(&title_str)
                    .pick_folder()
                    .map(|p| p.to_string_lossy().into_owned())
            } else {
                rfd::FileDialog::new()
                    .set_title(&title_str)
                    .pick_file()
                    .map(|p| p.to_string_lossy().into_owned())
            }
        }
        FILE_CALLBACK_SAVE => rfd::FileDialog::new()
            .set_title(&title_str)
            .save_file()
            .map(|p| p.to_string_lossy().into_owned()),
        FILE_CALLBACK_CONFIRM_PROJECT_JACK_APPS => {
            let answer = rfd::MessageDialog::new()
                .set_title("Launch JACK Applications?")
                .set_description(&format!(
                    "The project '{title_str}' contains JACK applications. Allow launching them?"
                ))
                .set_buttons(rfd::MessageButtons::YesNo)
                .show();
            if matches!(answer, rfd::MessageDialogResult::Yes) {
                Some("yes".to_string())
            } else {
                None
            }
        }
        _ => None,
    };

    if let Ok(mut guard) = state.file_result.lock() {
        *guard = chosen.and_then(|s| CString::new(s).ok());
        guard.as_ref().map_or(std::ptr::null(), |c| c.as_ptr())
    } else {
        std::ptr::null()
    }
}

pub unsafe fn cstr_to_string(ptr: *const c_char) -> String {
    if ptr.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(ptr) }
            .to_string_lossy()
            .into_owned()
    }
}

pub unsafe fn read_string_list(ptr: *const *const c_char) -> Vec<String> {
    let mut out = Vec::new();
    if ptr.is_null() {
        return out;
    }
    let mut idx = 0usize;
    loop {
        let item = unsafe { *ptr.add(idx) };
        if item.is_null() {
            break;
        }
        out.push(unsafe { cstr_to_string(item) });
        idx += 1;
    }
    out
}

pub struct CarlaHost {
    lib: Library,
    handle: CarlaHostHandle,
    rx: Receiver<EngineEvent>,
    _cb_state: Box<CallbackState>,
}

impl CarlaHost {
    pub fn load(lib_path: &Path) -> Result<Self, String> {
        let lib = unsafe { Library::new(lib_path) }
            .map_err(|e| format!("Failed to load {}: {e}", lib_path.display()))?;

        let init_fn: libloading::Symbol<unsafe extern "C" fn() -> CarlaHostHandle> = unsafe {
            lib.get(b"carla_standalone_host_init\0")
                .map_err(|e| format!("Missing carla_standalone_host_init: {e}"))?
        };
        let handle = unsafe { init_fn() };
        if handle.is_null() {
            return Err("carla_standalone_host_init returned NULL".into());
        }

        let cancel_engine_fn: Option<unsafe extern "C" fn(CarlaHostHandle)> = unsafe {
            lib.get::<unsafe extern "C" fn(CarlaHostHandle)>(b"carla_cancel_engine_action\0")
                .ok()
                .map(|sym| *sym)
        };

        let (tx, rx) = mpsc::channel();
        let cb_state = Box::new(CallbackState {
            tx,
            file_result: std::sync::Mutex::new(None),
            handle,
            cancel_engine_fn,
            cancelable_active: AtomicBool::new(false),
            cancelable_plugins_loaded: AtomicI32::new(0),
        });
        let cb_ptr = (&*cb_state) as *const CallbackState as *mut c_void;

        unsafe {
            let set_cb: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, EngineCallbackFunc, *mut c_void),
            > = lib
                .get(b"carla_set_engine_callback\0")
                .map_err(|e| format!("Missing carla_set_engine_callback: {e}"))?;
            set_cb(handle, engine_callback_trampoline, cb_ptr);

            let set_fcb: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, FileCallbackFunc, *mut c_void),
            > = lib
                .get(b"carla_set_file_callback\0")
                .map_err(|e| format!("Missing carla_set_file_callback: {e}"))?;
            set_fcb(handle, file_callback_trampoline, cb_ptr);
        }

        Ok(Self {
            lib,
            handle,
            rx,
            _cb_state: cb_state,
        })
    }

    pub fn drain_events(&self) -> Vec<EngineEvent> {
        let mut out = Vec::new();
        while let Ok(ev) = self.rx.try_recv() {
            out.push(ev);
        }
        out
    }

    pub fn get_engine_driver_count(&self) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn() -> c_uint> =
                self.lib.get(b"carla_get_engine_driver_count\0").unwrap();
            f()
        }
    }

    pub fn get_engine_driver_name(&self, index: u32) -> String {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(c_uint) -> *const c_char> =
                self.lib.get(b"carla_get_engine_driver_name\0").unwrap();
            cstr_to_string(f(index))
        }
    }

    pub fn get_engine_driver_device_names(&self, index: u32) -> Vec<String> {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(c_uint) -> *const *const c_char> =
                self.lib.get(b"carla_get_engine_driver_device_names\0").unwrap();
            read_string_list(f(index))
        }
    }

    pub fn get_engine_driver_device_info(&self, index: u32, name: &str) -> DriverDeviceInfo {
        let Ok(cname) = CString::new(name) else {
            return DriverDeviceInfo::default();
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(c_uint, *const c_char) -> *const RawEngineDriverDeviceInfo,
            > = self.lib.get(b"carla_get_engine_driver_device_info\0").unwrap();
            let ptr = f(index, cname.as_ptr());
            if ptr.is_null() {
                return DriverDeviceInfo::default();
            }
            let raw = &*ptr;
            let mut buffer_sizes = Vec::new();
            if !raw.buffer_sizes.is_null() {
                let mut i = 0;
                loop {
                    let v = *raw.buffer_sizes.add(i);
                    if v == 0 {
                        break;
                    }
                    buffer_sizes.push(v);
                    i += 1;
                }
            }
            let mut sample_rates = Vec::new();
            if !raw.sample_rates.is_null() {
                let mut i = 0;
                loop {
                    let v = *raw.sample_rates.add(i);
                    if v <= 0.0 {
                        break;
                    }
                    sample_rates.push(v);
                    i += 1;
                }
            }
            DriverDeviceInfo {
                hints: raw.hints,
                buffer_sizes,
                sample_rates,
            }
        }
    }

    pub fn show_engine_driver_device_control_panel(&self, index: u32, name: &str) -> bool {
        let Ok(cname) = CString::new(name) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(c_uint, *const c_char) -> bool> = self
                .lib
                .get(b"carla_show_engine_driver_device_control_panel\0")
                .unwrap();
            f(index, cname.as_ptr())
        }
    }

    pub fn set_engine_option(&self, option: c_int, value: i32, value_str: &str) {
        let cstr = CString::new(value_str).unwrap_or_else(|_| CString::new("").unwrap());
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_int, c_int, *const c_char),
            > = self.lib.get(b"carla_set_engine_option\0").unwrap();
            f(self.handle, option, value, cstr.as_ptr());
        }
    }

    pub fn engine_init(&self, driver_name: &str, client_name: &str) -> bool {
        let Ok(d) = CString::new(driver_name) else {
            return false;
        };
        let Ok(c) = CString::new(client_name) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, *const c_char, *const c_char) -> bool,
            > = self.lib.get(b"carla_engine_init\0").unwrap();
            f(self.handle, d.as_ptr(), c.as_ptr())
        }
    }

    pub fn engine_close(&self) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> bool> =
                self.lib.get(b"carla_engine_close\0").unwrap();
            f(self.handle)
        }
    }

    pub fn engine_idle(&self) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle)> =
                self.lib.get(b"carla_engine_idle\0").unwrap();
            f(self.handle);
        }
    }

    pub fn is_engine_running(&self) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> bool> =
                self.lib.get(b"carla_is_engine_running\0").unwrap();
            f(self.handle)
        }
    }

    pub fn get_runtime_engine_info(&self) -> CarlaRuntimeEngineInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle) -> *const CarlaRuntimeEngineInfo,
            > = self.lib.get(b"carla_get_runtime_engine_info\0").unwrap();
            let ptr = f(self.handle);
            if ptr.is_null() {
                CarlaRuntimeEngineInfo::default()
            } else {
                *ptr
            }
        }
    }

    pub fn get_runtime_engine_driver_device_info(&self) -> RuntimeDriverDeviceInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(
                    CarlaHostHandle,
                ) -> *const RawCarlaRuntimeEngineDriverDeviceInfo,
            > = self
                .lib
                .get(b"carla_get_runtime_engine_driver_device_info\0")
                .unwrap();
            let ptr = f(self.handle);
            if ptr.is_null() {
                return RuntimeDriverDeviceInfo::default();
            }
            let raw = &*ptr;
            let mut buffer_sizes = Vec::new();
            if !raw.buffer_sizes.is_null() {
                let mut i = 0;
                loop {
                    let v = *raw.buffer_sizes.add(i);
                    if v == 0 {
                        break;
                    }
                    buffer_sizes.push(v);
                    i += 1;
                }
            }
            let mut sample_rates = Vec::new();
            if !raw.sample_rates.is_null() {
                let mut i = 0;
                loop {
                    let v = *raw.sample_rates.add(i);
                    if v <= 0.0 {
                        break;
                    }
                    sample_rates.push(v);
                    i += 1;
                }
            }
            RuntimeDriverDeviceInfo {
                name: cstr_to_string(raw.name),
                hints: raw.hints,
                buffer_size: raw.buffer_size,
                buffer_sizes,
                sample_rate: raw.sample_rate,
                sample_rates,
            }
        }
    }

    pub fn set_engine_buffer_size_and_sample_rate(&self, buffer_size: u32, sample_rate: f64) -> bool {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, c_double) -> bool,
            > = self
                .lib
                .get(b"carla_set_engine_buffer_size_and_sample_rate\0")
                .unwrap();
            f(self.handle, buffer_size, sample_rate)
        }
    }

    pub fn show_engine_device_control_panel(&self) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> bool> = self
                .lib
                .get(b"carla_show_engine_device_control_panel\0")
                .unwrap();
            f(self.handle)
        }
    }

    pub fn clear_engine_xruns(&self) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle)> =
                self.lib.get(b"carla_clear_engine_xruns\0").unwrap();
            f(self.handle);
        }
    }

    pub fn cancel_engine_action(&self) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle)> =
                self.lib.get(b"carla_cancel_engine_action\0").unwrap();
            f(self.handle);
        }
    }

    pub fn finish_cancelable_ui(&self) {
        if self._cb_state.cancelable_active.swap(false, Ordering::SeqCst) {
            self._cb_state
                .cancelable_plugins_loaded
                .store(0, Ordering::SeqCst);
            unsafe {
                franca_qt_set_cancelable_action(false, std::ptr::null(), None, std::ptr::null_mut());
            }
        }
    }

    pub fn set_engine_about_to_close(&self) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> bool> =
                self.lib.get(b"carla_set_engine_about_to_close\0").unwrap();
            f(self.handle)
        }
    }

    pub fn load_file(&self, filename: &str) -> bool {
        let Ok(c) = CString::new(filename) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, *const c_char) -> bool> =
                self.lib.get(b"carla_load_file\0").unwrap();
            f(self.handle, c.as_ptr())
        }
    }

    pub fn load_project(&self, filename: &str) -> bool {
        let Ok(c) = CString::new(filename) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, *const c_char) -> bool> =
                self.lib.get(b"carla_load_project\0").unwrap();
            f(self.handle, c.as_ptr())
        }
    }

    pub fn save_project(&self, filename: &str) -> bool {
        let Ok(c) = CString::new(filename) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, *const c_char) -> bool> =
                self.lib.get(b"carla_save_project\0").unwrap();
            f(self.handle, c.as_ptr())
        }
    }

    pub fn clear_project_filename(&self) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle)> =
                self.lib.get(b"carla_clear_project_filename\0").unwrap();
            f(self.handle);
        }
    }

    pub fn patchbay_connect(
        &self,
        external: bool,
        group_a: u32,
        port_a: u32,
        group_b: u32,
        port_b: u32,
    ) -> bool {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, bool, c_uint, c_uint, c_uint, c_uint) -> bool,
            > = self.lib.get(b"carla_patchbay_connect\0").unwrap();
            f(self.handle, external, group_a, port_a, group_b, port_b)
        }
    }

    pub fn patchbay_disconnect(&self, external: bool, connection_id: u32) -> bool {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, bool, c_uint) -> bool,
            > = self.lib.get(b"carla_patchbay_disconnect\0").unwrap();
            f(self.handle, external, connection_id)
        }
    }

    pub fn patchbay_set_group_pos(
        &self,
        external: bool,
        group_id: u32,
        x1: i32,
        y1: i32,
        x2: i32,
        y2: i32,
    ) -> bool {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(
                    CarlaHostHandle,
                    bool,
                    c_uint,
                    c_int,
                    c_int,
                    c_int,
                    c_int,
                ) -> bool,
            > = self.lib.get(b"carla_patchbay_set_group_pos\0").unwrap();
            f(self.handle, external, group_id, x1, y1, x2, y2)
        }
    }

    pub fn patchbay_refresh(&self, external: bool) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, bool) -> bool> =
                self.lib.get(b"carla_patchbay_refresh\0").unwrap();
            f(self.handle, external)
        }
    }

    pub fn transport_play(&self) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle)> =
                self.lib.get(b"carla_transport_play\0").unwrap();
            f(self.handle);
        }
    }

    pub fn transport_pause(&self) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle)> =
                self.lib.get(b"carla_transport_pause\0").unwrap();
            f(self.handle);
        }
    }

    pub fn transport_bpm(&self, bpm: f64) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_double)> =
                self.lib.get(b"carla_transport_bpm\0").unwrap();
            f(self.handle, bpm);
        }
    }

    pub fn transport_relocate(&self, frame: u64) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, u64)> =
                self.lib.get(b"carla_transport_relocate\0").unwrap();
            f(self.handle, frame);
        }
    }

    pub fn get_transport_info(&self) -> CarlaTransportInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle) -> *const CarlaTransportInfo,
            > = self.lib.get(b"carla_get_transport_info\0").unwrap();
            let ptr = f(self.handle);
            if ptr.is_null() {
                CarlaTransportInfo::default()
            } else {
                *ptr
            }
        }
    }

    pub fn get_current_plugin_count(&self) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> u32> =
                self.lib.get(b"carla_get_current_plugin_count\0").unwrap();
            f(self.handle)
        }
    }

    pub fn get_max_plugin_number(&self) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> u32> =
                self.lib.get(b"carla_get_max_plugin_number\0").unwrap();
            f(self.handle)
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub fn add_plugin(
        &self,
        btype: c_int,
        ptype: c_int,
        filename: &str,
        name: Option<&str>,
        label: &str,
        unique_id: i64,
        options: u32,
    ) -> bool {
        let cf = CString::new(filename).unwrap_or_else(|_| CString::new("").unwrap());
        let cn = name.and_then(|n| CString::new(n).ok());
        let cl = CString::new(label).unwrap_or_else(|_| CString::new("").unwrap());
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(
                    CarlaHostHandle,
                    c_int,
                    c_int,
                    *const c_char,
                    *const c_char,
                    *const c_char,
                    i64,
                    *const c_void,
                    c_uint,
                ) -> bool,
            > = self.lib.get(b"carla_add_plugin\0").unwrap();
            f(
                self.handle,
                btype,
                ptype,
                cf.as_ptr(),
                cn.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
                cl.as_ptr(),
                unique_id,
                std::ptr::null(),
                options,
            )
        }
    }

    pub fn remove_plugin(&self, plugin_id: u32) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> bool> =
                self.lib.get(b"carla_remove_plugin\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn remove_all_plugins(&self) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> bool> =
                self.lib.get(b"carla_remove_all_plugins\0").unwrap();
            f(self.handle)
        }
    }

    pub fn rename_plugin(&self, plugin_id: u32, new_name: &str) -> bool {
        let Ok(c) = CString::new(new_name) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, *const c_char) -> bool,
            > = self.lib.get(b"carla_rename_plugin\0").unwrap();
            f(self.handle, plugin_id, c.as_ptr())
        }
    }

    pub fn clone_plugin(&self, plugin_id: u32) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> bool> =
                self.lib.get(b"carla_clone_plugin\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn replace_plugin(&self, plugin_id: u32) -> bool {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> bool> =
                self.lib.get(b"carla_replace_plugin\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn switch_plugins(&self, id_a: u32, id_b: u32) -> bool {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, c_uint) -> bool,
            > = self.lib.get(b"carla_switch_plugins\0").unwrap();
            f(self.handle, id_a, id_b)
        }
    }

    pub fn load_plugin_state(&self, plugin_id: u32, filename: &str) -> bool {
        let Ok(c) = CString::new(filename) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, *const c_char) -> bool,
            > = self.lib.get(b"carla_load_plugin_state\0").unwrap();
            f(self.handle, plugin_id, c.as_ptr())
        }
    }

    pub fn save_plugin_state(&self, plugin_id: u32, filename: &str) -> bool {
        let Ok(c) = CString::new(filename) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, *const c_char) -> bool,
            > = self.lib.get(b"carla_save_plugin_state\0").unwrap();
            f(self.handle, plugin_id, c.as_ptr())
        }
    }

    pub fn export_plugin_lv2(&self, plugin_id: u32, lv2_path: &str) -> bool {
        let Ok(c) = CString::new(lv2_path) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, *const c_char) -> bool,
            > = self.lib.get(b"carla_export_plugin_lv2\0").unwrap();
            f(self.handle, plugin_id, c.as_ptr())
        }
    }

    pub fn get_plugin_info(&self, plugin_id: u32) -> PluginInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint) -> *const RawCarlaPluginInfo,
            > = self.lib.get(b"carla_get_plugin_info\0").unwrap();
            let ptr = f(self.handle, plugin_id);
            if ptr.is_null() {
                return PluginInfo::default();
            }
            let r = &*ptr;
            PluginInfo {
                ptype: r.ptype,
                category: r.category,
                hints: r.hints,
                options_available: r.options_available,
                options_enabled: r.options_enabled,
                filename: cstr_to_string(r.filename),
                name: cstr_to_string(r.name),
                label: cstr_to_string(r.label),
                maker: cstr_to_string(r.maker),
                copyright: cstr_to_string(r.copyright),
                icon_name: cstr_to_string(r.icon_name),
                unique_id: r.unique_id,
            }
        }
    }

    pub fn get_audio_port_count_info(&self, plugin_id: u32) -> CarlaPortCountInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint) -> *const CarlaPortCountInfo,
            > = self.lib.get(b"carla_get_audio_port_count_info\0").unwrap();
            let ptr = f(self.handle, plugin_id);
            if ptr.is_null() {
                CarlaPortCountInfo::default()
            } else {
                *ptr
            }
        }
    }

    pub fn get_midi_port_count_info(&self, plugin_id: u32) -> CarlaPortCountInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint) -> *const CarlaPortCountInfo,
            > = self.lib.get(b"carla_get_midi_port_count_info\0").unwrap();
            let ptr = f(self.handle, plugin_id);
            if ptr.is_null() {
                CarlaPortCountInfo::default()
            } else {
                *ptr
            }
        }
    }

    pub fn get_parameter_count_info(&self, plugin_id: u32) -> CarlaPortCountInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint) -> *const CarlaPortCountInfo,
            > = self.lib.get(b"carla_get_parameter_count_info\0").unwrap();
            let ptr = f(self.handle, plugin_id);
            if ptr.is_null() {
                CarlaPortCountInfo::default()
            } else {
                *ptr
            }
        }
    }

    pub fn get_parameter_count(&self, plugin_id: u32) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> u32> =
                self.lib.get(b"carla_get_parameter_count\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn get_parameter_info(&self, plugin_id: u32, param_id: u32) -> ParameterInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> *const RawCarlaParameterInfo,
            > = self.lib.get(b"carla_get_parameter_info\0").unwrap();
            let ptr = f(self.handle, plugin_id, param_id);
            if ptr.is_null() {
                return ParameterInfo::default();
            }
            let r = &*ptr;
            ParameterInfo {
                name: cstr_to_string(r.name),
                symbol: cstr_to_string(r.symbol),
                unit: cstr_to_string(r.unit),
                comment: cstr_to_string(r.comment),
                group_name: cstr_to_string(r.group_name),
                scale_point_count: r.scale_point_count,
            }
        }
    }

    pub fn get_parameter_scalepoint_info(
        &self,
        plugin_id: u32,
        param_id: u32,
        sp_id: u32,
    ) -> ScalePointInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(
                    CarlaHostHandle,
                    c_uint,
                    u32,
                    u32,
                ) -> *const RawCarlaScalePointInfo,
            > = self
                .lib
                .get(b"carla_get_parameter_scalepoint_info\0")
                .unwrap();
            let ptr = f(self.handle, plugin_id, param_id, sp_id);
            if ptr.is_null() {
                return ScalePointInfo {
                    value: 0.0,
                    label: String::new(),
                };
            }
            let r = &*ptr;
            ScalePointInfo {
                value: r.value,
                label: cstr_to_string(r.label),
            }
        }
    }

    pub fn get_parameter_data(&self, plugin_id: u32, param_id: u32) -> ParameterData {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> *const ParameterData,
            > = self.lib.get(b"carla_get_parameter_data\0").unwrap();
            let ptr = f(self.handle, plugin_id, param_id);
            if ptr.is_null() {
                ParameterData::default()
            } else {
                *ptr
            }
        }
    }

    pub fn get_parameter_ranges(&self, plugin_id: u32, param_id: u32) -> ParameterRanges {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> *const ParameterRanges,
            > = self.lib.get(b"carla_get_parameter_ranges\0").unwrap();
            let ptr = f(self.handle, plugin_id, param_id);
            if ptr.is_null() {
                ParameterRanges::default()
            } else {
                *ptr
            }
        }
    }

    pub fn get_parameter_text(&self, plugin_id: u32, param_id: u32) -> String {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> *const c_char,
            > = self.lib.get(b"carla_get_parameter_text\0").unwrap();
            cstr_to_string(f(self.handle, plugin_id, param_id))
        }
    }

    pub fn get_default_parameter_value(&self, plugin_id: u32, param_id: u32) -> f32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> f32> =
                self.lib.get(b"carla_get_default_parameter_value\0").unwrap();
            f(self.handle, plugin_id, param_id)
        }
    }

    pub fn get_current_parameter_value(&self, plugin_id: u32, param_id: u32) -> f32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> f32> =
                self.lib.get(b"carla_get_current_parameter_value\0").unwrap();
            f(self.handle, plugin_id, param_id)
        }
    }

    pub fn get_internal_parameter_value(&self, plugin_id: u32, param_id: i32) -> f32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, i32) -> f32> =
                self.lib
                    .get(b"carla_get_internal_parameter_value\0")
                    .unwrap();
            f(self.handle, plugin_id, param_id)
        }
    }

    pub fn get_program_count(&self, plugin_id: u32) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> u32> =
                self.lib.get(b"carla_get_program_count\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn get_program_name(&self, plugin_id: u32, program_id: u32) -> String {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> *const c_char,
            > = self.lib.get(b"carla_get_program_name\0").unwrap();
            cstr_to_string(f(self.handle, plugin_id, program_id))
        }
    }

    pub fn get_current_program_index(&self, plugin_id: u32) -> i32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> i32> =
                self.lib.get(b"carla_get_current_program_index\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn get_midi_program_count(&self, plugin_id: u32) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> u32> =
                self.lib.get(b"carla_get_midi_program_count\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn get_midi_program_data(&self, plugin_id: u32, idx: u32) -> MidiProgramInfo {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32) -> *const RawMidiProgramData,
            > = self.lib.get(b"carla_get_midi_program_data\0").unwrap();
            let ptr = f(self.handle, plugin_id, idx);
            if ptr.is_null() {
                return MidiProgramInfo {
                    bank: 0,
                    program: 0,
                    name: String::new(),
                };
            }
            let r = &*ptr;
            MidiProgramInfo {
                bank: r.bank,
                program: r.program,
                name: cstr_to_string(r.name),
            }
        }
    }

    pub fn get_current_midi_program_index(&self, plugin_id: u32) -> i32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> i32> = self
                .lib
                .get(b"carla_get_current_midi_program_index\0")
                .unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn get_custom_data_value(&self, plugin_id: u32, dtype: &str, key: &str) -> String {
        let Ok(ct) = CString::new(dtype) else {
            return String::new();
        };
        let Ok(ck) = CString::new(key) else {
            return String::new();
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(
                    CarlaHostHandle,
                    c_uint,
                    *const c_char,
                    *const c_char,
                ) -> *const c_char,
            > = self.lib.get(b"carla_get_custom_data_value\0").unwrap();
            cstr_to_string(f(self.handle, plugin_id, ct.as_ptr(), ck.as_ptr()))
        }
    }

    pub fn set_custom_data(&self, plugin_id: u32, dtype: &str, key: &str, value: &str) {
        let Ok(ct) = CString::new(dtype) else {
            return;
        };
        let Ok(ck) = CString::new(key) else {
            return;
        };
        let Ok(cv) = CString::new(value) else {
            return;
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(
                    CarlaHostHandle,
                    c_uint,
                    *const c_char,
                    *const c_char,
                    *const c_char,
                ),
            > = self.lib.get(b"carla_set_custom_data\0").unwrap();
            f(
                self.handle,
                plugin_id,
                ct.as_ptr(),
                ck.as_ptr(),
                cv.as_ptr(),
            );
        }
    }

    pub fn get_input_peak_value(&self, plugin_id: u32, is_left: bool) -> f32 {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, bool) -> c_float,
            > = self.lib.get(b"carla_get_input_peak_value\0").unwrap();
            f(self.handle, plugin_id, is_left)
        }
    }

    pub fn get_output_peak_value(&self, plugin_id: u32, is_left: bool) -> f32 {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, bool) -> c_float,
            > = self.lib.get(b"carla_get_output_peak_value\0").unwrap();
            f(self.handle, plugin_id, is_left)
        }
    }

    pub fn get_plugin_latency(&self, plugin_id: u32) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint) -> u32> =
                self.lib.get(b"carla_get_plugin_latency\0").unwrap();
            f(self.handle, plugin_id)
        }
    }

    pub fn set_active(&self, plugin_id: u32, active: bool) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, bool)> =
                self.lib.get(b"carla_set_active\0").unwrap();
            f(self.handle, plugin_id, active);
        }
    }

    pub fn set_drywet(&self, plugin_id: u32, value: f32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, c_float)> =
                self.lib.get(b"carla_set_drywet\0").unwrap();
            f(self.handle, plugin_id, value);
        }
    }

    pub fn set_volume(&self, plugin_id: u32, value: f32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, c_float)> =
                self.lib.get(b"carla_set_volume\0").unwrap();
            f(self.handle, plugin_id, value);
        }
    }

    pub fn set_balance_left(&self, plugin_id: u32, value: f32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, c_float)> =
                self.lib.get(b"carla_set_balance_left\0").unwrap();
            f(self.handle, plugin_id, value);
        }
    }

    pub fn set_balance_right(&self, plugin_id: u32, value: f32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, c_float)> =
                self.lib.get(b"carla_set_balance_right\0").unwrap();
            f(self.handle, plugin_id, value);
        }
    }

    pub fn set_panning(&self, plugin_id: u32, value: f32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, c_float)> =
                self.lib.get(b"carla_set_panning\0").unwrap();
            f(self.handle, plugin_id, value);
        }
    }

    pub fn set_ctrl_channel(&self, plugin_id: u32, channel: i8) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, i8)> =
                self.lib.get(b"carla_set_ctrl_channel\0").unwrap();
            f(self.handle, plugin_id, channel);
        }
    }

    pub fn set_option(&self, plugin_id: u32, option: u32, yes_no: bool) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, c_uint, bool)> =
                self.lib.get(b"carla_set_option\0").unwrap();
            f(self.handle, plugin_id, option, yes_no);
        }
    }

    pub fn set_parameter_value(&self, plugin_id: u32, param_id: u32, value: f32) {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32, c_float),
            > = self.lib.get(b"carla_set_parameter_value\0").unwrap();
            f(self.handle, plugin_id, param_id, value);
        }
    }

    pub fn set_parameter_midi_channel(&self, plugin_id: u32, param_id: u32, channel: u8) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, u32, u8)> =
                self.lib
                    .get(b"carla_set_parameter_midi_channel\0")
                    .unwrap();
            f(self.handle, plugin_id, param_id, channel);
        }
    }

    pub fn set_parameter_mapped_control_index(&self, plugin_id: u32, param_id: u32, index: i16) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, u32, i16)> =
                self.lib
                    .get(b"carla_set_parameter_mapped_control_index\0")
                    .unwrap();
            f(self.handle, plugin_id, param_id, index);
        }
    }

    pub fn set_parameter_mapped_range(
        &self,
        plugin_id: u32,
        param_id: u32,
        min_val: f32,
        max_val: f32,
    ) {
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, c_uint, u32, c_float, c_float),
            > = self
                .lib
                .get(b"carla_set_parameter_mapped_range\0")
                .unwrap();
            f(self.handle, plugin_id, param_id, min_val, max_val);
        }
    }

    pub fn set_program(&self, plugin_id: u32, program_id: u32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, u32)> =
                self.lib.get(b"carla_set_program\0").unwrap();
            f(self.handle, plugin_id, program_id);
        }
    }

    pub fn set_midi_program(&self, plugin_id: u32, midi_program_id: u32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, u32)> =
                self.lib.get(b"carla_set_midi_program\0").unwrap();
            f(self.handle, plugin_id, midi_program_id);
        }
    }

    pub fn reset_parameters(&self, plugin_id: u32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint)> =
                self.lib.get(b"carla_reset_parameters\0").unwrap();
            f(self.handle, plugin_id);
        }
    }

    pub fn randomize_parameters(&self, plugin_id: u32) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint)> =
                self.lib.get(b"carla_randomize_parameters\0").unwrap();
            f(self.handle, plugin_id);
        }
    }

    pub fn send_midi_note(&self, plugin_id: u32, channel: u8, note: u8, velocity: u8) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, u8, u8, u8)> =
                self.lib.get(b"carla_send_midi_note\0").unwrap();
            f(self.handle, plugin_id, channel, note, velocity);
        }
    }

    pub fn show_custom_ui(&self, plugin_id: u32, show: bool) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_uint, bool)> =
                self.lib.get(b"carla_show_custom_ui\0").unwrap();
            f(self.handle, plugin_id, show);
        }
    }

    pub fn get_buffer_size(&self) -> u32 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> u32> =
                self.lib.get(b"carla_get_buffer_size\0").unwrap();
            f(self.handle)
        }
    }

    pub fn get_sample_rate(&self) -> f64 {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> c_double> =
                self.lib.get(b"carla_get_sample_rate\0").unwrap();
            f(self.handle)
        }
    }

    pub fn get_last_error(&self) -> String {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> *const c_char> =
                self.lib.get(b"carla_get_last_error\0").unwrap();
            cstr_to_string(f(self.handle))
        }
    }

    pub fn get_host_osc_url_tcp(&self) -> String {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> *const c_char> =
                self.lib.get(b"carla_get_host_osc_url_tcp\0").unwrap();
            cstr_to_string(f(self.handle))
        }
    }

    pub fn get_host_osc_url_udp(&self) -> String {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle) -> *const c_char> =
                self.lib.get(b"carla_get_host_osc_url_udp\0").unwrap();
            cstr_to_string(f(self.handle))
        }
    }

    pub fn nsm_init(&self, pid: u64, executable_name: &str) -> bool {
        let Ok(c) = CString::new(executable_name) else {
            return false;
        };
        unsafe {
            let f: libloading::Symbol<
                unsafe extern "C" fn(CarlaHostHandle, u64, *const c_char) -> bool,
            > = self.lib.get(b"carla_nsm_init\0").unwrap();
            f(self.handle, pid, c.as_ptr())
        }
    }

    pub fn nsm_ready(&self, opcode: c_int) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_int)> =
                self.lib.get(b"carla_nsm_ready\0").unwrap();
            f(self.handle, opcode);
        }
    }

    pub fn nsm_report_result(&self, opcode: c_int, succeeded: bool) {
        unsafe {
            let f: libloading::Symbol<unsafe extern "C" fn(CarlaHostHandle, c_int, bool)> =
                self.lib.get(b"carla_nsm_report_result\0").unwrap();
            f(self.handle, opcode, succeeded);
        }
    }
}

impl Drop for CarlaHost {
    fn drop(&mut self) {
        if self.is_engine_running() {
            let _ = self.set_engine_about_to_close();
            let _ = self.engine_close();
        }
    }
}
