cmake_minimum_required(VERSION 3.15)
project(carla)

set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

#######################################################################################################################
# required dependencies

find_package(PkgConfig)
find_package(Threads REQUIRED)

if(NOT (APPLE OR HAIKU OR WIN32))
  set(CARLA_LIBDL dl)
  set(CARLA_LIBM m)
  set(CARLA_LIBRT rt)
endif()

# set(carla_pthread_libs ${CMAKE_THREAD_LIBS_INIT})

#######################################################################################################################
# optional dependencies

if(PKGCONFIG_FOUND)
  pkg_check_modules(FLUIDSYNTH IMPORTED_TARGET fluidsynth)
  pkg_check_modules(SNDFILE IMPORTED_TARGET sndfile)
else()
  set(FLUIDSYNTH_FOUND FALSE)
  set(SNDFILE_FOUND FALSE)
endif()

if(PKGCONFIG_FOUND AND NOT WIN32)
  pkg_check_modules(LIBMAGIC IMPORTED_TARGET libmagic)
else()
  set(LIBMAGIC_FOUND FALSE)
endif()

if(PKGCONFIG_FOUND AND NOT (APPLE OR WIN32))
  pkg_check_modules(X11 IMPORTED_TARGET x11)
else()
  set(X11_FOUND FALSE)
endif()

add_library(carla-none INTERFACE)

if(NOT FLUIDSYNTH_FOUND)
  add_library(PkgConfig::FLUIDSYNTH ALIAS carla-none)
endif()

if(NOT LIBMAGIC_FOUND)
  add_library(PkgConfig::LIBMAGIC ALIAS carla-none)
endif()

if(NOT SNDFILE_FOUND)
  add_library(PkgConfig::SNDFILE ALIAS carla-none)
endif()

if(NOT X11_FOUND)
  add_library(PkgConfig::X11 ALIAS carla-none)
endif()

#######################################################################################################################
# utilities

function(set_common_target_properties TARGET)
  target_compile_definitions(${TARGET}
    PRIVATE
      BUILDING_CARLA
      HAVE_YSFX
      $<$<BOOL:${FLUIDSYNTH_FOUND}>:HAVE_FLUIDSYNTH>
      $<$<BOOL:${LIBMAGIC_FOUND}>:HAVE_LIBMAGIC>
      $<$<BOOL:${SNDFILE_FOUND}>:HAVE_SNDFILE>
      $<$<BOOL:${X11_FOUND}>:HAVE_X11>
  )

  target_compile_options(${TARGET}
    PRIVATE
      $<$<BOOL:${MSVC}>:/wd4244 /wd4267 /wd4273>
  )

  if("${C_COMPILER_ID}" MATCHES "GNU")
    target_link_options(${TARGET}
      PRIVATE
        -Wl,--no-undefined
    )
  endif()

  set_target_properties(${TARGET}
    PROPERTIES
      OSX_ARCHITECTURES "x86_64;arm64"
      POSITION_INDEPENDENT_CODE ON
  )
endfunction()

#######################################################################################################################
# audio_decoder

add_library(carla-audio-decoder STATIC)
add_library(carla::audio-decoder ALIAS carla-audio-decoder)

set_common_target_properties(carla-audio-decoder)

target_include_directories(carla-audio-decoder
  PRIVATE
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-audio-decoder
  PRIVATE
    PkgConfig::SNDFILE
)

target_sources(carla-audio-decoder
  PRIVATE
    ../source/modules/audio_decoder/ad_dr_mp3.c
    ../source/modules/audio_decoder/ad_ffmpeg.c
    ../source/modules/audio_decoder/ad_minimp3.c
    ../source/modules/audio_decoder/ad_plugin.c
    ../source/modules/audio_decoder/ad_soundfile.c
)

#######################################################################################################################
# jackbridge

add_library(carla-jackbridge STATIC)
add_library(carla::jackbridge ALIAS carla-jackbridge)

set_common_target_properties(carla-jackbridge)

