// SPDX-License-Identifier: GPL-2.0-or-later

#![allow(dead_code, non_camel_case_types)]

use std::ffi::{c_char, c_double, c_float, c_int, c_uchar, c_uint, c_void};

pub const CARLA_VERSION_STRING: &str = "2.6.0-alpha1";

pub const MAX_DEFAULT_PLUGINS: u32 = 512;
pub const MAX_RACK_PLUGINS: u32 = 64;
pub const MAX_PATCHBAY_PLUGINS: u32 = 255;
pub const MAX_DEFAULT_PARAMETERS: u32 = 200;
pub const MAIN_CARLA_PLUGIN_ID: u32 = 0xFFFF;
pub const MAX_PLUGIN_ID_ALLOWED: i32 = 0x7FF;

// Engine driver device hints
pub const ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL: u32 = 0x1;
pub const ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER: u32 = 0x2;
pub const ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE: u32 = 0x4;
pub const ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE: u32 = 0x8;

// Plugin hints
pub const PLUGIN_IS_BRIDGE: u32 = 0x001;
pub const PLUGIN_IS_RTSAFE: u32 = 0x002;
pub const PLUGIN_IS_SYNTH: u32 = 0x004;
pub const PLUGIN_HAS_CUSTOM_UI: u32 = 0x008;
pub const PLUGIN_CAN_DRYWET: u32 = 0x010;
pub const PLUGIN_CAN_VOLUME: u32 = 0x020;
pub const PLUGIN_CAN_BALANCE: u32 = 0x040;
pub const PLUGIN_CAN_PANNING: u32 = 0x080;
pub const PLUGIN_NEEDS_FIXED_BUFFERS: u32 = 0x100;
pub const PLUGIN_NEEDS_UI_MAIN_THREAD: u32 = 0x200;
pub const PLUGIN_USES_MULTI_PROGS: u32 = 0x400;
pub const PLUGIN_HAS_INLINE_DISPLAY: u32 = 0x800;
pub const PLUGIN_HAS_CUSTOM_EMBED_UI: u32 = 0x1000;
pub const PLUGIN_HAS_CUSTOM_UI_USING_FILE_OPEN: u32 = 0x2000;
pub const PLUGIN_NEEDS_MAIN_THREAD_IDLE: u32 = 0x4000;
pub const PLUGIN_HAS_CUSTOM_RESIZABLE_UI: u32 = 0x8000;

// Plugin options
pub const PLUGIN_OPTION_FIXED_BUFFERS: u32 = 0x001;
pub const PLUGIN_OPTION_FORCE_STEREO: u32 = 0x002;
pub const PLUGIN_OPTION_MAP_PROGRAM_CHANGES: u32 = 0x004;
pub const PLUGIN_OPTION_USE_CHUNKS: u32 = 0x008;
pub const PLUGIN_OPTION_SEND_CONTROL_CHANGES: u32 = 0x010;
pub const PLUGIN_OPTION_SEND_CHANNEL_PRESSURE: u32 = 0x020;
pub const PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH: u32 = 0x040;
pub const PLUGIN_OPTION_SEND_PITCHBEND: u32 = 0x080;
pub const PLUGIN_OPTION_SEND_ALL_SOUND_OFF: u32 = 0x100;
pub const PLUGIN_OPTION_SEND_PROGRAM_CHANGES: u32 = 0x200;
pub const PLUGIN_OPTION_SKIP_SENDING_NOTES: u32 = 0x400;
pub const PLUGIN_OPTIONS_NULL: u32 = 0x10000;

// Parameter hints
pub const PARAMETER_IS_BOOLEAN: u32 = 0x001;
pub const PARAMETER_IS_INTEGER: u32 = 0x002;
pub const PARAMETER_IS_LOGARITHMIC: u32 = 0x004;
pub const PARAMETER_IS_ENABLED: u32 = 0x010;
pub const PARAMETER_IS_AUTOMATABLE: u32 = 0x020;
pub const PARAMETER_IS_READ_ONLY: u32 = 0x040;
pub const PARAMETER_USES_SAMPLERATE: u32 = 0x100;
pub const PARAMETER_USES_SCALEPOINTS: u32 = 0x200;
pub const PARAMETER_USES_CUSTOM_TEXT: u32 = 0x400;
pub const PARAMETER_CAN_BE_CV_CONTROLLED: u32 = 0x800;
pub const PARAMETER_IS_NOT_SAVED: u32 = 0x1000;

