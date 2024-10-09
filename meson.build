project (
  'carla', ['c', 'cpp',],
  license: 'GPL-2.0-or-later',
  default_options: [
    'cpp_std=c++14',
    'buildtype=debugoptimized',
    'warning_level=1',
    ],
  # see also source/frontend/carla_shared.py and source/includes/CarlaDefines.h
  version: '2.5.1',
  )

pkgconfig_mod = import ('pkgconfig')

is_darwin = false
is_linux = false
is_freebsd = false
is_windows = false
bin_suffix = ''
lib_suffix = '.so'

if host_machine.system() == 'darwin'
  is_darwin = true
  lib_suffix = '.dylib'
elif host_machine.system() == 'linux'
  is_linux = true
elif host_machine.system() == 'freebsd'
  is_freebsd = true
elif host_machine.system() == 'windows'
  is_windows = true
  bin_suffix = '.exe'
  lib_suffix = '.dll'
else
  error ('unknown host system ' + host_machine.system ())
endif

if is_darwin
  add_languages ('objc')
endif

cpp = meson.get_compiler('cpp')
is_gcc = cpp.get_id () == 'gcc'

# === Dependencies ===

add_project_arguments (
  '-DREAL_BUILD',
  '-DCARLA_LIB_EXT="' + lib_suffix + '"',
  '-DHAVE_FLUIDSYNTH=1',
  '-DHAVE_LIBLO=1',
  '-DUSING_JUCE',
  '-DJUCE_APP_CONFIG_HEADER="AppConfig.h"',
  '-mtune=generic',
  '-msse',
  '-msse2',
  '-mfpmath=sse',
  language: [ 'c', 'cpp', ],
  )

if is_linux or is_freebsd
  add_project_arguments (
    '-DHAVE_X11=1',
    '-ffast-math',
    language: ['c', 'cpp',],
    )
endif

if is_windows
  add_project_arguments (
    '-D_WIN32_WINNT=0x0600',
    language: ['c', 'cpp',],
    )
endif

if get_option ('buildtype') == 'debug'
  add_project_arguments (
    '-DDEBUG',
    language: ['c', 'cpp',],
    )
endif

# === Libraries ===

carla_engine_plugin_srcs = []
carla_plugin_srcs = []
native_plugins_srcs = []

source_backend_inc = include_directories ('source/backend')
source_includes_inc = include_directories ('source/includes')
source_includes_vst3sdk_inc = include_directories ('source/includes/vst3sdk')
source_modules_inc = include_directories ('source/modules')
source_utils_inc = include_directories ('source/utils')
source_inc = include_directories ('source')

lilv_config_inc = include_directories ('source/modules/lilv/config')