target_include_directories(carla-jackbridge
  PRIVATE
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-jackbridge
  PRIVATE
    ${CARLA_LIBDL}
    ${CARLA_LIBRT}
)

target_sources(carla-jackbridge
  PRIVATE
    ../source/jackbridge/JackBridge1.cpp
    ../source/jackbridge/JackBridge2.cpp
)

#######################################################################################################################
# lilv

# serd
add_library(carla-lilv_serd STATIC)

set_common_target_properties(carla-lilv_serd)

target_compile_options(carla-lilv_serd
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133>
)

target_include_directories(carla-lilv_serd
  PRIVATE
    ../source/includes
    ../source/modules/lilv/config
    ../source/modules/lilv/serd-0.24.0
)

target_sources(carla-lilv_serd
  PRIVATE
    ../source/modules/lilv/serd.c
)

# sord
add_library(carla-lilv_sord STATIC)

set_common_target_properties(carla-lilv_sord)

target_compile_options(carla-lilv_sord
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133>
    # workaround compiler bug, see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109585
    $<$<C_COMPILER_ID:GNU>:-fno-strict-aliasing>
)

target_include_directories(carla-lilv_sord
  PRIVATE
    ../source/includes
    ../source/modules/lilv/config
    ../source/modules/lilv/sord-0.16.0
    ../source/modules/lilv/sord-0.16.0/src
)

target_link_libraries(carla-lilv_sord
  PRIVATE
    carla-lilv_serd
)

target_sources(carla-lilv_sord
  PRIVATE
    ../source/modules/lilv/sord.c
)

# sratom
add_library(carla-lilv_sratom STATIC)

set_common_target_properties(carla-lilv_sratom)

target_compile_options(carla-lilv_sratom
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133>
)

target_include_directories(carla-lilv_sratom
  PRIVATE
    ../source/includes
    ../source/modules/lilv/config
    ../source/modules/lilv/sratom-0.6.0
)

target_link_libraries(carla-lilv_sratom
  PRIVATE
    carla-lilv_serd
)

target_sources(carla-lilv_sratom
  PRIVATE
    ../source/modules/lilv/sratom.c
)

# lilv
add_library(carla-lilv_lilv STATIC)

set_common_target_properties(carla-lilv_lilv)

target_compile_options(carla-lilv_lilv
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133>
    $<$<C_COMPILER_ID:GNU>:-Wno-deprecated-declarations -Wno-discarded-qualifiers>
)

target_include_directories(carla-lilv_lilv
  PRIVATE
    ../source/includes
    ../source/modules/lilv/config
    ../source/modules/lilv/lilv-0.24.0
    ../source/modules/lilv/lilv-0.24.0/src
)

target_link_libraries(carla-lilv_lilv
  PRIVATE
    carla-lilv_serd
    carla-lilv_sord
    carla-lilv_sratom
)

target_sources(carla-lilv_lilv
  PRIVATE
    ../source/modules/lilv/lilv.c
)

# combined target
add_library(carla-lilv INTERFACE)
add_library(carla::lilv ALIAS carla-lilv)

target_link_libraries(carla-lilv
  INTERFACE
    carla-lilv_serd
    carla-lilv_sord
    carla-lilv_sratom
    carla-lilv_lilv
    ${CARLA_LIBDL}
    ${CARLA_LIBM}
    ${CARLA_LIBRT}
)

#######################################################################################################################
# native-plugins

add_library(carla-native-plugins STATIC)
add_library(carla::native-plugins ALIAS carla-native-plugins)

set_common_target_properties(carla-native-plugins)

target_include_directories(carla-native-plugins
  PRIVATE
    ../source/includes
    ../source/modules
    ../source/utils
)