// Patchbay port hints
pub const PATCHBAY_PORT_IS_INPUT: u32 = 0x01;
pub const PATCHBAY_PORT_TYPE_AUDIO: u32 = 0x02;
pub const PATCHBAY_PORT_TYPE_CV: u32 = 0x04;
pub const PATCHBAY_PORT_TYPE_MIDI: u32 = 0x08;
pub const PATCHBAY_PORT_TYPE_OSC: u32 = 0x10;

// Custom data types
pub const CUSTOM_DATA_TYPE_BOOLEAN: &str = "http://kxstudio.sf.net/ns/carla/boolean";
pub const CUSTOM_DATA_TYPE_CHUNK: &str = "http://kxstudio.sf.net/ns/carla/chunk";
pub const CUSTOM_DATA_TYPE_PATH: &str = "http://kxstudio.sf.net/ns/carla/path";
pub const CUSTOM_DATA_TYPE_PROPERTY: &str = "http://kxstudio.sf.net/ns/carla/property";
pub const CUSTOM_DATA_TYPE_STRING: &str = "http://kxstudio.sf.net/ns/carla/string";

// BinaryType
pub const BINARY_NONE: c_int = 0;
pub const BINARY_POSIX32: c_int = 1;
pub const BINARY_POSIX64: c_int = 2;
pub const BINARY_WIN32: c_int = 3;
pub const BINARY_WIN64: c_int = 4;
pub const BINARY_OTHER: c_int = 5;

#[cfg(target_pointer_width = "64")]
pub const BINARY_NATIVE: c_int = BINARY_POSIX64;
#[cfg(target_pointer_width = "32")]
pub const BINARY_NATIVE: c_int = BINARY_POSIX32;

// FileType
pub const FILE_NONE: c_int = 0;
pub const FILE_AUDIO: c_int = 1;
pub const FILE_MIDI: c_int = 2;

// PluginType
pub const PLUGIN_NONE: c_int = 0;
pub const PLUGIN_INTERNAL: c_int = 1;
pub const PLUGIN_LADSPA: c_int = 2;
pub const PLUGIN_DSSI: c_int = 3;
pub const PLUGIN_LV2: c_int = 4;
pub const PLUGIN_VST2: c_int = 5;
pub const PLUGIN_VST3: c_int = 6;
pub const PLUGIN_AU: c_int = 7;
pub const PLUGIN_DLS: c_int = 8;
pub const PLUGIN_GIG: c_int = 9;
pub const PLUGIN_SF2: c_int = 10;
pub const PLUGIN_SFZ: c_int = 11;
pub const PLUGIN_JACK: c_int = 12;
pub const PLUGIN_JSFX: c_int = 13;
pub const PLUGIN_CLAP: c_int = 14;
pub const PLUGIN_TYPE_COUNT: c_int = 15;

// PluginCategory
pub const PLUGIN_CATEGORY_NONE: c_int = 0;
pub const PLUGIN_CATEGORY_SYNTH: c_int = 1;
pub const PLUGIN_CATEGORY_DELAY: c_int = 2;
pub const PLUGIN_CATEGORY_EQ: c_int = 3;
pub const PLUGIN_CATEGORY_FILTER: c_int = 4;
pub const PLUGIN_CATEGORY_DISTORTION: c_int = 5;
pub const PLUGIN_CATEGORY_DYNAMICS: c_int = 6;
pub const PLUGIN_CATEGORY_MODULATOR: c_int = 7;
pub const PLUGIN_CATEGORY_UTILITY: c_int = 8;
pub const PLUGIN_CATEGORY_OTHER: c_int = 9;

// ParameterType
pub const PARAMETER_UNKNOWN: c_int = 0;
pub const PARAMETER_INPUT: c_int = 1;
pub const PARAMETER_OUTPUT: c_int = 2;

