cmake_minimum_required(VERSION 3.15)

# adjust for last compatible macOS version
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.8" CACHE STRING "Minimum macOS deployment version")
  set(CARLA_OSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
endif()

project(carla)

set(PROJECT_VERSION 2.6.0-alpha1)
set(PROJECT_SHORT_VERSION 2.5.91)

set(CMAKE_POLICY_DEFAULT_CMP0025 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0126 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

#######################################################################################################################
# fix compat with cmake < 3.26

if(CMAKE_VERSION VERSION_LESS 3.26)
  if(MSVC)
    set(CMAKE_C_COMPILER_FRONTEND_VARIANT "MSVC")
  else()
    set(CMAKE_C_COMPILER_FRONTEND_VARIANT "GNU")
  endif()
  mark_as_advanced(CMAKE_C_COMPILER_FRONTEND_VARIANT)
endif()

#######################################################################################################################
# build options

if(MSVC)
  set(CARLA_USE_JACK_DEFAULT FALSE)
  set(CARLA_USE_OSC_DEFAULT FALSE)
else()
  set(CARLA_USE_JACK_DEFAULT TRUE)
  set(CARLA_USE_OSC_DEFAULT TRUE)
endif()

set(CARLA_BUILD_FRAMEWORKS FALSE CACHE BOOL "Build libraries as macOS frameworks")
set(CARLA_BUILD_STATIC FALSE CACHE BOOL "Build static libraries")
set(CARLA_ENABLE_JSFX TRUE CACHE BOOL "Enable JSFX")
set(CARLA_USE_JACK ${CARLA_USE_JACK_DEFAULT} CACHE BOOL "Enable JACK")
set(CARLA_USE_OSC ${CARLA_USE_OSC_DEFAULT} CACHE BOOL "Enable OSC")

if(${CARLA_BUILD_FRAMEWORKS} AND NOT APPLE)
  set(CARLA_BUILD_FRAMEWORKS FALSE)
  mark_as_advanced(CARLA_BUILD_FRAMEWORKS)
endif()

if(${CARLA_BUILD_STATIC})
  set(CARLA_LIB_TYPE STATIC)
else()
  set(CARLA_LIB_TYPE SHARED)
endif()

if(${CARLA_BUILD_FRAMEWORKS})
  execute_process(COMMAND date -u +%y%m%d%H%M%S OUTPUT_VARIABLE CARLA_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

#######################################################################################################################
# required dependencies

include(GNUInstallDirs)

find_package(Threads REQUIRED)

if(NOT (APPLE OR HAIKU OR WIN32))
  set(CARLA_LIBDL dl)
  set(CARLA_LIBM m)
  set(CARLA_LIBRT rt)
endif()

#######################################################################################################################
# optional dependencies

find_package(PkgConfig QUIET)

# homebrew does not support universal binaries, disable external deps if it is in use
if(APPLE)
  find_program(HOMEBREW brew)
  if(HOMEBREW)
    set(PKGCONFIG_FOUND FALSE)
    mark_as_advanced(PKGCONFIG_FOUND)
  endif()
endif()

if(PKGCONFIG_FOUND)
  pkg_check_modules(FLUIDSYNTH IMPORTED_TARGET fluidsynth)
  pkg_check_modules(SNDFILE IMPORTED_TARGET sndfile)
else()
  set(FLUIDSYNTH_FOUND FALSE)
  set(SNDFILE_FOUND FALSE)
endif()

if(PKGCONFIG_FOUND AND CARLA_USE_OSC)
  pkg_check_modules(LIBLO IMPORTED_TARGET liblo)
else()
  set(LIBLO_FOUND FALSE)
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

if(NOT LIBLO_FOUND)
  add_library(PkgConfig::LIBLO ALIAS carla-none)
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

# TODO
set(PYQT_FOUND FALSE)

#######################################################################################################################
# utilities

if(WIN32)
  set(CARLA_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
  set(CARLA_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
else()
  set(CARLA_INSTALL_BINDIR ${CMAKE_INSTALL_LIBDIR}/carla)
  set(CARLA_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/carla)
endif()
mark_as_advanced(CARLA_INSTALL_LIBDIR)

function(set_common_target_properties TARGET)
  target_compile_definitions(${TARGET}
    PRIVATE
      BUILDING_CARLA
      $<$<BOOL:${MSVC}>:_USE_MATH_DEFINES>
    PUBLIC
      $<$<BOOL:${CARLA_ENABLE_JSFX}>:HAVE_YSFX>
      $<$<BOOL:${CARLA_USE_JACK}>:HAVE_JACK>
      $<$<BOOL:${FLUIDSYNTH_FOUND}>:HAVE_FLUIDSYNTH>
      $<$<BOOL:${LIBLO_FOUND}>:HAVE_LIBLO>
      $<$<BOOL:${LIBMAGIC_FOUND}>:HAVE_LIBMAGIC>
      $<$<BOOL:${PYQT_FOUND}>:HAVE_PYQT>
      $<$<BOOL:${SNDFILE_FOUND}>:HAVE_SNDFILE>
      $<$<BOOL:${X11_FOUND}>:HAVE_X11>
  )

  target_compile_options(${TARGET}
    PRIVATE
      $<$<BOOL:${MSVC}>:/wd4244>
      $<$<BOOL:${MSVC}>:/wd4267>
      $<$<BOOL:${MSVC}>:/wd4273>
  )

  target_include_directories(${TARGET}
    PRIVATE
      ../source/modules/distrho
  )

  target_link_options(${TARGET}
    PRIVATE
      $<$<C_COMPILER_ID:GNU>:-Wl,--no-undefined>
  )

  set_property(TARGET ${TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)

  if(APPLE)
    set_property(TARGET ${TARGET} APPEND PROPERTY OSX_ARCHITECTURES arm64)
    set_property(TARGET ${TARGET} APPEND PROPERTY OSX_ARCHITECTURES x86_64)
  endif()
endfunction()

#######################################################################################################################
# setup pthreads for msvc

if(MSVC)
  include(FetchContent)
  FetchContent_Declare(pthreads4w
    GIT_REPOSITORY https://git.code.sf.net/p/pthreads4w/code
    GIT_TAG f12b445b336ee0117b43fca1d4b9f22c9af82c36
  )
  FetchContent_MakeAvailable(pthreads4w)

  add_library(pthreads4w STATIC)
  add_library(carla::pthreads4w ALIAS pthreads4w)

  target_sources(pthreads4w PRIVATE ${pthreads4w_SOURCE_DIR}/pthread.c)
  target_compile_definitions(pthreads4w
    PRIVATE
      HAVE_CONFIG_H
      HAVE_STDINT_H=1
      _POSIX_C_SOURCE=200112L
    PUBLIC
      INCLUDE_NP
      PTW32_DLLPORT
      __PTW32_STATIC_LIB
  )

  target_include_directories(pthreads4w
    PUBLIC
      ${pthreads4w_SOURCE_DIR}
  )
  set(CARLA_PTHREADS carla::pthreads4w)
else()
  set(CARLA_PTHREADS ${CMAKE_THREAD_LIBS_INIT})
endif()

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
  PUBLIC
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
  PUBLIC
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
    $<$<BOOL:${MSVC}>:/wd4005>
    $<$<BOOL:${MSVC}>:/wd4090>
    $<$<BOOL:${MSVC}>:/wd4133>
    $<$<C_COMPILER_ID:GNU>:-Wno-format-overflow>
    $<$<C_COMPILER_ID:GNU>:-Wno-implicit-fallthrough>
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
    $<$<BOOL:${MSVC}>:/wd4005>
    $<$<BOOL:${MSVC}>:/wd4090>
    $<$<BOOL:${MSVC}>:/wd4133>
    $<$<C_COMPILER_ID:GNU>:-Wno-maybe-uninitialized>
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-unused-parameter>
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
    $<$<BOOL:${MSVC}>:/wd4005>
    $<$<BOOL:${MSVC}>:/wd4090>
    $<$<BOOL:${MSVC}>:/wd4133>
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
    $<$<BOOL:${MSVC}>:/wd4005>
    $<$<BOOL:${MSVC}>:/wd4090>
    $<$<BOOL:${MSVC}>:/wd4133>
    $<$<C_COMPILER_ID:AppleClang>:-Wno-incompatible-pointer-types-discards-qualifiers>
    $<$<C_COMPILER_ID:GNU>:-Wno-deprecated-declarations>
    $<$<C_COMPILER_ID:GNU>:-Wno-discarded-qualifiers>
    $<$<C_COMPILER_ID:GNU>:-Wno-format-overflow>
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-unused-parameter>
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

target_link_libraries(carla-native-plugins
  PUBLIC
    ${CARLA_PTHREADS}
)

target_sources(carla-native-plugins
  PRIVATE
    ../source/native-plugins/_all.c
    ../source/native-plugins/_data.cpp
    ../source/native-plugins/audio-gain.c
    ../source/native-plugins/bypass.c
    ../source/native-plugins/cv-to-audio.c
    ../source/native-plugins/lfo.c
    ../source/native-plugins/quadpanner.c
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
    # these rely on PyQt
    $<$<BOOL:${PYQT_FOUND}>:../source/native-plugins/bigmeter.cpp>
    $<$<BOOL:${PYQT_FOUND}>:../source/native-plugins/midi-pattern.cpp>
    $<$<BOOL:${PYQT_FOUND}>:../source/native-plugins/notes.cpp>
    $<$<BOOL:${PYQT_FOUND}>:../source/native-plugins/xycontroller.cpp>
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
  PUBLIC
    ${CARLA_LIBDL}
    ${CARLA_LIBRT}
    ${CARLA_PTHREADS}
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
  PUBLIC
    ${CARLA_PTHREADS}
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
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-deprecated-copy>
)

target_include_directories(carla-water
  PRIVATE
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-water
  PUBLIC
    $<$<BOOL:${APPLE}>:$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>>
    $<$<BOOL:${WIN32}>:comdlg32>
    $<$<BOOL:${WIN32}>:ole32>
    $<$<BOOL:${WIN32}>:winmm>
    ${CARLA_LIBDL}
    ${CARLA_LIBRT}
    ${CARLA_PTHREADS}
)

target_sources(carla-water
  PRIVATE
    ../source/modules/water/water.cpp
)

#######################################################################################################################
# water-files

add_library(carla-water-files STATIC)
add_library(carla::water-files ALIAS carla-water-files)

set_common_target_properties(carla-water-files)

target_compile_options(carla-water-files
  PRIVATE
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-deprecated-copy>
)

target_include_directories(carla-water-files
  PRIVATE
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-water-files
  PUBLIC
    $<$<BOOL:${APPLE}>:$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>>
    $<$<BOOL:${WIN32}>:ole32>
    $<$<BOOL:${WIN32}>:winmm>
    ${CARLA_LIBDL}
)

target_sources(carla-water-files
  PRIVATE
    ../source/modules/water/water.files.cpp
)

#######################################################################################################################
# ysfx

if(${CARLA_ENABLE_JSFX})

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
    WDL_WIN32_UTF8_NO_UI_IMPL
    YSFX_API=
    YSFX_NO_GFX
    YSFX_NO_STANDARD_MUTEX
    $<$<BOOL:${WIN32}>:NOMINMAX>
    $<$<NOT:$<BOOL:${MINGW}>>:_FILE_OFFSET_BITS=64>
)

# NOTE ugly -U /U due to cmake not supporting `target_remove_definitions`
# see https://gitlab.kitware.com/cmake/cmake/-/issues/19796

target_compile_options(carla-ysfx
  PRIVATE
    $<$<BOOL:${MINGW}>:-UUNICODE>
    $<$<BOOL:${MINGW}>:-U_UNICODE>
    $<$<BOOL:${MSVC}>:/wd4018>
    $<$<BOOL:${MSVC}>:/wd4297>
    $<$<BOOL:${MSVC}>:/UUNICODE>
    $<$<BOOL:${MSVC}>:/U_UNICODE>
    $<$<C_COMPILER_ID:AppleClang>:-Wno-newline-eof>
    $<$<C_COMPILER_ID:GNU>:-Wno-extra>
    $<$<C_COMPILER_ID:GNU>:-Wno-ignored-attributes>
    $<$<C_COMPILER_ID:GNU>:-Wno-unused-function>
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-fsigned-char>
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-sign-compare>
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-unused-parameter>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>>:-Wno-missing-field-initializers>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-deprecated-copy>
)

target_include_directories(carla-ysfx
  PRIVATE
    ../source/modules/ysfx/include
    ../source/modules/ysfx/sources
    ../source/modules/ysfx/thirdparty/dr_libs
    ../source/modules/ysfx/thirdparty/stb
    ../source/modules/ysfx/thirdparty/WDL/source
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

endif(${CARLA_ENABLE_JSFX})

#######################################################################################################################
# zita-resampler

add_library(carla-zita-resampler STATIC)
add_library(carla::zita-resampler ALIAS carla-zita-resampler)

set_common_target_properties(carla-zita-resampler)

target_include_directories(carla-zita-resampler
  PRIVATE
    ../source/includes
)

target_link_libraries(carla-zita-resampler
  PUBLIC
    ${CARLA_PTHREADS}
)

target_sources(carla-zita-resampler
  PRIVATE
    ../source/modules/zita-resampler/cresampler.cc
    ../source/modules/zita-resampler/resampler-table.cc
    ../source/modules/zita-resampler/resampler.cc
    ../source/modules/zita-resampler/vresampler.cc
)

#######################################################################################################################
# carla bridge-native

add_executable(carla-bridge-native)

set_common_target_properties(carla-bridge-native)

if(NOT ${CARLA_BUILD_FRAMEWORKS})
  install(TARGETS carla-bridge-native
    RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
  )
endif()

target_compile_definitions(carla-bridge-native
  PRIVATE
    BUILD_BRIDGE
    CARLA_LIB_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

# FIXME
target_compile_options(carla-bridge-native
  PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wno-format-truncation>
    $<$<C_COMPILER_ID:GNU>:-Wno-stringop-overflow>
    $<$<C_COMPILER_ID:GNU>:-Wno-unused-but-set-variable>
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-unused-parameter>
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
    $<$<BOOL:${CARLA_ENABLE_JSFX}>:carla-ysfx>
    carla-zita-resampler
  PUBLIC
    PkgConfig::FLUIDSYNTH
    PkgConfig::LIBLO
    PkgConfig::LIBMAGIC
    PkgConfig::X11
    $<$<BOOL:${APPLE}>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>
    ${CARLA_PTHREADS}
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
    ../source/backend/plugin/CarlaPluginJSFX.cpp
    ../source/backend/plugin/CarlaPluginLADSPADSSI.cpp
    ../source/backend/plugin/CarlaPluginLV2.cpp
    ../source/backend/plugin/CarlaPluginNative.cpp
    ../source/backend/plugin/CarlaPluginSFZero.cpp
    ../source/backend/plugin/CarlaPluginVST2.cpp
    ../source/backend/plugin/CarlaPluginVST3.cpp
    $<$<BOOL:${CARLA_USE_JACK}>:../source/backend/engine/CarlaEngineJack.cpp>
    $<$<BOOL:${CARLA_USE_JACK}>:../source/backend/plugin/CarlaPluginJack.cpp>
)

#######################################################################################################################
# carla bridge-lv2-gtk2

add_executable(carla-bridge-lv2-gtk2)

set_common_target_properties(carla-bridge-lv2-gtk2)

if(NOT ${CARLA_BUILD_FRAMEWORKS})
  install(TARGETS carla-bridge-lv2-gtk2
    RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
  )
endif()

target_compile_definitions(carla-bridge-lv2-gtk2
  PRIVATE
    BUILD_BRIDGE
    BUILD_BRIDGE_UI
    BRIDGE_GTK2
)

target_include_directories(carla-bridge-lv2-gtk2
  PRIVATE
    ../source/backend
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-bridge-lv2-gtk2
  PRIVATE
    carla-lilv
    carla-water-files
  PUBLIC
    PkgConfig::X11
    ${CARLA_PTHREADS}
)

target_sources(carla-bridge-lv2-gtk2
  PRIVATE
    ../source/bridges-ui/CarlaBridgeFormat.cpp
    ../source/bridges-ui/CarlaBridgeFormatLV2.cpp
    ../source/bridges-ui/CarlaBridgeToolkitGtk.cpp
)

#######################################################################################################################
# carla bridge-lv2-gtk3

add_executable(carla-bridge-lv2-gtk3)

set_common_target_properties(carla-bridge-lv2-gtk3)

if(NOT ${CARLA_BUILD_FRAMEWORKS})
  install(TARGETS carla-bridge-lv2-gtk3
    RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
  )
endif()

target_compile_definitions(carla-bridge-lv2-gtk3
  PRIVATE
    BUILD_BRIDGE
    BUILD_BRIDGE_UI
    BRIDGE_GTK3
)

target_include_directories(carla-bridge-lv2-gtk3
  PRIVATE
    ../source/backend
    ../source/includes
    ../source/modules
    ../source/utils
)

target_link_libraries(carla-bridge-lv2-gtk3
  PRIVATE
    carla-lilv
    carla-water-files
  PUBLIC
    PkgConfig::X11
    ${CARLA_PTHREADS}
)

target_sources(carla-bridge-lv2-gtk3
  PRIVATE
    ../source/bridges-ui/CarlaBridgeFormat.cpp
    ../source/bridges-ui/CarlaBridgeFormatLV2.cpp
    ../source/bridges-ui/CarlaBridgeToolkitGtk.cpp
)

#######################################################################################################################
# carla discovery-native

add_executable(carla-discovery-native)

set_common_target_properties(carla-discovery-native)

if(NOT ${CARLA_BUILD_FRAMEWORKS})
  install(TARGETS carla-discovery-native
    RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
  )
endif()

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
    $<$<BOOL:${CARLA_ENABLE_JSFX}>:carla-ysfx>
  PUBLIC
    PkgConfig::FLUIDSYNTH
    ${CARLA_PTHREADS}
)

target_sources(carla-discovery-native
  PRIVATE
    ../source/discovery/carla-discovery.cpp
)

#######################################################################################################################
# carla host-plugin

add_library(carla-host-plugin ${CARLA_LIB_TYPE})
add_library(carla::host-plugin ALIAS carla-host-plugin)

set_common_target_properties(carla-host-plugin)

if(${CARLA_BUILD_FRAMEWORKS})
  set(carla-host-plugin-public-headers
    ../source/backend/CarlaBackend.h
    ../source/backend/CarlaHost.h
    ../source/backend/CarlaUtils.h
  )

  set_target_properties(carla-host-plugin
    PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      OUTPUT_NAME carla-host-plugin
      PUBLIC_HEADER "${carla-host-plugin-public-headers}"
      MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-framework.plist.in"
      MACOSX_FRAMEWORK_IDENTIFIER "studio.kx.carla.host-plugin"
      MACOSX_FRAMEWORK_BUNDLE_VERSION "${CARLA_DATE}"
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${PROJECT_SHORT_VERSION}"
  )
else()
  set_target_properties(carla-host-plugin
    PROPERTIES
      OUTPUT_NAME carla_host-plugin
      IMPORT_PREFIX lib
      PREFIX lib
  )
endif()

target_compile_definitions(carla-host-plugin
  PRIVATE
    CARLA_PLUGIN_BUILD
    CARLA_LIB_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

# FIXME
# target_compile_options(carla-host-plugin
#   PRIVATE
#     $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-unused-parameter>
# )

target_include_directories(carla-host-plugin
  PRIVATE
    ../source
    ../source/modules
  PUBLIC
    ../source/backend
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-host-plugin
  PRIVATE
    carla-jackbridge
    carla-lilv
    carla-native-plugins
    carla-rtmempool
    carla-sfzero
    carla-water
    $<$<BOOL:${CARLA_ENABLE_JSFX}>:carla-ysfx>
    carla-zita-resampler
  PUBLIC
    PkgConfig::FLUIDSYNTH
    PkgConfig::LIBLO
    PkgConfig::LIBMAGIC
    PkgConfig::X11
    $<$<BOOL:${APPLE}>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>
    ${CARLA_PTHREADS}
)

target_sources(carla-host-plugin
  PRIVATE
    ../source/backend/engine/CarlaEngine.cpp
    ../source/backend/engine/CarlaEngineClient.cpp
    ../source/backend/engine/CarlaEngineData.cpp
    ../source/backend/engine/CarlaEngineGraph.cpp
    ../source/backend/engine/CarlaEngineInternal.cpp
    ../source/backend/engine/CarlaEngineNative.cpp
    ../source/backend/engine/CarlaEngineOsc.cpp
    ../source/backend/engine/CarlaEngineOscHandlers.cpp
    ../source/backend/engine/CarlaEngineOscSend.cpp
    ../source/backend/engine/CarlaEnginePorts.cpp
    ../source/backend/engine/CarlaEngineRunner.cpp
    ../source/backend/plugin/CarlaPlugin.cpp
    ../source/backend/plugin/CarlaPluginBridge.cpp
    ../source/backend/plugin/CarlaPluginInternal.cpp
    ../source/backend/plugin/CarlaPluginAU.cpp
    ../source/backend/plugin/CarlaPluginCLAP.cpp
    ../source/backend/plugin/CarlaPluginFluidSynth.cpp
    ../source/backend/plugin/CarlaPluginJSFX.cpp
    ../source/backend/plugin/CarlaPluginLADSPADSSI.cpp
    ../source/backend/plugin/CarlaPluginLV2.cpp
    ../source/backend/plugin/CarlaPluginNative.cpp
    ../source/backend/plugin/CarlaPluginSFZero.cpp
    ../source/backend/plugin/CarlaPluginVST2.cpp
    ../source/backend/plugin/CarlaPluginVST3.cpp
    ../source/plugin/carla-host-plugin.cpp
    $<$<BOOL:${CARLA_USE_JACK}>:../source/backend/plugin/CarlaPluginJack.cpp>
  PUBLIC
    ${carla-host-plugin-public-headers}
)

install(TARGETS carla-host-plugin
  ARCHIVE DESTINATION ${CARLA_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CARLA_INSTALL_LIBDIR}
  FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
)

if(${CARLA_BUILD_FRAMEWORKS})
  install(FILES
    ../source/includes/CarlaDefines.h
    ../source/includes/CarlaNative.h
    ../source/includes/CarlaNativePlugin.h
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-standalone.framework/Versions/A/Headers/includes)
endif()

#######################################################################################################################
# carla native-plugin

add_library(carla-native-plugin ${CARLA_LIB_TYPE})
add_library(carla::native-plugin ALIAS carla-native-plugin)

set_common_target_properties(carla-native-plugin)

if(${CARLA_BUILD_FRAMEWORKS})
  set(carla-native-plugin-public-headers
    ../source/backend/CarlaBackend.h
    ../source/backend/CarlaHost.h
    ../source/backend/CarlaUtils.h
  )

  set_target_properties(carla-native-plugin
    PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      OUTPUT_NAME carla-native-plugin
      PUBLIC_HEADER "${carla-native-plugin-public-headers}"
      MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-framework.plist.in"
      MACOSX_FRAMEWORK_IDENTIFIER "studio.kx.carla.native-plugin"
      MACOSX_FRAMEWORK_BUNDLE_VERSION "${CARLA_DATE}"
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${PROJECT_SHORT_VERSION}"
  )
else()
  set_target_properties(carla-native-plugin
    PROPERTIES
      OUTPUT_NAME carla_native-plugin
      IMPORT_PREFIX lib
      PREFIX lib
  )
endif()

target_compile_definitions(carla-native-plugin
  PRIVATE
    CARLA_PLUGIN_BUILD
    CARLA_LIB_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

# FIXME
# target_compile_options(carla-native-plugin
#   PRIVATE
#     $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-unused-parameter>
# )

target_include_directories(carla-native-plugin
  PRIVATE
    ../source
    ../source/modules
  PUBLIC
    ../source/backend
    ../source/includes
    ../source/utils
)

if(APPLE)
  target_link_options(carla-native-plugin
    PRIVATE
      -Wl,-exported_symbol,_carla_get_native_patchbay_plugin
      -Wl,-exported_symbol,_carla_get_native_patchbay16_plugin
      -Wl,-exported_symbol,_carla_get_native_patchbay32_plugin
      -Wl,-exported_symbol,_carla_get_native_patchbay64_plugin
      -Wl,-exported_symbol,_carla_get_native_patchbay_cv_plugin
      -Wl,-exported_symbol,_carla_get_native_patchbay_cv8_plugin
      -Wl,-exported_symbol,_carla_get_native_patchbay_cv32_plugin
      -Wl,-exported_symbol,_carla_get_native_patchbay_obs_plugin
      -Wl,-exported_symbol,_carla_create_native_plugin_host_handle
      -Wl,-exported_symbol,_carla_host_handle_free
      -Wl,-exported_symbol,_carla_get_native_plugin_engine
      -Wl,-exported_symbol,_carla_get_complete_license_text
      -Wl,-exported_symbol,_carla_get_supported_file_extensions
      -Wl,-exported_symbol,_carla_get_supported_features
      -Wl,-exported_symbol,_carla_get_library_filename
      -Wl,-exported_symbol,_carla_get_library_folder
  )
elseif(EMSCRIPTEN)
  target_link_options(carla-native-plugin
    PRIVATE
      -sSIDE_MODULE=2
      -sEXPORTED_FUNCTIONS="['carla_get_native_rack_plugin','carla_get_native_patchbay_plugin','carla_get_native_patchbay16_plugin','carla_get_native_patchbay32_plugin','carla_get_native_patchbay64_plugin','carla_get_native_patchbay_cv_plugin','carla_get_native_patchbay_cv8_plugin','carla_get_native_patchbay_cv32_plugin','carla_get_native_patchbay_obs_plugin','carla_create_native_plugin_host_handle','carla_host_handle_free','carla_get_native_plugin_engine','carla_get_complete_license_text','carla_get_supported_file_extensions','carla_get_supported_features','carla_get_library_filename','carla_get_library_folder']"
  )
elseif(MSVC)
  target_link_options(carla-native-plugin
    PRIVATE
      /DEF:${CMAKE_CURRENT_SOURCE_DIR}/../source/plugin/symbols/carla-native-plugin.def
  )
elseif(WIN32)
  target_link_options(carla-native-plugin
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/../source/plugin/symbols/carla-native-plugin.def
  )
else()
  target_link_options(carla-native-plugin
    PRIVATE
      -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../source/plugin/symbols/carla-native-plugin.version
  )
endif()

target_link_libraries(carla-native-plugin
  PRIVATE
    carla-jackbridge
    carla-lilv
    carla-native-plugins
    carla-rtmempool
    carla-sfzero
    carla-water
    $<$<BOOL:${CARLA_ENABLE_JSFX}>:carla-ysfx>
    carla-zita-resampler
  PUBLIC
    PkgConfig::FLUIDSYNTH
    PkgConfig::LIBLO
    PkgConfig::LIBMAGIC
    PkgConfig::X11
    $<$<BOOL:${APPLE}>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>
    ${CARLA_PTHREADS}
)

target_sources(carla-native-plugin
  PRIVATE
    ../source/backend/engine/CarlaEngine.cpp
    ../source/backend/engine/CarlaEngineClient.cpp
    ../source/backend/engine/CarlaEngineData.cpp
    ../source/backend/engine/CarlaEngineGraph.cpp
    ../source/backend/engine/CarlaEngineInternal.cpp
    ../source/backend/engine/CarlaEngineNative.cpp
    ../source/backend/engine/CarlaEngineOsc.cpp
    ../source/backend/engine/CarlaEngineOscHandlers.cpp
    ../source/backend/engine/CarlaEngineOscSend.cpp
    ../source/backend/engine/CarlaEnginePorts.cpp
    ../source/backend/engine/CarlaEngineRunner.cpp
    ../source/backend/plugin/CarlaPlugin.cpp
    ../source/backend/plugin/CarlaPluginBridge.cpp
    ../source/backend/plugin/CarlaPluginInternal.cpp
    ../source/backend/plugin/CarlaPluginAU.cpp
    ../source/backend/plugin/CarlaPluginCLAP.cpp
    ../source/backend/plugin/CarlaPluginFluidSynth.cpp
    ../source/backend/plugin/CarlaPluginJSFX.cpp
    ../source/backend/plugin/CarlaPluginLADSPADSSI.cpp
    ../source/backend/plugin/CarlaPluginLV2.cpp
    ../source/backend/plugin/CarlaPluginNative.cpp
    ../source/backend/plugin/CarlaPluginSFZero.cpp
    ../source/backend/plugin/CarlaPluginVST2.cpp
    ../source/backend/plugin/CarlaPluginVST3.cpp
    ../source/plugin/carla-native-plugin.cpp
    $<$<BOOL:${CARLA_USE_JACK}>:../source/backend/plugin/CarlaPluginJack.cpp>
  PUBLIC
    ${carla-native-plugin-public-headers}
)

install(TARGETS carla-native-plugin
  ARCHIVE DESTINATION ${CARLA_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CARLA_INSTALL_LIBDIR}
  FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
)

if(${CARLA_BUILD_FRAMEWORKS})
  install(FILES
    ../source/includes/CarlaDefines.h
    ../source/includes/CarlaNative.h
    ../source/includes/CarlaNativePlugin.h
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-standalone.framework/Versions/A/Headers/includes)
endif()

#######################################################################################################################
# carla standalone

add_library(carla-standalone ${CARLA_LIB_TYPE})
add_library(carla::standalone ALIAS carla-standalone)

set_common_target_properties(carla-standalone)

if(${CARLA_BUILD_FRAMEWORKS})
  set(carla-standalone-public-headers
    ../source/backend/CarlaBackend.h
    ../source/backend/CarlaHost.h
    ../source/backend/CarlaEngine.hpp
    ../source/backend/CarlaPlugin.hpp
  )

  set_target_properties(carla-standalone
    PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      OUTPUT_NAME carla-standalone
      PUBLIC_HEADER "${carla-standalone-public-headers}"
      MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-framework.plist.in"
      MACOSX_FRAMEWORK_IDENTIFIER "studio.kx.carla.standalone"
      MACOSX_FRAMEWORK_BUNDLE_VERSION "${CARLA_DATE}"
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${PROJECT_SHORT_VERSION}"
  )
else()
  set_target_properties(carla-standalone
    PROPERTIES
      OUTPUT_NAME carla_standalone2
      IMPORT_PREFIX lib
      PREFIX lib
  )
endif()

target_compile_definitions(carla-standalone
  PRIVATE
    CARLA_LIB_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

# FIXME
target_compile_options(carla-standalone
  PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wno-error=cpp>
    $<$<C_COMPILER_ID:GNU>:-Wno-format-truncation>
    $<$<C_COMPILER_ID:GNU>:-Wno-stringop-overflow>
    $<$<C_COMPILER_ID:GNU>:-Wno-unused-but-set-variable>
    $<$<STREQUAL:${CMAKE_C_COMPILER_FRONTEND_VARIANT},GNU>:-Wno-unused-parameter>
)

target_include_directories(carla-standalone
  PRIVATE
    ../source
    ../source/modules
  PUBLIC
    ../source/backend
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-standalone
  PRIVATE
    carla-jackbridge
    carla-lilv
    carla-native-plugins
    carla-rtmempool
    carla-sfzero
    carla-water
    $<$<BOOL:${CARLA_ENABLE_JSFX}>:carla-ysfx>
    carla-zita-resampler
  PUBLIC
    PkgConfig::FLUIDSYNTH
    PkgConfig::LIBLO
    PkgConfig::LIBMAGIC
    PkgConfig::X11
    $<$<BOOL:${APPLE}>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>
    ${CARLA_PTHREADS}
)

target_sources(carla-standalone
  PRIVATE
    ../source/backend/CarlaStandalone.cpp
    ../source/backend/CarlaStandaloneNSM.cpp
    ../source/backend/engine/CarlaEngine.cpp
    ../source/backend/engine/CarlaEngineClient.cpp
    ../source/backend/engine/CarlaEngineDummy.cpp
    ../source/backend/engine/CarlaEngineData.cpp
    ../source/backend/engine/CarlaEngineGraph.cpp
    ../source/backend/engine/CarlaEngineInternal.cpp
    ../source/backend/engine/CarlaEngineNative.cpp
    ../source/backend/engine/CarlaEngineOsc.cpp
    ../source/backend/engine/CarlaEngineOscHandlers.cpp
    ../source/backend/engine/CarlaEngineOscSend.cpp
    ../source/backend/engine/CarlaEnginePorts.cpp
    ../source/backend/engine/CarlaEngineRunner.cpp
    ../source/backend/plugin/CarlaPlugin.cpp
    ../source/backend/plugin/CarlaPluginBridge.cpp
    ../source/backend/plugin/CarlaPluginInternal.cpp
    ../source/backend/plugin/CarlaPluginAU.cpp
    ../source/backend/plugin/CarlaPluginCLAP.cpp
    ../source/backend/plugin/CarlaPluginFluidSynth.cpp
    ../source/backend/plugin/CarlaPluginJSFX.cpp
    ../source/backend/plugin/CarlaPluginLADSPADSSI.cpp
    ../source/backend/plugin/CarlaPluginLV2.cpp
    ../source/backend/plugin/CarlaPluginNative.cpp
    ../source/backend/plugin/CarlaPluginSFZero.cpp
    ../source/backend/plugin/CarlaPluginVST2.cpp
    ../source/backend/plugin/CarlaPluginVST3.cpp
    $<$<BOOL:${CARLA_USE_JACK}>:../source/backend/engine/CarlaEngineJack.cpp>
    $<$<BOOL:${CARLA_USE_JACK}>:../source/backend/plugin/CarlaPluginJack.cpp>
  PUBLIC
    ${carla-standalone-public-headers}
)

install(TARGETS carla-standalone
  ARCHIVE DESTINATION ${CARLA_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CARLA_INSTALL_LIBDIR}
  FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
)

if(${CARLA_BUILD_FRAMEWORKS})
  install(TARGETS
    carla-bridge-lv2-gtk2
    carla-bridge-lv2-gtk3
    carla-bridge-native
    carla-discovery-native
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-standalone.framework/Versions/A)

  install(FILES
    ../source/includes/CarlaDefines.h
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-standalone.framework/Versions/A/Headers/includes)

  install(FILES
    ../source/utils/CarlaUtils.hpp
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-standalone.framework/Versions/A/Headers/utils)

  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-bridge-lv2-gtk2
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-standalone.framework/carla-bridge-lv2-gtk2)")
  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-bridge-lv2-gtk3
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-standalone.framework/carla-bridge-lv2-gtk3)")
  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-bridge-native
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-standalone.framework/carla-bridge-native)")
  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-discovery-native
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-standalone.framework/carla-discovery-native)")
endif()

#######################################################################################################################
# carla utils

add_library(carla-utils ${CARLA_LIB_TYPE})
add_library(carla::utils ALIAS carla-utils)

set_common_target_properties(carla-utils)

if(${CARLA_BUILD_FRAMEWORKS})
  set(carla-utils-public-headers
    ../source/backend/CarlaBackend.h
    ../source/backend/CarlaUtils.h
  )

  set_target_properties(carla-utils
    PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      OUTPUT_NAME carla-utils
      PUBLIC_HEADER "${carla-utils-public-headers}"
      MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-framework.plist.in"
      MACOSX_FRAMEWORK_IDENTIFIER "studio.kx.carla.utils"
      MACOSX_FRAMEWORK_BUNDLE_VERSION "${CARLA_DATE}"
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${PROJECT_SHORT_VERSION}"
  )
else()
  set_target_properties(carla-utils
    PROPERTIES
      OUTPUT_NAME carla_utils
      IMPORT_PREFIX lib
      PREFIX lib
  )
endif()

target_include_directories(carla-utils
  PRIVATE
    ../source
    ../source/modules
  PUBLIC
    ../source/backend
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-utils
  PRIVATE
    carla-jackbridge
    carla-lilv
    carla-water-files
    $<$<BOOL:${CARLA_ENABLE_JSFX}>:carla-ysfx>
  PUBLIC
    PkgConfig::FLUIDSYNTH
    PkgConfig::LIBMAGIC
    PkgConfig::X11
    ${CARLA_PTHREADS}
    $<$<BOOL:${APPLE}>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>
    $<$<BOOL:${WIN32}>:winmm>
)

target_sources(carla-utils
  PRIVATE
    ../source/backend/utils/CachedPlugins.cpp
    ../source/backend/utils/CarlaUtils.cpp
    ../source/backend/utils/Information.cpp
    ../source/backend/utils/PipeClient.cpp
    ../source/backend/utils/PluginDiscovery.cpp
    ../source/backend/utils/System.cpp
    ../source/backend/utils/Windows.cpp
  PUBLIC
    ${carla-utils-public-headers}
)

install(TARGETS carla-utils
  ARCHIVE DESTINATION ${CARLA_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CARLA_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CARLA_INSTALL_BINDIR}
  FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(${CARLA_BUILD_FRAMEWORKS})
  install(TARGETS
    carla-bridge-lv2-gtk2
    carla-bridge-lv2-gtk3
    carla-bridge-native
    carla-discovery-native
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-utils.framework/Versions/A)

  install(FILES
    ../source/includes/CarlaDefines.h
    ../source/includes/CarlaMIDI.h
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-utils.framework/Versions/A/Headers/includes)

  install(FILES
    ../source/utils/CarlaUtils.hpp
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/carla-utils.framework/Versions/A/Headers/utils)

  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-bridge-lv2-gtk2
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-utils.framework/carla-bridge-lv2-gtk2)")
  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-bridge-lv2-gtk3
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-utils.framework/carla-bridge-lv2-gtk3)")
  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-bridge-native
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-utils.framework/carla-bridge-native)")
  install(CODE
    "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
      Versions/Current/carla-discovery-native
      $ENV{DESTDIR}${CMAKE_INSTALL_FULL_LIBDIR}/carla-utils.framework/carla-discovery-native)")
endif()

#######################################################################################################################

if(APPLE)
  set_source_files_properties(
    ../source/backend/CarlaStandalone.cpp
    ../source/backend/engine/CarlaEngineNative.cpp
    ../source/backend/plugin/CarlaPluginAU.cpp
    ../source/backend/plugin/CarlaPluginCLAP.cpp
    ../source/backend/plugin/CarlaPluginVST2.cpp
    ../source/backend/plugin/CarlaPluginVST3.cpp
    ../source/backend/utils/CarlaUtils.cpp
    ../source/backend/utils/Windows.cpp
    ../source/bridges-plugin/CarlaBridgePlugin.cpp
    ../source/bridges-ui/CarlaBridgeFormatLV2.cpp
    ../source/discovery/carla-discovery.cpp
    ../source/modules/water/water.cpp
    ../source/modules/water/water.files.cpp
    PROPERTIES COMPILE_FLAGS -ObjC++)
endif()

#######################################################################################################################
# install code headers

add_library(carla-headers-backend INTERFACE)
add_library(carla-headers-includes INTERFACE)
add_library(carla-headers-utils INTERFACE)

set_property(TARGET carla-headers-backend
  PROPERTY PUBLIC_HEADER
    ../source/backend/CarlaBackend.h
    ../source/backend/CarlaHost.h
    ../source/backend/CarlaUtils.h
    ../source/backend/CarlaEngine.hpp
    ../source/backend/CarlaPlugin.hpp
)

set_property(TARGET carla-headers-includes
  PROPERTY PUBLIC_HEADER
    ../source/includes/CarlaDefines.h
    ../source/includes/CarlaMIDI.h
    ../source/includes/CarlaNative.h
    ../source/includes/CarlaNativePlugin.h
)

set_property(TARGET carla-headers-utils
  PROPERTY PUBLIC_HEADER
    ../source/utils/CarlaUtils.hpp
)

install(TARGETS carla-headers-backend PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/carla)
install(TARGETS carla-headers-includes PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/carla/includes)
install(TARGETS carla-headers-utils PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/carla/utils)

#######################################################################################################################
# pkg-config stuff

if(NOT ${CARLA_BUILD_FRAMEWORKS} AND NOT MSVC)
  configure_file(carla-utils.pc.in carla-utils.pc @ONLY)
  configure_file(carla-standalone.pc.in carla-standalone.pc @ONLY)
  configure_file(carla-native-plugin.pc.in carla-native-plugin.pc @ONLY)
  configure_file(carla-host-plugin.pc.in carla-host-plugin.pc @ONLY)

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/carla-utils.pc
    ${CMAKE_CURRENT_BINARY_DIR}/carla-standalone.pc
    ${CMAKE_CURRENT_BINARY_DIR}/carla-native-plugin.pc
    ${CMAKE_CURRENT_BINARY_DIR}/carla-host-plugin.pc
    DESTINATION
      ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
endif()

#######################################################################################################################