target_sources(carla-native-plugins
  PRIVATE
    ../source/native-plugins/_all.c
    ../source/native-plugins/_data.cpp
    ../source/native-plugins/audio-gain.c
    ../source/native-plugins/bypass.c
    ../source/native-plugins/cv-to-audio.c
    ../source/native-plugins/lfo.c
    ../source/native-plugins/midi-channel-filter.c
    ../source/native-plugins/midi-channel-ab.c
    ../source/native-plugins/midi-channelize.c
    ../source/native-plugins/midi-gain.c
    ../source/native-plugins/midi-join.c
    ../source/native-plugins/midi-split.c
    ../source/native-plugins/midi-to-cv.c
    ../source/native-plugins/midi-through.c
    ../source/native-plugins/midi-transpose.c
    ../source/native-plugins/audio-file.cpp
    ../source/native-plugins/midi-file.cpp
    # these rely on PyQt, skip them
    # ../source/native-plugins/bigmeter.cpp
    # ../source/native-plugins/midi-pattern.cpp
    # ../source/native-plugins/notes.cpp
    # ../source/native-plugins/xycontroller.cpp
)

#######################################################################################################################
# rtmempool

add_library(carla-rtmempool STATIC)
add_library(carla::rtmempool ALIAS carla-rtmempool)

set_common_target_properties(carla-rtmempool)

target_include_directories(carla-rtmempool
  PRIVATE
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-rtmempool
  PRIVATE
    ${CARLA_LIBDL}
    ${CARLA_LIBRT}
)

target_sources(carla-rtmempool
  PRIVATE
    ../source/modules/rtmempool/rtmempool.c
)

#######################################################################################################################
# sfzero

add_library(carla-sfzero STATIC)
add_library(carla::sfzero ALIAS carla-sfzero)

set_common_target_properties(carla-sfzero)

target_include_directories(carla-sfzero
  PRIVATE
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-sfzero
  PRIVATE
    carla-audio-decoder
)

target_sources(carla-sfzero
  PRIVATE
    ../source/modules/sfzero/SFZero.cpp
)

#######################################################################################################################
# water

add_library(carla-water STATIC)
add_library(carla::water ALIAS carla-water)

set_common_target_properties(carla-water)

target_compile_options(carla-water
  PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wno-deprecated-copy>
)

target_include_directories(carla-water
  PRIVATE
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-water
  PRIVATE
    $<$<BOOL:${APPLE}>:${APPKIT}>
    $<$<BOOL:${WIN32}>:comdlg32>
    $<$<BOOL:${WIN32}>:ole32>
    $<$<BOOL:${WIN32}>:winmm>
    ${CARLA_LIBDL}
    ${CARLA_LIBRT}
)

target_sources(carla-water
  PRIVATE
    ../source/modules/water/water.cpp
)

if (APPLE)
  set_source_files_properties(../source/modules/water/water.cpp PROPERTIES COMPILE_FLAGS -ObjC++)
endif()

#######################################################################################################################
# water-files

add_library(carla-water-files STATIC)
add_library(carla::water-files ALIAS carla-water-files)

set_common_target_properties(carla-water-files)

target_compile_options(carla-water-files
  PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wno-deprecated-copy>
)

target_include_directories(carla-water-files
  PRIVATE
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-water-files
  PRIVATE
    $<$<BOOL:${APPLE}>:${APPKIT}>
    $<$<BOOL:${WIN32}>:ole32>
    $<$<BOOL:${WIN32}>:winmm>
    ${CARLA_LIBDL}
)

target_sources(carla-water-files
  PRIVATE
    ../source/modules/water/water.files.cpp
)

if (APPLE)
  set_source_files_properties(../source/modules/water/water.files.cpp PROPERTIES COMPILE_FLAGS -ObjC++)
endif()

#######################################################################################################################
# ysfx

add_library(carla-ysfx STATIC)
add_library(carla::ysfx ALIAS carla-ysfx)

set_common_target_properties(carla-ysfx)

# YSFX_FTS_LACKS_LFS_SUPPORT