// InternalParameterIndex
pub const PARAMETER_NULL: i32 = -1;
pub const PARAMETER_ACTIVE: i32 = -2;
pub const PARAMETER_DRYWET: i32 = -3;
pub const PARAMETER_VOLUME: i32 = -4;
pub const PARAMETER_BALANCE_LEFT: i32 = -5;
pub const PARAMETER_BALANCE_RIGHT: i32 = -6;
pub const PARAMETER_PANNING: i32 = -7;
pub const PARAMETER_CTRL_CHANNEL: i32 = -8;
pub const PARAMETER_MAX: i32 = -9;

// SpecialMappedControlIndex
pub const CONTROL_INDEX_NONE: i16 = -1;
pub const CONTROL_INDEX_CV: i16 = 130;
pub const CONTROL_INDEX_MIDI_PITCHBEND: i16 = 131;
pub const CONTROL_INDEX_MIDI_LEARN: i16 = 132;

// EngineCallbackOpcode
pub const ENGINE_CALLBACK_DEBUG: c_int = 0;
pub const ENGINE_CALLBACK_PLUGIN_ADDED: c_int = 1;
pub const ENGINE_CALLBACK_PLUGIN_REMOVED: c_int = 2;
pub const ENGINE_CALLBACK_PLUGIN_RENAMED: c_int = 3;
pub const ENGINE_CALLBACK_PLUGIN_UNAVAILABLE: c_int = 4;
pub const ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED: c_int = 5;
pub const ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED: c_int = 6;
pub const ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED: c_int = 7;
pub const ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED: c_int = 8;
pub const ENGINE_CALLBACK_OPTION_CHANGED: c_int = 9;
pub const ENGINE_CALLBACK_PROGRAM_CHANGED: c_int = 10;
pub const ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED: c_int = 11;
pub const ENGINE_CALLBACK_UI_STATE_CHANGED: c_int = 12;
pub const ENGINE_CALLBACK_NOTE_ON: c_int = 13;
pub const ENGINE_CALLBACK_NOTE_OFF: c_int = 14;
pub const ENGINE_CALLBACK_UPDATE: c_int = 15;
pub const ENGINE_CALLBACK_RELOAD_INFO: c_int = 16;
pub const ENGINE_CALLBACK_RELOAD_PARAMETERS: c_int = 17;
pub const ENGINE_CALLBACK_RELOAD_PROGRAMS: c_int = 18;
pub const ENGINE_CALLBACK_RELOAD_ALL: c_int = 19;
pub const ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED: c_int = 20;
pub const ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED: c_int = 21;
pub const ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED: c_int = 22;
pub const ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED: c_int = 23;
pub const ENGINE_CALLBACK_PATCHBAY_PORT_ADDED: c_int = 24;
pub const ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED: c_int = 25;
pub const ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED: c_int = 26;
pub const ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED: c_int = 27;
pub const ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED: c_int = 28;
pub const ENGINE_CALLBACK_ENGINE_STARTED: c_int = 29;
pub const ENGINE_CALLBACK_ENGINE_STOPPED: c_int = 30;
pub const ENGINE_CALLBACK_PROCESS_MODE_CHANGED: c_int = 31;
pub const ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED: c_int = 32;
pub const ENGINE_CALLBACK_BUFFER_SIZE_CHANGED: c_int = 33;
pub const ENGINE_CALLBACK_SAMPLE_RATE_CHANGED: c_int = 34;
pub const ENGINE_CALLBACK_CANCELABLE_ACTION: c_int = 35;
pub const ENGINE_CALLBACK_PROJECT_LOAD_FINISHED: c_int = 36;
pub const ENGINE_CALLBACK_NSM: c_int = 37;
pub const ENGINE_CALLBACK_IDLE: c_int = 38;
pub const ENGINE_CALLBACK_INFO: c_int = 39;
pub const ENGINE_CALLBACK_ERROR: c_int = 40;
pub const ENGINE_CALLBACK_QUIT: c_int = 41;
pub const ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW: c_int = 42;
pub const ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_ADDED: c_int = 43;
pub const ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_REMOVED: c_int = 44;
pub const ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_CHANGED: c_int = 45;
pub const ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED: c_int = 46;
pub const ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED: c_int = 47;
pub const ENGINE_CALLBACK_EMBED_UI_RESIZED: c_int = 48;