serd_lib = static_library ('serd',
  'source/modules/lilv/serd.c',
  include_directories: [
    'source/modules/lilv/serd-0.24.0',
    lilv_config_inc,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )
serd_dep = declare_dependency (
  include_directories: 'source/modules/lilv/serd-0.24.0',
  link_with: serd_lib)

sord_lib = static_library ('sord',
  'source/modules/lilv/sord.c',
  include_directories: [
    'source/modules/lilv/sord-0.16.0',
    'source/modules/lilv/sord-0.16.0/src',
    lilv_config_inc,
    ],
  dependencies: serd_dep,
  gnu_symbol_visibility: 'inlineshidden',
  )
sord_dep = declare_dependency (
  include_directories: 'source/modules/lilv/sord-0.16.0',
  dependencies: serd_dep,
  link_with: sord_lib)

sratom_lib = static_library ('sratom',
  'source/modules/lilv/sratom.c',
  include_directories: [
    'source/modules/lilv/sratom-0.6.0',
    lilv_config_inc,
    source_includes_inc,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  dependencies: sord_dep,
  )
sratom_dep = declare_dependency (
  include_directories: 'source/modules/lilv/sratom-0.6.0',
  dependencies: sord_dep,
  link_with: sratom_lib)

lilv_lib = static_library ('lilv',
  'source/modules/lilv/lilv.c',
  include_directories: [
    'source/modules/lilv/lilv-0.24.0',
    'source/modules/lilv/lilv-0.24.0/src',
    lilv_config_inc,
    source_includes_inc,
    ],
  dependencies: sratom_dep,
  gnu_symbol_visibility: 'inlineshidden',
  )

rtmempool_lib = static_library ('rtmempool',
  'source/modules/rtmempool/rtmempool.c',
  include_directories: [
    source_includes_inc,
    ],
  )

audio_decoder_srcs = [
  'source/modules/audio_decoder/ad_dr_mp3.c',
  'source/modules/audio_decoder/ad_ffmpeg.c',
  'source/modules/audio_decoder/ad_minimp3.c',
  'source/modules/audio_decoder/ad_plugin.c',
  'source/modules/audio_decoder/ad_soundfile.c',
  ]
audio_decoder_lib = static_library ('audio_decoder',
  audio_decoder_srcs,
  include_directories: [
    source_includes_inc,
    ],
  )

sfzero_lib = static_library ('sfzero',
  'source/modules/sfzero/SFZero.cpp',
  include_directories: [
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    ],
  link_with: audio_decoder_lib,
  dependencies: [
    dependency ('threads'),
    ],
  )

water_lib = static_library ('water',
  'source/modules/water/water.cpp',
  include_directories: [
    source_includes_inc,
    source_utils_inc,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

ysfx_flags = [
  '-fsigned-char',
  '-DEELSCRIPT_NO_NET',
  '-DEELSCRIPT_NO_LICE',
  '-DNSEEL_ATOF=ysfx_wdl_atof',
  '-DWDL_FFT_REALSIZE=8',
  '-DWDL_LINEPARSE_ATOF=ysfx_wdl_atof',
  '-DYSFX_API=',
  '-DYSFX_NO_STANDARD_MUTEX',
  '-Wno-sign-compare',
  '-Wno-unused-function',
  '-Wno-unused-parameter',
  ]

if is_windows
  ysfx_flags += [
    '-DNOMINMAX',
    ]
elif is_darwin
else
  ysfx_flags += [
    '-DSWELL_LICE_GDI',
    '-DSWELL_FONTCONFIG',
    '-DSWELL_FREETYPE',
    ]
endif

ysfx_srcs = [
  'source/modules/ysfx/sources/lice_stb/lice_stb_bmp.cpp',
  'source/modules/ysfx/sources/lice_stb/lice_stb_gif.cpp',
  'source/modules/ysfx/sources/lice_stb/lice_stb_jpg.cpp',
  'source/modules/ysfx/sources/lice_stb/lice_stb_loaders.cpp',
  'source/modules/ysfx/sources/lice_stb/lice_stb_png.cpp',
  'source/modules/ysfx/sources/lice_stb/lice_stb_write.cpp',
  'source/modules/ysfx/sources/ysfx.cpp',
  'source/modules/ysfx/sources/ysfx_api_eel.cpp',
  'source/modules/ysfx/sources/ysfx_api_file.cpp',
  'source/modules/ysfx/sources/ysfx_api_gfx.cpp',
  'source/modules/ysfx/sources/ysfx_api_reaper.cpp',
  'source/modules/ysfx/sources/ysfx_audio_flac.cpp',
  'source/modules/ysfx/sources/ysfx_audio_wav.cpp',
  'source/modules/ysfx/sources/ysfx_config.cpp',
  'source/modules/ysfx/sources/ysfx_eel_utils.cpp',
  'source/modules/ysfx/sources/ysfx_midi.cpp',
  'source/modules/ysfx/sources/ysfx_parse.cpp',
  'source/modules/ysfx/sources/ysfx_parse_menu.cpp',
  'source/modules/ysfx/sources/ysfx_preset.cpp',
  'source/modules/ysfx/sources/ysfx_reader.cpp',
  'source/modules/ysfx/sources/ysfx_utils.cpp',
  'source/modules/ysfx/sources/ysfx_utils_fts.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/fft.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-caltab.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-cfunc.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-compiler.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-eval.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-lextab.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-ram.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-yylex.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/y.tab.c',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_arc.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_colorspace.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_image.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_line.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_palette.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_texgen.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_text.cpp',
  'source/modules/ysfx/thirdparty/WDL/source/WDL/lice/lice_textnew.cpp',
  ]

if is_gcc
  ysfx_srcs += [
    'source/modules/ysfx/sources/eel2-gas/sources/asm-nseel-x64-sse.S',
    ]
endif

if is_windows
  ysfx_srcs += [
    'source/modules/ysfx/thirdparty/WDL/source/WDL/win32_utf8.c',
    ]
else # windows
  ysfx_srcs += [
    'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-ini.cpp',
    'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell.cpp',
    ]
  if is_darwin
    ysfx_srcs += [
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-appstub.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-dlg.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-gdi.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-kb.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-menu.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-misc.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-miscdlg.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-modstub.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-wnd.mm',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swellappmain.mm',
      ]
  else # macos
    ysfx_srcs += [
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-appstub-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-dlg-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-gdi-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-gdi-lice.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-generic-gdk.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-generic-headless.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-kb-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-menu-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-misc-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-miscdlg-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-modstub-generic.cpp',
      'source/modules/ysfx/thirdparty/WDL/source/WDL/swell/swell-wnd-generic.cpp',
      ]
  endif # macos
endif # windows

ysfx_lib = static_library ('ysfx',
  ysfx_srcs,
  cpp_args: ysfx_flags,
  include_directories: [
    'source/modules/ysfx/include',
    'source/modules/ysfx/sources',
    'source/modules/ysfx/thirdparty/WDL/source',
    'source/modules/ysfx/thirdparty/dr_libs',
    'source/modules/ysfx/thirdparty/stb',
    ],
  dependencies: [
    dependency('threads'),
    dependency('fontconfig'),
    dependency('freetype2'),
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

zita_resampler_srcs = [
  'source/modules/zita-resampler/cresampler.cc',
  'source/modules/zita-resampler/resampler-table.cc',
  'source/modules/zita-resampler/resampler.cc',
  'source/modules/zita-resampler/vresampler.cc',
  ]
zita_resampler_lib = static_library ('zita_resampler',
  zita_resampler_srcs,
  include_directories: [
    source_includes_inc,
    ],
  )

juce_core_lib = static_library ('juce_core',
  'source/modules/juce_core/juce_core.cpp',
  include_directories: [
    source_modules_inc,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

juce_data_structures_lib = static_library ('juce_data_structures',
  'source/modules/juce_data_structures/juce_data_structures.cpp',
  include_directories: [
    source_modules_inc,
    source_includes_vst3sdk_inc,
    ],
  cpp_args: [
    '-DRELEASE',
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

juce_gui_extra_lib = static_library ('juce_gui_extra',
  'source/modules/juce_gui_extra/juce_gui_extra.cpp',
  include_directories: [
    source_modules_inc,
    source_includes_vst3sdk_inc,
    ],
  cpp_args: [
    '-DRELEASE',
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

# needed for older mingw, crashes on optimized float operations
juce_audio_basics_args = []
if is_windows
  juce_audio_basics_args += [
    '-mpreferred-stack-boundary=2',
    ]
endif

juce_audio_basics_lib = static_library ('juce_audio_basics',
  'source/modules/juce_audio_basics/juce_audio_basics.cpp',
  include_directories: [
    source_modules_inc,
    ],
  cpp_args: juce_audio_basics_args,
  gnu_symbol_visibility: 'inlineshidden',
  )

juce_audio_processors_lib = static_library ('juce_audio_processors',
  'source/modules/juce_audio_processors/juce_audio_processors.cpp',
  include_directories: [
    source_modules_inc,
    source_includes_vst3sdk_inc,
    ],
  cpp_args: [
    '-DRELEASE',
    ],
  link_with: [
    juce_data_structures_lib,
    juce_gui_extra_lib,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

juce_events_lib = static_library ('juce_events',
  'source/modules/juce_events/juce_events.cpp',
  include_directories: [
    source_modules_inc,
    ],
  link_with: [
    juce_core_lib,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

juce_graphics_deps = []
if is_linux
  juce_graphics_deps += dependency ('freetype2')
endif

juce_graphics_lib = static_library ('juce_graphics',
  'source/modules/juce_graphics/juce_graphics.cpp',
  include_directories: [
    source_modules_inc,
    source_includes_vst3sdk_inc,
    ],
  cpp_args: [
    '-DRELEASE',
    ],
  dependencies: [
    juce_graphics_deps,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

juce_gui_basics_lib = static_library ('juce_gui_basics',
  'source/modules/juce_gui_basics/juce_gui_basics.cpp',
  include_directories: [
    source_modules_inc,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

carla_juce_lib = static_library ('carla_juce',
  'source/modules/carla_juce/carla_juce.cpp',
  include_directories: [
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    ],
  link_with: [
    juce_events_lib,
    ],
  gnu_symbol_visibility: 'inlineshidden',
  )

source_utils_srcs = [
  'source/utils/CarlaPipeUtils.cpp',
  ]
source_utils_lib = static_library ('source_utils',
  source_utils_srcs,
  include_directories: [
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    ],
  link_with: [
    water_lib,
    ],
  )

jackbridge_srcs = [
  'source/jackbridge/JackBridge1.cpp',
  'source/jackbridge/JackBridge2.cpp',
  'source/jackbridge/JackBridge3.cpp',
  'source/jackbridge/JackBridgeExport.cpp',
  ]
jackbridge_lib = static_library ('jackbridge',
  jackbridge_srcs,
  include_directories: [
    source_includes_inc,
    source_utils_inc,
    ],
  )

backend_utils_srcs = [
  'source/backend/utils/CachedPlugins.cpp',
  'source/backend/utils/Information.cpp',
  'source/backend/utils/JUCE.cpp',
  'source/backend/utils/PipeClient.cpp',
  'source/backend/utils/System.cpp',
  'source/backend/utils/Windows.cpp',
  ]
backend_utils_lib = static_library ('backend_utils',
  backend_utils_srcs,
  include_directories: [
    source_backend_inc,
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    source_inc,
    ],
  )

carla_engine_srcs = [
  'source/backend/engine/CarlaEngine.cpp',
  'source/backend/engine/CarlaEngineClient.cpp',
  'source/backend/engine/CarlaEngineData.cpp',
  'source/backend/engine/CarlaEngineGraph.cpp',
  'source/backend/engine/CarlaEngineInternal.cpp',
  'source/backend/engine/CarlaEngineNative.cpp',
  'source/backend/engine/CarlaEnginePorts.cpp',
  'source/backend/engine/CarlaEngineRunner.cpp',
  'source/backend/engine/CarlaEngineDummy.cpp',
  ]

backend_engine_lib = static_library ('carla_engine',
  carla_engine_srcs,
  include_directories: [
    source_backend_inc,
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    source_inc,
    ],
  link_with: [
    water_lib,
    jackbridge_lib,
    ],
  )

backend_plugin_srcs = files([
  'source/backend/plugin/CarlaPlugin.cpp',
  'source/backend/plugin/CarlaPluginAU.cpp',
  'source/backend/plugin/CarlaPluginBridge.cpp',
  'source/backend/plugin/CarlaPluginCLAP.cpp',
  'source/backend/plugin/CarlaPluginFluidSynth.cpp',
  'source/backend/plugin/CarlaPluginInternal.cpp',
  'source/backend/plugin/CarlaPluginJack.cpp',
  'source/backend/plugin/CarlaPluginJSFX.cpp',
  'source/backend/plugin/CarlaPluginJuce.cpp',
  'source/backend/plugin/CarlaPluginLADSPADSSI.cpp',
  'source/backend/plugin/CarlaPluginLV2.cpp',
  'source/backend/plugin/CarlaPluginNative.cpp',
  'source/backend/plugin/CarlaPluginSFZero.cpp',
  'source/backend/plugin/CarlaPluginVST2.cpp',
  'source/backend/plugin/CarlaPluginVST3.cpp',
  ])

backend_plugin_lib = static_library ('backend_plugin',
  backend_plugin_srcs,
  include_directories: [
    source_backend_inc,
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    source_inc,
    ],
  link_with: [
    water_lib,
    jackbridge_lib,
    rtmempool_lib,
    ysfx_lib,
    ],
  dependencies: [
    dependency ('threads'),
    ],
  )

carla_discovery_exe = executable ('carla-discovery-native',
  'source/discovery/carla-discovery.cpp',
  include_directories: [
    source_backend_inc,
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    source_inc,
    ],
  link_with: [
    carla_juce_lib,
    backend_plugin_lib,
    water_lib,
    lilv_lib,
    juce_audio_basics_lib,
    juce_audio_processors_lib,
    juce_graphics_lib,
    juce_gui_basics_lib,
    ],
  dependencies: [
    dependency ('fluidsynth'),
    ],
  install: true,
  install_dir: get_option('libdir') / 'carla',
  )

source_native_plugins_srcs = [
  'source/native-plugins/audio-file.cpp',
  'source/native-plugins/audio-gain.c',
  'source/native-plugins/bypass.c',
  'source/native-plugins/cv-to-audio.c',
  'source/native-plugins/lfo.c',
  'source/native-plugins/midi-to-cv.c',
  'source/native-plugins/midi-channel-ab.c',
  'source/native-plugins/midi-channelize.c',
  'source/native-plugins/midi-channel-filter.c',
  'source/native-plugins/midi-file.cpp',
  'source/native-plugins/midi-gain.c',
  'source/native-plugins/midi-join.c',
  'source/native-plugins/midi-split.c',
  'source/native-plugins/midi-through.c',
  'source/native-plugins/midi-transpose.c',
  ]

source_native_plugins_lib = static_library ('source_native_plugins',
  source_native_plugins_srcs,
  include_directories: [
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    ],
  link_with: [
    zita_resampler_lib,
    ],
  )

carla_plugin_deps = [
  dependency ('fluidsynth'),
  dependency ('liblo'),
  dependency ('threads'),
  ]
if is_linux or is_freebsd
  carla_plugin_deps += [
    dependency ('x11'),
    dependency ('xcursor'),
    dependency ('xext'),
    ]
endif

carla_host_plugin_srcs = [
  'source/plugin/carla-host-plugin.cpp',
  'source/native-plugins/_all.c',
  'source/native-plugins/_data.cpp',
  'source/backend/engine/CarlaEngineOsc.cpp',
  'source/backend/engine/CarlaEngineOscSend.cpp',
  'source/backend/engine/CarlaEngineOscHandlers.cpp',
  'source/backend/engine/CarlaEngineNative.cpp',
  'source/backend/plugin/CarlaPlugin.cpp',
  'source/utils/CarlaDssiUtils.cpp',
  'source/utils/CarlaPluginUI.cpp',
  'source/utils/CarlaStateUtils.cpp',
  'source/utils/CarlaProcessUtils.cpp',
  ]

carla_host_plugin_lib = both_libraries ('carla_host-plugin',
  carla_host_plugin_srcs,
  include_directories: [
    source_backend_inc,
    source_includes_inc,
    source_modules_inc,
    source_utils_inc,
    ],
  cpp_args: [
    '-DCARLA_PLUGIN_UI_CLASS_PREFIX=Plugin',
    ],
  dependencies: carla_plugin_deps,
  link_with: [
    lilv_lib,
    backend_plugin_lib,
    backend_engine_lib,
    source_utils_lib,
    source_native_plugins_lib,
    ysfx_lib,
    water_lib,
    jackbridge_lib,
    sfzero_lib,
    carla_juce_lib,
    juce_audio_basics_lib,
    juce_audio_processors_lib,
    juce_graphics_lib,
    juce_gui_basics_lib,
    ],
  gnu_symbol_visibility: 'hidden',
  install: true,
  install_dir: get_option('libdir') / 'carla',
  )

carla_host_plugin_dep = declare_dependency (
  link_with: carla_host_plugin_lib)

pkgconfig_mod.generate (
  carla_host_plugin_lib,
  subdirs: 'carla',
  extra_cflags: [
    '-DREAL_BUILD',
    '-I${includedir}/carla/includes',
    ],
  filebase: 'carla-host-plugin',
  name: 'carla-host-plugin',
  install_dir: get_option('libdir') / 'pkgconfig',
  description: 'Carla Host as Native Plugin')

install_headers (
  'source/includes/CarlaDefines.h',
  'source/includes/CarlaMIDI.h',
  'source/includes/CarlaNative.h',
  'source/includes/CarlaNativePlugin.h',
  subdir: 'carla/includes')

install_headers (
  'source/backend/CarlaBackend.h',
  'source/backend/CarlaHost.h',
  'source/backend/CarlaUtils.h',
  subdir: 'carla')