target_compile_definitions(carla-ysfx
  PRIVATE
    EEL_TARGET_PORTABLE
    EELSCRIPT_NO_NET
    EELSCRIPT_NO_LICE
    NSEEL_ATOF=ysfx_wdl_atof
    WDL_FFT_REALSIZE=8
    WDL_LINEPARSE_ATOF=ysfx_wdl_atof
    YSFX_API=
    YSFX_NO_GFX
    YSFX_NO_STANDARD_MUTEX
    $<$<BOOL:${WIN32}>:NOMINMAX>
    $<$<NOT:$<BOOL:${MINGW}>>:_FILE_OFFSET_BITS=64>
)

target_compile_options(carla-ysfx
  PRIVATE
    $<$<C_COMPILER_ID:GNU>:-fsigned-char>
    $<$<C_COMPILER_ID:GNU>:-Wno-ignored-attributes>
    $<$<C_COMPILER_ID:GNU>:-Wno-sign-compare>
    $<$<C_COMPILER_ID:GNU>:-Wno-unused-function>
    $<$<C_COMPILER_ID:GNU>:-Wno-unused-parameter>
)

target_include_directories(carla-ysfx
  PRIVATE
    ../source/modules/ysfx/include
    ../source/modules/ysfx/sources
    ../source/modules/ysfx/thirdparty/dr_libs
    ../source/modules/ysfx/thirdparty/stb
    ../source/modules/ysfx/thirdparty/WDL/source
)

target_link_libraries(carla-ysfx
  PRIVATE
)

target_sources(carla-ysfx
  PRIVATE
    ../source/modules/ysfx/sources/ysfx.cpp
    ../source/modules/ysfx/sources/ysfx_api_eel.cpp
    ../source/modules/ysfx/sources/ysfx_api_file.cpp
    ../source/modules/ysfx/sources/ysfx_api_gfx.cpp
    ../source/modules/ysfx/sources/ysfx_api_reaper.cpp
    ../source/modules/ysfx/sources/ysfx_audio_flac.cpp
    ../source/modules/ysfx/sources/ysfx_audio_wav.cpp
    ../source/modules/ysfx/sources/ysfx_config.cpp
    ../source/modules/ysfx/sources/ysfx_eel_utils.cpp
    ../source/modules/ysfx/sources/ysfx_midi.cpp
    ../source/modules/ysfx/sources/ysfx_parse.cpp
    ../source/modules/ysfx/sources/ysfx_reader.cpp
    ../source/modules/ysfx/sources/ysfx_utils.cpp
    ../source/modules/ysfx/sources/ysfx_utils_fts.cpp
    ../source/modules/ysfx/sources/eel2-gas/sources/asm-nseel-x64-sse.S
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-caltab.c
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-cfunc.c
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-compiler.c
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-eval.c
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-lextab.c
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-ram.c
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/eel2/nseel-yylex.c
    ../source/modules/ysfx/thirdparty/WDL/source/WDL/fft.c
    $<$<BOOL:${WIN32}>:../source/modules/ysfx/thirdparty/WDL/source/WDL/win32_utf8.c>
)

#######################################################################################################################
# zita-resampler

add_library(carla-zita-resampler STATIC)
add_library(carla::zita-resampler ALIAS carla-zita-resampler)

set_common_target_properties(carla-zita-resampler)

target_include_directories(carla-zita-resampler
  PRIVATE
    ../source/includes
)

target_sources(carla-zita-resampler
  PRIVATE
    ../source/modules/zita-resampler/cresampler.cc
    ../source/modules/zita-resampler/resampler-table.cc
    ../source/modules/zita-resampler/resampler.cc
    ../source/modules/zita-resampler/vresampler.cc
)

#######################################################################################################################
# carla bridge

add_executable(carla-bridge-native)

set_common_target_properties(carla-bridge-native)