// NsmCallbackOpcode
pub const NSM_CALLBACK_INIT: c_int = 0;
pub const NSM_CALLBACK_ERROR: c_int = 1;
pub const NSM_CALLBACK_ANNOUNCE: c_int = 2;
pub const NSM_CALLBACK_OPEN: c_int = 3;
pub const NSM_CALLBACK_SAVE: c_int = 4;
pub const NSM_CALLBACK_SESSION_IS_LOADED: c_int = 5;
pub const NSM_CALLBACK_SHOW_OPTIONAL_GUI: c_int = 6;
pub const NSM_CALLBACK_HIDE_OPTIONAL_GUI: c_int = 7;
pub const NSM_CALLBACK_SET_CLIENT_NAME_ID: c_int = 8;

// EngineOption
pub const ENGINE_OPTION_DEBUG: c_int = 0;
pub const ENGINE_OPTION_PROCESS_MODE: c_int = 1;
pub const ENGINE_OPTION_TRANSPORT_MODE: c_int = 2;
pub const ENGINE_OPTION_FORCE_STEREO: c_int = 3;
pub const ENGINE_OPTION_PREFER_PLUGIN_BRIDGES: c_int = 4;
pub const ENGINE_OPTION_PREFER_UI_BRIDGES: c_int = 5;
pub const ENGINE_OPTION_UIS_ALWAYS_ON_TOP: c_int = 6;
pub const ENGINE_OPTION_MAX_PARAMETERS: c_int = 7;
pub const ENGINE_OPTION_RESET_XRUNS: c_int = 8;
pub const ENGINE_OPTION_UI_BRIDGES_TIMEOUT: c_int = 9;
pub const ENGINE_OPTION_AUDIO_BUFFER_SIZE: c_int = 10;
pub const ENGINE_OPTION_AUDIO_SAMPLE_RATE: c_int = 11;
pub const ENGINE_OPTION_AUDIO_TRIPLE_BUFFER: c_int = 12;
pub const ENGINE_OPTION_AUDIO_DRIVER: c_int = 13;
pub const ENGINE_OPTION_AUDIO_DEVICE: c_int = 14;
pub const ENGINE_OPTION_OSC_ENABLED: c_int = 15;
pub const ENGINE_OPTION_OSC_PORT_TCP: c_int = 16;
pub const ENGINE_OPTION_OSC_PORT_UDP: c_int = 17;
pub const ENGINE_OPTION_FILE_PATH: c_int = 18;
pub const ENGINE_OPTION_PLUGIN_PATH: c_int = 19;
pub const ENGINE_OPTION_PATH_BINARIES: c_int = 20;
pub const ENGINE_OPTION_PATH_RESOURCES: c_int = 21;
pub const ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR: c_int = 22;
pub const ENGINE_OPTION_FRONTEND_BACKGROUND_COLOR: c_int = 23;
pub const ENGINE_OPTION_FRONTEND_FOREGROUND_COLOR: c_int = 24;
pub const ENGINE_OPTION_FRONTEND_UI_SCALE: c_int = 25;
pub const ENGINE_OPTION_FRONTEND_WIN_ID: c_int = 26;
pub const ENGINE_OPTION_WINE_EXECUTABLE: c_int = 27;
pub const ENGINE_OPTION_WINE_AUTO_PREFIX: c_int = 28;
pub const ENGINE_OPTION_WINE_FALLBACK_PREFIX: c_int = 29;
pub const ENGINE_OPTION_WINE_RT_PRIO_ENABLED: c_int = 30;
pub const ENGINE_OPTION_WINE_BASE_RT_PRIO: c_int = 31;
pub const ENGINE_OPTION_WINE_SERVER_RT_PRIO: c_int = 32;
pub const ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT: c_int = 33;
pub const ENGINE_OPTION_CLIENT_NAME_PREFIX: c_int = 34;
pub const ENGINE_OPTION_PLUGINS_ARE_STANDALONE: c_int = 35;

// EngineProcessMode
pub const ENGINE_PROCESS_MODE_SINGLE_CLIENT: c_int = 0;
pub const ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS: c_int = 1;
pub const ENGINE_PROCESS_MODE_CONTINUOUS_RACK: c_int = 2;
pub const ENGINE_PROCESS_MODE_PATCHBAY: c_int = 3;
pub const ENGINE_PROCESS_MODE_BRIDGE: c_int = 4;

