cmake_minimum_required(VERSION 3.0)
project(carla)

set(CMAKE_CXX_STANDARD 11)

#######################################################################################################################
# check dependencies

find_package(PkgConfig)
find_package(Threads REQUIRED)

if(APPLE)
  find_library(APPKIT AppKit)
  set(CARLA_OBJCPP_EXT "mm")
else()
  set(CARLA_OBJCPP_EXT "cpp")
endif()

# set(carla_pthread_libs ${CMAKE_THREAD_LIBS_INIT})

# Optional: transient X11 window flags
if(PKGCONFIG_FOUND AND NOT (APPLE OR WIN32))
  pkg_check_modules(X11 "x11")
else()
  set(X11_FOUND FALSE)
endif()

if(NOT (APPLE OR HAIKU OR WIN32))
  set(CARLA_LIBDL dl)
  set(CARLA_LIBM m)
  set(CARLA_LIBRT rt)
endif()

#######################################################################################################################
# utilities

#######################################################################################################################
# jackbridge

add_library(carla-jackbridge STATIC)
add_library(carla::jackbridge ALIAS carla-jackbridge)

set_target_properties(carla-jackbridge
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

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

set_target_properties(carla-lilv_serd
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_options(carla-lilv_serd
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133 /wd4244 /wd4267 /wd4273>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-deprecated-declarations>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-implicit-fallthrough>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-incompatible-pointer-types-discards-qualifiers>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-unused-parameter>
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

set_target_properties(carla-lilv_sord
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_options(carla-lilv_sord
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133 /wd4244 /wd4267 /wd4273>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-deprecated-declarations>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-implicit-fallthrough>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-incompatible-pointer-types-discards-qualifiers>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-unused-parameter>
    # workaround compiler bug, see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109585
    $<$<NOT:$<BOOL:${MSVC}>>:-fno-strict-aliasing>
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

set_target_properties(carla-lilv_sratom
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_options(carla-lilv_sratom
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133 /wd4244 /wd4267 /wd4273>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-deprecated-declarations>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-implicit-fallthrough>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-incompatible-pointer-types-discards-qualifiers>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-unused-parameter>
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

set_target_properties(carla-lilv_lilv
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_options(carla-lilv_lilv
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4005 /wd4090 /wd4133 /wd4244 /wd4267 /wd4273>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-deprecated-declarations>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-implicit-fallthrough>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-incompatible-pointer-types-discards-qualifiers>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-unused-parameter>
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
# rtmempool

add_library(carla-rtmempool STATIC)
add_library(carla::rtmempool ALIAS carla-rtmempool)

set_target_properties(carla-rtmempool
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

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
# water

add_library(carla-water STATIC)
add_library(carla::water ALIAS carla-water)

set_target_properties(carla-water
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_options(carla-water
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4267>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-deprecated-copy>
)

target_include_directories(carla-water
  PRIVATE
    ../source/includes
    ../source/utils
)

#   find_library(APPKIT AppKit)
#       "uuid"
#       "wsock32"
#       "wininet"
#       "version"
#       "ole32"
#       "ws2_32"
#       "oleaut32"
#       "imm32"
#       "comdlg32"
#       "shlwapi"
#       "rpcrt4"
#       "winmm")
target_link_libraries(carla-water
  PRIVATE
    $<$<BOOL:${APPLE}>:${APPKIT}>
    $<$<BOOL:${WIN32}>:comdlg32 ole32 winmm>
    ${CARLA_LIBDL}
    ${CARLA_LIBRT}
)

target_sources(carla-water
  PRIVATE
    ../source/modules/water/water.${CARLA_OBJCPP_EXT}
)

#######################################################################################################################
# water-files

add_library(carla-water-files STATIC)
add_library(carla::water-files ALIAS carla-water-files)

set_target_properties(carla-water-files
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_options(carla-water-files
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4267>
    $<$<NOT:$<BOOL:${MSVC}>>:-Wno-deprecated-copy>
)

target_include_directories(carla-water-files
  PRIVATE
    ../source/includes
    ../source/utils
)

target_link_libraries(carla-water-files
  PRIVATE
    $<$<BOOL:${APPLE}>:${APPKIT}>
    $<$<BOOL:${WIN32}>:ole32 winmm>
    ${CARLA_LIBDL}
)

target_sources(carla-water-files
  PRIVATE
    ../source/modules/water/water.files.${CARLA_OBJCPP_EXT}
)

#######################################################################################################################
# carla bridge

add_executable(carla-bridge-native)

set_target_properties(carla-bridge-native
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(carla-bridge-native
  PRIVATE
    BUILDING_CARLA
    BUILD_BRIDGE
    BUILD_BRIDGE_ALTERNATIVE_ARCH
    CARLA_LIB_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}"
    $<$<BOOL:${X11_FOUND}>:HAVE_X11>
)

target_compile_options(carla-bridge-native
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4244 /wd4267 /wd4273>
    ${X11_CFLAGS}
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
    ${X11_INCLUDE_DIRS}
)

target_link_libraries(carla-bridge-native
  PRIVATE
    carla-jackbridge
    carla-lilv
    carla-rtmempool
    carla-water
    ${X11_LIBRARIES}
)

target_sources(carla-bridge-native
  PRIVATE
    ../source/bridges-plugin/CarlaBridgePlugin.${CARLA_OBJCPP_EXT}
    ../source/backend/CarlaStandalone.${CARLA_OBJCPP_EXT}
    ../source/backend/engine/CarlaEngine.cpp
    ../source/backend/engine/CarlaEngineBridge.cpp
    ../source/backend/engine/CarlaEngineClient.cpp
    ../source/backend/engine/CarlaEngineData.cpp
    ../source/backend/engine/CarlaEngineInternal.cpp
    ../source/backend/engine/CarlaEnginePorts.cpp
    ../source/backend/engine/CarlaEngineRunner.cpp
    ../source/backend/plugin/CarlaPlugin.cpp
    ../source/backend/plugin/CarlaPluginBridge.cpp
    ../source/backend/plugin/CarlaPluginJuce.cpp
    ../source/backend/plugin/CarlaPluginInternal.cpp
    ../source/backend/plugin/CarlaPluginAU.cpp
    ../source/backend/plugin/CarlaPluginCLAP.${CARLA_OBJCPP_EXT}
    ../source/backend/plugin/CarlaPluginLADSPADSSI.cpp
    ../source/backend/plugin/CarlaPluginLV2.cpp
    ../source/backend/plugin/CarlaPluginVST2.${CARLA_OBJCPP_EXT}
    ../source/backend/plugin/CarlaPluginVST3.${CARLA_OBJCPP_EXT}
)
                                                  
#######################################################################################################################
# carla discovery

add_executable(carla-discovery-native)

set_target_properties(carla-discovery-native
  PROPERTIES
    OSX_ARCHITECTURES "x86_64;arm64"
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(carla-discovery-native
  PRIVATE
    BUILDING_CARLA
)

target_compile_options(carla-discovery-native
  PRIVATE
    $<$<BOOL:${MSVC}>:/wd4244 /wd4267 /wd4273>
)

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
)

target_sources(carla-discovery-native
  PRIVATE
    ../source/discovery/carla-discovery.${CARLA_OBJCPP_EXT}
)

#######################################################################################################################
# carla utils

add_library(carla-utils SHARED)
add_library(carla::utils ALIAS carla-utils)

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
    carla-water
)

target_sources(carla-utils
  PRIVATE
    ../source/backend/utils/CachedPlugins.cpp
    ../source/backend/utils/Information.cpp
    ../source/backend/utils/JUCE.cpp
    ../source/backend/utils/PipeClient.cpp
    ../source/backend/utils/PluginDiscovery.cpp
    ../source/backend/utils/System.cpp
    #../source/backend/utils/Windows.cpp
)

#######################################################################################################################