target_compile_definitions(carla-bridge-native
  PRIVATE
    BUILD_BRIDGE
    CARLA_LIB_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

target_include_directories(carla-bridge-native
  PRIVATE
    ../source
    ../source/backend
    ../source/backend/engine
    ../source/backend/plugin
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-bridge-native
  PRIVATE
    carla-audio-decoder
    carla-jackbridge
    carla-lilv
    carla-native-plugins
    carla-rtmempool
    carla-sfzero
    carla-water
    carla-ysfx
    carla-zita-resampler
    PkgConfig::FLUIDSYNTH
    PkgConfig::LIBMAGIC
    PkgConfig::X11
)

target_sources(carla-bridge-native
  PRIVATE
    ../source/bridges-plugin/CarlaBridgePlugin.cpp
    ../source/backend/CarlaStandalone.cpp
    ../source/backend/engine/CarlaEngine.cpp
    ../source/backend/engine/CarlaEngineBridge.cpp
    ../source/backend/engine/CarlaEngineClient.cpp
    ../source/backend/engine/CarlaEngineDummy.cpp
    ../source/backend/engine/CarlaEngineData.cpp
    ../source/backend/engine/CarlaEngineGraph.cpp
    ../source/backend/engine/CarlaEngineInternal.cpp
    ../source/backend/engine/CarlaEnginePorts.cpp
    ../source/backend/engine/CarlaEngineRunner.cpp
    ../source/backend/plugin/CarlaPlugin.cpp
    ../source/backend/plugin/CarlaPluginBridge.cpp
    ../source/backend/plugin/CarlaPluginInternal.cpp
    ../source/backend/plugin/CarlaPluginAU.cpp
    ../source/backend/plugin/CarlaPluginCLAP.cpp
    ../source/backend/plugin/CarlaPluginFluidSynth.cpp
    ../source/backend/plugin/CarlaPluginJuce.cpp
    ../source/backend/plugin/CarlaPluginJSFX.cpp
    ../source/backend/plugin/CarlaPluginLADSPADSSI.cpp
    ../source/backend/plugin/CarlaPluginLV2.cpp
    ../source/backend/plugin/CarlaPluginNative.cpp
    ../source/backend/plugin/CarlaPluginSFZero.cpp
    ../source/backend/plugin/CarlaPluginVST2.cpp
    ../source/backend/plugin/CarlaPluginVST3.cpp
)

if (APPLE)
  set_source_files_properties(
    ../source/bridges-plugin/CarlaBridgePlugin.cpp
    ../source/backend/CarlaStandalone.cpp
    ../source/backend/plugin/CarlaPluginCLAP.cpp
    ../source/backend/plugin/CarlaPluginVST2.cpp
    ../source/backend/plugin/CarlaPluginVST3.cpp
      PROPERTIES COMPILE_FLAGS -ObjC++)
endif()

#######################################################################################################################
# carla discovery

add_executable(carla-discovery-native)

set_common_target_properties(carla-discovery-native)

target_include_directories(carla-discovery-native
  PRIVATE
    ../source/backend
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-discovery-native
  PRIVATE
    carla-lilv
    carla-water-files
    carla-ysfx
    PkgConfig::FLUIDSYNTH
)

target_sources(carla-discovery-native
  PRIVATE
    ../source/discovery/carla-discovery.cpp
)

if (APPLE)
  set_source_files_properties(../source/discovery/carla-discovery.cpp PROPERTIES COMPILE_FLAGS -ObjC++)
endif()

#######################################################################################################################
# carla utils

add_library(carla-utils SHARED)
add_library(carla::utils ALIAS carla-utils)

set_common_target_properties(carla-utils)

target_include_directories(carla-utils
  PRIVATE
    ../source/backend
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-utils
  PRIVATE
    carla-lilv
    carla-water-files
    carla-ysfx
    PkgConfig::FLUIDSYNTH
    PkgConfig::X11
)

target_sources(carla-utils
  PRIVATE
    ../source/backend/utils/CachedPlugins.cpp
    ../source/backend/utils/Information.cpp
    ../source/backend/utils/JUCE.cpp
    ../source/backend/utils/PipeClient.cpp
    ../source/backend/utils/PluginDiscovery.cpp
    ../source/backend/utils/System.cpp
    ../source/backend/utils/Windows.cpp
)

if(APPLE)
  set_source_files_properties(../source/backend/utils/Windows.cpp PROPERTIES COMPILE_FLAGS -ObjC++)
endif()

#######################################################################################################################