// EngineTransportMode
pub const ENGINE_TRANSPORT_MODE_DISABLED: c_int = 0;
pub const ENGINE_TRANSPORT_MODE_INTERNAL: c_int = 1;
pub const ENGINE_TRANSPORT_MODE_JACK: c_int = 2;
pub const ENGINE_TRANSPORT_MODE_PLUGIN: c_int = 3;
pub const ENGINE_TRANSPORT_MODE_BRIDGE: c_int = 4;

// FileCallbackOpcode
pub const FILE_CALLBACK_DEBUG: c_int = 0;
pub const FILE_CALLBACK_OPEN: c_int = 1;
pub const FILE_CALLBACK_SAVE: c_int = 2;
pub const FILE_CALLBACK_CONFIRM_PROJECT_JACK_APPS: c_int = 3;

// PatchbayIcon
pub const PATCHBAY_ICON_APPLICATION: c_int = 0;
pub const PATCHBAY_ICON_PLUGIN: c_int = 1;
pub const PATCHBAY_ICON_HARDWARE: c_int = 2;
pub const PATCHBAY_ICON_CARLA: c_int = 3;
pub const PATCHBAY_ICON_DISTRHO: c_int = 4;
pub const PATCHBAY_ICON_FILE: c_int = 5;

pub type CarlaHostHandle = *mut c_void;
pub type CarlaPluginDiscoveryHandle = *mut c_void;

pub type EngineCallbackFunc = unsafe extern "C" fn(
    ptr: *mut c_void,
    action: c_int,
    plugin_id: c_uint,
    value1: c_int,
    value2: c_int,
    value3: c_int,
    valuef: c_float,
    value_str: *const c_char,
);

pub type FileCallbackFunc = unsafe extern "C" fn(
    ptr: *mut c_void,
    action: c_int,
    is_dir: bool,
    title: *const c_char,
    filter: *const c_char,
) -> *const c_char;

pub type CarlaPluginDiscoveryCallback = unsafe extern "C" fn(
    ptr: *mut c_void,
    info: *const RawCarlaPluginDiscoveryInfo,
    sha1: *const c_char,
);

pub type CarlaPluginCheckCacheCallback = unsafe extern "C" fn(
    ptr: *mut c_void,
    filename: *const c_char,
    sha1: *const c_char,
) -> bool;

#[repr(C)]
pub struct RawCarlaPluginInfo {
    pub ptype: c_int,
    pub category: c_int,
    pub hints: c_uint,
    pub options_available: c_uint,
    pub options_enabled: c_uint,
    pub filename: *const c_char,
    pub name: *const c_char,
    pub label: *const c_char,
    pub maker: *const c_char,
    pub copyright: *const c_char,
    pub icon_name: *const c_char,
    pub unique_id: i64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct CarlaPortCountInfo {
    pub ins: u32,
    pub outs: u32,
}

#[repr(C)]
pub struct RawCarlaParameterInfo {
    pub name: *const c_char,
    pub symbol: *const c_char,
    pub unit: *const c_char,
    pub comment: *const c_char,
    pub group_name: *const c_char,
    pub scale_point_count: u32,
}

#[repr(C)]
pub struct RawCarlaScalePointInfo {
    pub value: c_float,
    pub label: *const c_char,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct ParameterData {
    pub ptype: c_int,
    pub hints: c_uint,
    pub index: i32,
    pub rindex: i32,
    pub midi_channel: u8,
    pub mapped_control_index: i16,
    pub mapped_minimum: c_float,
    pub mapped_maximum: c_float,
    pub mapped_flags: c_uint,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct ParameterRanges {
    pub def: c_float,
    pub min: c_float,
    pub max: c_float,
    pub step: c_float,
    pub step_small: c_float,
    pub step_large: c_float,
}

#[repr(C)]
pub struct RawMidiProgramData {
    pub bank: u32,
    pub program: u32,
    pub name: *const c_char,
}

#[repr(C)]
pub struct RawCustomData {
    pub dtype: *const c_char,
    pub key: *const c_char,
    pub value: *const c_char,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct CarlaTransportInfo {
    pub playing: bool,
    pub frame: u64,
    pub bar: i32,
    pub beat: i32,
    pub tick: i32,
    pub bpm: c_double,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct CarlaRuntimeEngineInfo {
    pub load: c_float,
    pub xruns: u32,
}

#[repr(C)]
pub struct RawEngineDriverDeviceInfo {
    pub hints: c_uint,
    pub buffer_sizes: *const u32,
    pub sample_rates: *const c_double,
}

#[repr(C)]
pub struct RawCarlaRuntimeEngineDriverDeviceInfo {
    pub name: *const c_char,
    pub hints: c_uint,
    pub buffer_size: c_uint,
    pub buffer_sizes: *const u32,
    pub sample_rate: c_double,
    pub sample_rates: *const c_double,
}

#[repr(C)]
pub struct RawCarlaInlineDisplayImageSurface {
    pub data: *mut c_uchar,
    pub width: c_int,
    pub height: c_int,
    pub stride: c_int,
}

#[repr(C)]
pub struct RawCarlaPluginDiscoveryMetadata {
    pub name: *const c_char,
    pub maker: *const c_char,
    pub category: c_int,
    pub hints: c_uint,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct CarlaPluginDiscoveryIO {
    pub audio_ins: u32,
    pub audio_outs: u32,
    pub cv_ins: u32,
    pub cv_outs: u32,
    pub midi_ins: u32,
    pub midi_outs: u32,
    pub parameter_ins: u32,
    pub parameter_outs: u32,
}

#[repr(C)]
pub struct RawCarlaPluginDiscoveryInfo {
    pub btype: c_int,
    pub ptype: c_int,
    pub filename: *const c_char,
    pub label: *const c_char,
    pub unique_id: u64,
    pub metadata: RawCarlaPluginDiscoveryMetadata,
    pub io: CarlaPluginDiscoveryIO,
}

#[repr(C)]
pub struct RawCarlaCachedPluginInfo {
    pub valid: bool,
    pub category: c_int,
    pub hints: c_uint,
    pub audio_ins: u32,
    pub audio_outs: u32,
    pub cv_ins: u32,
    pub cv_outs: u32,
    pub midi_ins: u32,
    pub midi_outs: u32,
    pub parameter_ins: u32,
    pub parameter_outs: u32,
    pub name: *const c_char,
    pub label: *const c_char,
    pub maker: *const c_char,
    pub copyright: *const c_char,
}

pub fn plugin_type_label(ptype: c_int) -> &'static str {
    match ptype {
        PLUGIN_INTERNAL => "Internal",
        PLUGIN_LADSPA => "LADSPA",
        PLUGIN_DSSI => "DSSI",
        PLUGIN_LV2 => "LV2",
        PLUGIN_VST2 => "VST2",
        PLUGIN_VST3 => "VST3",
        PLUGIN_AU => "AU",
        PLUGIN_DLS => "DLS",
        PLUGIN_GIG => "GIG",
        PLUGIN_SF2 => "SF2",
        PLUGIN_SFZ => "SFZ",
        PLUGIN_JACK => "JACK",
        PLUGIN_JSFX => "JSFX",
        PLUGIN_CLAP => "CLAP",
        _ => "Unknown",
    }
}

pub fn process_mode_label(mode: c_int) -> &'static str {
    match mode {
        ENGINE_PROCESS_MODE_SINGLE_CLIENT => "Single Client",
        ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS => "Multiple Clients",
        ENGINE_PROCESS_MODE_CONTINUOUS_RACK => "Continuous Rack",
        ENGINE_PROCESS_MODE_PATCHBAY => "Patchbay",
        ENGINE_PROCESS_MODE_BRIDGE => "Bridge",
        _ => "Unknown",
    }
}

pub fn binary_type_label(btype: c_int) -> &'static str {
    match btype {
        BINARY_POSIX32 => "POSIX 32-bit",
        BINARY_POSIX64 => "POSIX 64-bit",
        BINARY_WIN32 => "Windows 32-bit",
        BINARY_WIN64 => "Windows 64-bit",
        BINARY_OTHER => "Other",
        _ => "None",
    }
}
