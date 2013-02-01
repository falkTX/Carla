#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the GPL.txt file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from ctypes import *
from subprocess import Popen, PIPE

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_shared import *

try:
    import ladspa_rdf
    haveLRDF = True
except:
    print("LRDF Support not available (LADSPA-RDF will be disabled)")
    haveLRDF = False

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes struct into a python dict

def structToDict(struct):
    return dict((attr, getattr(struct, attr)) for attr, value in struct._fields_)

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders

if WINDOWS:
    splitter = ";"
    APPDATA = os.getenv("APPDATA")
    PROGRAMFILES = os.getenv("PROGRAMFILES")
    PROGRAMFILESx86 = os.getenv("PROGRAMFILES(x86)")
    COMMONPROGRAMFILES = os.getenv("COMMONPROGRAMFILES")

    # Small integrity tests
    if not APPDATA:
        print("APPDATA variable not set, cannot continue")
        sys.exit(1)

    if not PROGRAMFILES:
        print("PROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    if not COMMONPROGRAMFILES:
        print("COMMONPROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    DEFAULT_LADSPA_PATH = [
        os.path.join(APPDATA, "LADSPA"),
        os.path.join(PROGRAMFILES, "LADSPA")
    ]

    DEFAULT_DSSI_PATH = [
        os.path.join(APPDATA, "DSSI"),
        os.path.join(PROGRAMFILES, "DSSI")
    ]

    DEFAULT_LV2_PATH = [
        os.path.join(APPDATA, "LV2"),
        os.path.join(COMMONPROGRAMFILES, "LV2")
    ]

    DEFAULT_VST_PATH = [
        os.path.join(PROGRAMFILES, "VstPlugins"),
        os.path.join(PROGRAMFILES, "Steinberg", "VstPlugins")
    ]

    DEFAULT_GIG_PATH = [
        os.path.join(APPDATA, "GIG")
    ]

    DEFAULT_SF2_PATH = [
        os.path.join(APPDATA, "SF2")
    ]

    DEFAULT_SFZ_PATH = [
        os.path.join(APPDATA, "SFZ")
    ]

    if PROGRAMFILESx86:
        DEFAULT_LADSPA_PATH.append(os.path.join(PROGRAMFILESx86, "LADSPA"))
        DEFAULT_DSSI_PATH.append(os.path.join(PROGRAMFILESx86, "DSSI"))
        DEFAULT_VST_PATH.append(os.path.join(PROGRAMFILESx86, "VstPlugins"))
        DEFAULT_VST_PATH.append(os.path.join(PROGRAMFILESx86, "Steinberg", "VstPlugins"))

elif HAIKU:
    splitter = ":"

    DEFAULT_LADSPA_PATH = [
        # TODO
    ]

    DEFAULT_DSSI_PATH = [
        # TODO
    ]

    DEFAULT_LV2_PATH = [
        # TODO
    ]

    DEFAULT_VST_PATH = [
        # TODO
    ]

    DEFAULT_GIG_PATH = [
        # TODO
    ]

    DEFAULT_SF2_PATH = [
        # TODO
    ]

    DEFAULT_SFZ_PATH = [
        # TODO
    ]

elif MACOS:
    splitter = ":"

    DEFAULT_LADSPA_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LADSPA"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "LADSPA")
    ]

    DEFAULT_DSSI_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "DSSI"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "DSSI")
    ]

    DEFAULT_LV2_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LV2"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "LV2")
    ]

    DEFAULT_VST_PATH = [
        os.path.join(HOME, "Library", "Audio", "Plug-Ins", "VST"),
        os.path.join("/", "Library", "Audio", "Plug-Ins", "VST")
    ]

    DEFAULT_GIG_PATH = [
        # TODO
    ]

    DEFAULT_SF2_PATH = [
        # TODO
    ]

    DEFAULT_SFZ_PATH = [
        # TODO
    ]

else:
    splitter = ":"

    DEFAULT_LADSPA_PATH = [
        os.path.join(HOME, ".ladspa"),
        os.path.join("/", "usr", "lib", "ladspa"),
        os.path.join("/", "usr", "local", "lib", "ladspa")
    ]

    DEFAULT_DSSI_PATH = [
        os.path.join(HOME, ".dssi"),
        os.path.join("/", "usr", "lib", "dssi"),
        os.path.join("/", "usr", "local", "lib", "dssi")
    ]

    DEFAULT_LV2_PATH = [
        os.path.join(HOME, ".lv2"),
        os.path.join("/", "usr", "lib", "lv2"),
        os.path.join("/", "usr", "local", "lib", "lv2")
    ]

    DEFAULT_VST_PATH = [
        os.path.join(HOME, ".vst"),
        os.path.join("/", "usr", "lib", "vst"),
        os.path.join("/", "usr", "local", "lib", "vst")
    ]

    DEFAULT_GIG_PATH = [
        os.path.join(HOME, ".sounds"),
        os.path.join("/", "usr", "share", "sounds", "gig")
    ]

    DEFAULT_SF2_PATH = [
        os.path.join(HOME, ".sounds"),
        os.path.join("/", "usr", "share", "sounds", "sf2")
    ]

    DEFAULT_SFZ_PATH = [
        os.path.join(HOME, ".sounds"),
        os.path.join("/", "usr", "share", "sounds", "sfz")
    ]

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (set)

global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, GIG_PATH, SF2_PATH, SFZ_PATH

LADSPA_PATH = os.getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH)
DSSI_PATH = os.getenv("DSSI_PATH", DEFAULT_DSSI_PATH)
LV2_PATH = os.getenv("LV2_PATH", DEFAULT_LV2_PATH)
VST_PATH = os.getenv("VST_PATH", DEFAULT_VST_PATH)
GIG_PATH = os.getenv("GIG_PATH", DEFAULT_GIG_PATH)
SF2_PATH = os.getenv("SF2_PATH", DEFAULT_SF2_PATH)
SFZ_PATH = os.getenv("SFZ_PATH", DEFAULT_SFZ_PATH)

if haveLRDF:
    LADSPA_RDF_PATH_env = os.getenv("LADSPA_RDF_PATH")
    if LADSPA_RDF_PATH_env:
        ladspa_rdf.set_rdf_path(LADSPA_RDF_PATH_env.split(splitter))
    del LADSPA_RDF_PATH_env

# ------------------------------------------------------------------------------------------------------------
# Search for Carla library and tools

global carla_library_path
carla_library_path = ""

carla_discovery_native  = ""
carla_discovery_posix32 = ""
carla_discovery_posix64 = ""
carla_discovery_win32   = ""
carla_discovery_win64   = ""

carla_bridge_native  = ""
carla_bridge_posix32 = ""
carla_bridge_posix64 = ""
carla_bridge_win32   = ""
carla_bridge_win64   = ""

carla_bridge_lv2_gtk2    = ""
carla_bridge_lv2_gtk3    = ""
carla_bridge_lv2_qt4     = ""
carla_bridge_lv2_qt5     = ""
carla_bridge_lv2_cocoa   = ""
carla_bridge_lv2_windows = ""
carla_bridge_lv2_x11     = ""

carla_bridge_vst_cocoa = ""
carla_bridge_vst_hwnd  = ""
carla_bridge_vst_x11   = ""

if WINDOWS:
    carla_libname = "libcarla_standalone.dll"
elif MACOS:
    carla_libname = "libcarla_standalone.dylib"
else:
    carla_libname = "libcarla_standalone.so"

CWD = sys.path[0]

# make it work with cxfreeze
if CWD.endswith("%scarla" % os.sep):
    CWD = CWD.rsplit("%scarla" % os.sep, 1)[0]

# find carla_library_path
if os.path.exists(os.path.join(CWD, "backend", carla_libname)):
    carla_library_path = os.path.join(CWD, "backend", carla_libname)
else:
    if WINDOWS:
        CARLA_PATH = (os.path.join(PROGRAMFILES, "Carla"),)
    elif MACOS:
        CARLA_PATH = ("/opt/local/lib", "/usr/local/lib/", "/usr/lib")
    else:
        CARLA_PATH = ("/usr/local/lib/", "/usr/lib")

    for path in CARLA_PATH:
        if os.path.exists(os.path.join(path, "carla", carla_libname)):
            carla_library_path = os.path.join(path, "carla", carla_libname)
            break

# find any tool
def findTool(tdir, tname):
    if os.path.exists(os.path.join(CWD, tdir, tname)):
        return os.path.join(CWD, tdir, tname)

    for p in PATH:
        if os.path.exists(os.path.join(p, tname)):
            return os.path.join(p, tname)

    return ""

# find wine/windows tools
carla_discovery_win32 = findTool("carla-discovery", "carla-discovery-win32.exe")
carla_discovery_win64 = findTool("carla-discovery", "carla-discovery-win64.exe")
carla_bridge_win32    = findTool("carla-bridge", "carla-bridge-win32.exe")
carla_bridge_win64    = findTool("carla-bridge", "carla-bridge-win64.exe")

# find native and posix tools
if not WINDOWS:
    carla_discovery_native  = findTool("carla-discovery", "carla-discovery-native")
    carla_discovery_posix32 = findTool("carla-discovery", "carla-discovery-posix32")
    carla_discovery_posix64 = findTool("carla-discovery", "carla-discovery-posix64")
    carla_bridge_native     = findTool("carla-bridge", "carla-bridge-native")
    carla_bridge_posix32    = findTool("carla-bridge", "carla-bridge-posix32")
    carla_bridge_posix64    = findTool("carla-bridge", "carla-bridge-posix64")

# find windows only tools
if WINDOWS:
    carla_bridge_lv2_windows = findTool("carla-bridge", "carla-bridge-lv2-windows.exe")
    carla_bridge_vst_hwnd    = findTool("carla-bridge", "carla-bridge-vst-hwnd.exe")

# find mac os only tools
elif MACOS:
    carla_bridge_lv2_cocoa = findTool("carla-bridge", "carla-bridge-lv2-cocoa")
    carla_bridge_vst_cocoa = findTool("carla-bridge", "carla-bridge-vst-cocoa")

# find generic tools
else:
    carla_bridge_lv2_gtk2 = findTool("carla-bridge", "carla-bridge-lv2-gtk2")
    carla_bridge_lv2_gtk3 = findTool("carla-bridge", "carla-bridge-lv2-gtk3")
    carla_bridge_lv2_qt4  = findTool("carla-bridge", "carla-bridge-lv2-qt4")
    carla_bridge_lv2_qt5  = findTool("carla-bridge", "carla-bridge-lv2-qt5")

# find linux only tools
if LINUX:
    carla_bridge_lv2_x11 = os.path.join("carla-bridge", "carla-bridge-lv2-x11")
    carla_bridge_vst_x11 = os.path.join("carla-bridge", "carla-bridge-vst-x11")

# ------------------------------------------------------------------------------------------------------------
# Plugin Query (helper functions)

def findBinaries(bPATH, OS):
    binaries = []

    if OS == "WINDOWS":
        extensions = (".dll",)
    elif OS == "MACOS":
        extensions = (".dylib", ".so")
    else:
        extensions = (".so", ".sO", ".SO", ".So")

    for root, dirs, files in os.walk(bPATH):
        for name in [name for name in files if name.endswith(extensions)]:
            binaries.append(os.path.join(root, name))

    return binaries

# FIXME - may use any extension, just needs to have manifest.ttl
def findLV2Bundles(bPATH):
    bundles = []
    extensions = (".lv2", ".lV2", ".LV2", ".Lv2") if not WINDOWS else (".lv2",)

    for root, dirs, files in os.walk(bPATH):
        for dir_ in [dir_ for dir_ in dirs if dir_.endswith(extensions)]:
            bundles.append(os.path.join(root, dir_))

    return bundles

def findSoundKits(bPATH, stype):
    soundfonts = []

    if stype == "gig":
        extensions = (".gig", ".giG", ".gIG", ".GIG", ".GIg", ".Gig") if not WINDOWS else (".gig",)
    elif stype == "sf2":
        extensions = (".sf2", ".sF2", ".SF2", ".Sf2") if not WINDOWS else (".sf2",)
    elif stype == "sfz":
        extensions = (".sfz", ".sfZ", ".sFZ", ".SFZ", ".SFz", ".Sfz") if not WINDOWS else (".sfz",)
    else:
        return []

    for root, dirs, files in os.walk(bPATH):
        for name in [name for name in files if name.endswith(extensions)]:
            soundfonts.append(os.path.join(root, name))

    return soundfonts

def findDSSIGUI(filename, name, label):
    pluginDir = filename.rsplit(".", 1)[0]
    shortName = os.path.basename(pluginDir)
    guiFilename = ""

    checkName  = name.replace(" ", "_")
    checkLabel = label
    checkSName = shortName

    if checkName[-1]  != "_": checkName  += "_"
    if checkLabel[-1] != "_": checkLabel += "_"
    if checkSName[-1] != "_": checkSName += "_"

    for root, dirs, files in os.walk(pluginDir):
        guiFiles = files
        break
    else:
        guiFiles = []

    for gui in guiFiles:
        if gui.startswith(checkName) or gui.startswith(checkLabel) or gui.startswith(checkSName):
            guiFilename = os.path.join(pluginDir, gui)
            break

    return guiFilename

# ------------------------------------------------------------------------------------------------------------
# Plugin Query

PLUGIN_QUERY_API_VERSION = 1

PyPluginInfo = {
    'API': PLUGIN_QUERY_API_VERSION,
    'build': 0, # BINARY_NONE
    'type': 0, # PLUGIN_NONE
    'hints': 0x0,
    'binary': "",
    'name': "",
    'label': "",
    'maker': "",
    'copyright': "",
    'uniqueId': 0,
    'audio.ins': 0,
    'audio.outs': 0,
    'audio.totals': 0,
    'midi.ins': 0,
    'midi.outs': 0,
    'midi.totals': 0,
    'parameters.ins': 0,
    'parameters.outs': 0,
    'parameters.total': 0,
    'programs.total': 0
}

def runCarlaDiscovery(itype, stype, filename, tool, isWine=False):
    fakeLabel = os.path.basename(filename).rsplit(".", 1)[0]
    plugins = []
    command = []

    if LINUX or MACOS:
        command.append("env")
        command.append("LANG=C")
        if isWine:
            command.append("WINEDEBUG=-all")

    command.append(tool)
    command.append(stype)
    command.append(filename)

    Ps = Popen(command, stdout=PIPE)
    Ps.wait()
    output = Ps.stdout.read().decode("utf-8", errors="ignore").split("\n")

    pinfo = None

    for line in output:
        line = line.strip()
        if line == "carla-discovery::init::-----------":
            pinfo = deepcopy(PyPluginInfo)
            pinfo['type'] = itype
            pinfo['binary'] = filename

        elif line == "carla-discovery::end::------------":
            if pinfo != None:
                plugins.append(pinfo)
                pinfo = None

        elif line == "Segmentation fault":
            print("carla-discovery::crash::%s crashed during discovery" % filename)

        elif line.startswith("err:module:import_dll Library"):
            print(line)

        elif line.startswith("carla-discovery::error::"):
            print("%s - %s" % (line, filename))

        elif line.startswith("carla-discovery::"):
            if pinfo == None:
                continue

            prop, value = line.replace("carla-discovery::", "").split("::", 1)

            if prop == "name":
                pinfo['name'] = value if value else fakeLabel
            elif prop == "label":
                pinfo['label'] = value if value else fakeLabel
            elif prop == "maker":
                pinfo['maker'] = value
            elif prop == "copyright":
                pinfo['copyright'] = value
            elif prop == "uniqueId":
                if value.isdigit(): pinfo['uniqueId'] = int(value)
            elif prop == "hints":
                if value.isdigit(): pinfo['hints'] = int(value)
            elif prop == "audio.ins":
                if value.isdigit(): pinfo['audio.ins'] = int(value)
            elif prop == "audio.outs":
                if value.isdigit(): pinfo['audio.outs'] = int(value)
            elif prop == "audio.total":
                if value.isdigit(): pinfo['audio.total'] = int(value)
            elif prop == "midi.ins":
                if value.isdigit(): pinfo['midi.ins'] = int(value)
            elif prop == "midi.outs":
                if value.isdigit(): pinfo['midi.outs'] = int(value)
            elif prop == "midi.total":
                if value.isdigit(): pinfo['midi.total'] = int(value)
            elif prop == "parameters.ins":
                if value.isdigit(): pinfo['parameters.ins'] = int(value)
            elif prop == "parameters.outs":
                if value.isdigit(): pinfo['parameters.outs'] = int(value)
            elif prop == "parameters.total":
                if value.isdigit(): pinfo['parameters.total'] = int(value)
            elif prop == "programs.total":
                if value.isdigit(): pinfo['programs.total'] = int(value)
            elif prop == "build":
                if value.isdigit(): pinfo['build'] = int(value)

    # Additional checks
    for pinfo in plugins:
        if itype == PLUGIN_DSSI:
            if findDSSIGUI(pinfo['binary'], pinfo['name'], pinfo['label']):
                pinfo['hints'] |= PLUGIN_HAS_GUI

    return plugins

def checkPluginInternal(desc):
    plugins = []

    pinfo = deepcopy(PyPluginInfo)
    pinfo['type']  = PLUGIN_INTERNAL
    pinfo['name']  = cString(desc['name'])
    pinfo['label'] = cString(desc['label'])
    pinfo['maker'] = cString(desc['maker'])
    pinfo['copyright'] = cString(desc['copyright'])
    pinfo['hints'] = int(desc['hints'])
    pinfo['build'] = BINARY_NATIVE

    plugins.append(pinfo)

    return plugins

def checkPluginLADSPA(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LADSPA, "LADSPA", filename, tool, isWine)

def checkPluginDSSI(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_DSSI, "DSSI", filename, tool, isWine)

def checkPluginLV2(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LV2, "LV2", filename, tool, isWine)

def checkPluginVST(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_VST, "VST", filename, tool, isWine)

def checkPluginGIG(filename, tool):
    return runCarlaDiscovery(PLUGIN_GIG, "GIG", filename, tool)

def checkPluginSF2(filename, tool):
    return runCarlaDiscovery(PLUGIN_SF2, "SF2", filename, tool)

def checkPluginSFZ(filename, tool):
    return runCarlaDiscovery(PLUGIN_SFZ, "SFZ", filename, tool)

# ------------------------------------------------------------------------------------------------------------
# Backend C++ -> Python variables

c_enum = c_int
c_nullptr = None

#if kIs64bit:
    #c_uintptr = c_uint64
#else:
    #c_uintptr = c_uint32

CallbackFunc = CFUNCTYPE(None, c_void_p, c_enum, c_int, c_int, c_int, c_double, c_char_p)

class ParameterData(Structure):
    _fields_ = [
        ("type", c_enum),
        ("index", c_int32),
        ("rindex", c_int32),
        ("hints", c_int32),
        ("midiChannel", c_uint8),
        ("midiCC", c_int16)
    ]

class ParameterRanges(Structure):
    _fields_ = [
        ("def", c_float),
        ("min", c_float),
        ("max", c_float),
        ("step", c_float),
        ("stepSmall", c_float),
        ("stepLarge", c_float)
    ]

class MidiProgramData(Structure):
    _fields_ = [
        ("bank", c_uint32),
        ("program", c_uint32),
        ("name", c_char_p)
    ]

class CustomData(Structure):
    _fields_ = [
        ("type", c_char_p),
        ("key", c_char_p),
        ("value", c_char_p)
    ]

# ------------------------------------------------------------------------------------------------------------
# Standalone C++ -> Python variables

class CarlaPluginInfo(Structure):
    _fields_ = [
        ("type", c_enum),
        ("category", c_enum),
        ("hints", c_uint),
        ("binary", c_char_p),
        ("name", c_char_p),
        ("label", c_char_p),
        ("maker", c_char_p),
        ("copyright", c_char_p),
        ("uniqueId", c_long)
    ]

class CarlaNativePluginInfo(Structure):
    _fields_ = [
        ("category", c_enum),
        ("hints", c_uint),
        ("audioIns", c_uint32),
        ("audioOuts", c_uint32),
        ("midiIns", c_uint32),
        ("midiOuts", c_uint32),
        ("parameterIns", c_uint32),
        ("parameterOuts", c_uint32),
        ("binary", c_char_p),
        ("name", c_char_p),
        ("label", c_char_p),
        ("maker", c_char_p),
        ("copyright", c_char_p)
    ]

class CarlaPortCountInfo(Structure):
    _fields_ = [
        ("ins", c_uint32),
        ("outs", c_uint32),
        ("total", c_uint32)
    ]

class CarlaParameterInfo(Structure):
    _fields_ = [
        ("name", c_char_p),
        ("symbol", c_char_p),
        ("unit", c_char_p),
        ("scalePointCount", c_uint32)
    ]

class CarlaScalePointInfo(Structure):
    _fields_ = [
        ("value", c_float),
        ("label", c_char_p)
    ]

# ------------------------------------------------------------------------------------------------------------
# Standalone Python object

class Host(object):
    def __init__(self, lib_prefix_arg):
        object.__init__(self)

        global carla_library_path

        if lib_prefix_arg:
            carla_library_path = os.path.join(lib_prefix_arg, "lib", "carla", carla_libname)

        if not carla_library_path:
            self.lib = None
            return

        self.lib = cdll.LoadLibrary(carla_library_path)

        self.lib.carla_get_extended_license_text.argtypes = None
        self.lib.carla_get_extended_license_text.restype = c_char_p

        self.lib.carla_get_engine_driver_count.argtypes = None
        self.lib.carla_get_engine_driver_count.restype = c_uint

        self.lib.carla_get_engine_driver_name.argtypes = [c_uint]
        self.lib.carla_get_engine_driver_name.restype = c_char_p

        self.lib.carla_get_internal_plugin_count.argtypes = None
        self.lib.carla_get_internal_plugin_count.restype = c_uint

        self.lib.carla_get_internal_plugin_info.argtypes = [c_uint]
        self.lib.carla_get_internal_plugin_info.restype = POINTER(CarlaNativePluginInfo)

        self.lib.carla_engine_init.argtypes = [c_char_p, c_char_p]
        self.lib.carla_engine_init.restype = c_bool

        self.lib.carla_engine_close.argtypes = None
        self.lib.carla_engine_close.restype = c_bool

        self.lib.carla_is_engine_running.argtypes = None
        self.lib.carla_is_engine_running.restype = c_bool

        self.lib.carla_add_plugin.argtypes = [c_enum, c_enum, c_char_p, c_char_p, c_char_p, c_void_p]
        self.lib.carla_add_plugin.restype = c_bool

        self.lib.carla_remove_plugin.argtypes = [c_uint]
        self.lib.carla_remove_plugin.restype = c_bool

        self.lib.carla_get_plugin_info.argtypes = [c_uint]
        self.lib.carla_get_plugin_info.restype = POINTER(CarlaPluginInfo)

        self.lib.carla_get_audio_port_count_info.argtypes = [c_uint]
        self.lib.carla_get_audio_port_count_info.restype = POINTER(CarlaPortCountInfo)

        self.lib.carla_get_midi_port_count_info.argtypes = [c_uint]
        self.lib.carla_get_midi_port_count_info.restype = POINTER(CarlaPortCountInfo)

        self.lib.carla_get_parameter_count_info.argtypes = [c_uint]
        self.lib.carla_get_parameter_count_info.restype = POINTER(CarlaPortCountInfo)

        self.lib.carla_get_parameter_info.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_parameter_info.restype = POINTER(CarlaParameterInfo)

        self.lib.carla_get_parameter_scalepoint_info.argtypes = [c_uint, c_uint32, c_uint32]
        self.lib.carla_get_parameter_scalepoint_info.restype = POINTER(CarlaScalePointInfo)

        self.lib.carla_get_parameter_data.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_parameter_data.restype = POINTER(ParameterData)

        self.lib.carla_get_parameter_ranges.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_parameter_ranges.restype = POINTER(ParameterRanges)

        self.lib.carla_get_midi_program_data.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_midi_program_data.restype = POINTER(MidiProgramData)

        self.lib.carla_get_custom_data.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_custom_data.restype = POINTER(CustomData)

        self.lib.carla_get_chunk_data.argtypes = [c_uint]
        self.lib.carla_get_chunk_data.restype = c_char_p

        self.lib.carla_get_parameter_count.argtypes = [c_uint]
        self.lib.carla_get_parameter_count.restype = c_uint32

        self.lib.carla_get_program_count.argtypes = [c_uint]
        self.lib.carla_get_program_count.restype = c_uint32

        self.lib.carla_get_midi_program_count.argtypes = [c_uint]
        self.lib.carla_get_midi_program_count.restype = c_uint32

        self.lib.carla_get_custom_data_count.argtypes = [c_uint]
        self.lib.carla_get_custom_data_count.restype = c_uint32

        #self.lib.carla_get_parameter_text.argtypes = [c_uint, c_uint32]
        #self.lib.carla_get_parameter_text.restype = c_char_p

        #self.lib.carla_get_program_name.argtypes = [c_uint, c_uint32]
        #self.lib.carla_get_program_name.restype = c_char_p

        #self.lib.carla_get_midi_program_name.argtypes = [c_uint, c_uint32]
        #self.lib.carla_get_midi_program_name.restype = c_char_p

        #self.lib.carla_get_real_plugin_name.argtypes = [c_uint]
        #self.lib.carla_get_real_plugin_name.restype = c_char_p

        #self.lib.carla_get_current_program_index.argtypes = [c_uint]
        #self.lib.carla_get_current_program_index.restype = c_int32

        #self.lib.carla_get_current_midi_program_index.argtypes = [c_uint]
        #self.lib.carla_get_current_midi_program_index.restype = c_int32

        #self.lib.carla_get_default_parameter_value.argtypes = [c_uint, c_uint32]
        #self.lib.carla_get_default_parameter_value.restype = c_double

        #self.lib.carla_get_current_parameter_value.argtypes = [c_uint, c_uint32]
        #self.lib.carla_get_current_parameter_value.restype = c_double

        #self.lib.carla_get_input_peak_value.argtypes = [c_uint, c_ushort]
        #self.lib.carla_get_input_peak_value.restype = c_double

        #self.lib.carla_get_output_peak_value.argtypes = [c_uint, c_ushort]
        #self.lib.carla_get_output_peak_value.restype = c_double

        #self.lib.carla_set_active.argtypes = [c_uint, c_bool]
        #self.lib.carla_set_active.restype = None

        #self.lib.carla_set_drywet.argtypes = [c_uint, c_double]
        #self.lib.carla_set_drywet.restype = None

        #self.lib.carla_set_volume.argtypes = [c_uint, c_double]
        #self.lib.carla_set_volume.restype = None

        #self.lib.carla_set_balance_left.argtypes = [c_uint, c_double]
        #self.lib.carla_set_balance_left.restype = None

        #self.lib.carla_set_balance_right.argtypes = [c_uint, c_double]
        #self.lib.carla_set_balance_right.restype = None

        #self.lib.carla_set_parameter_value.argtypes = [c_uint, c_uint32, c_double]
        #self.lib.carla_set_parameter_value.restype = None

        #self.lib.carla_set_parameter_midi_cc.argtypes = [c_uint, c_uint32, c_int16]
        #self.lib.carla_set_parameter_midi_cc.restype = None

        #self.lib.carla_set_parameter_midi_channel.argtypes = [c_uint, c_uint32, c_uint8]
        #self.lib.carla_set_parameter_midi_channel.restype = None

        #self.lib.carla_set_program.argtypes = [c_uint, c_uint32]
        #self.lib.carla_set_program.restype = None

        #self.lib.carla_set_midi_program.argtypes = [c_uint, c_uint32]
        #self.lib.carla_set_midi_program.restype = None

        #self.lib.carla_set_custom_data.argtypes = [c_uint, c_char_p, c_char_p, c_char_p]
        #self.lib.carla_set_custom_data.restype = None

        #self.lib.carla_set_chunk_data.argtypes = [c_uint, c_char_p]
        #self.lib.carla_set_chunk_data.restype = None

        #self.lib.carla_set_gui_container.argtypes = [c_uint, c_uintptr]
        #self.lib.carla_set_gui_container.restype = None

        #self.lib.show_gui.argtypes = [c_uint, c_bool]
        #self.lib.show_gui.restype = None

        #self.lib.idle_guis.argtypes = None
        #self.lib.idle_guis.restype = None

        #self.lib.send_midi_note.argtypes = [c_uint, c_uint8, c_uint8, c_uint8]
        #self.lib.send_midi_note.restype = None

        #self.lib.prepare_for_save.argtypes = [c_uint]
        #self.lib.prepare_for_save.restype = None

        #self.lib.carla_get_buffer_size.argtypes = None
        #self.lib.carla_get_buffer_size.restype = c_uint32

        #self.lib.carla_get_sample_rate.argtypes = None
        #self.lib.carla_get_sample_rate.restype = c_double

        #self.lib.carla_get_last_error.argtypes = None
        #self.lib.carla_get_last_error.restype = c_char_p

        #self.lib.carla_get_host_osc_url.argtypes = None
        #self.lib.carla_get_host_osc_url.restype = c_char_p

        #self.lib.carla_set_callback_function.argtypes = [CallbackFunc]
        #self.lib.carla_set_callback_function.restype = None

        #self.lib.carla_set_option.argtypes = [c_enum, c_int, c_char_p]
        #self.lib.carla_set_option.restype = None

        #self.lib.nsm_announce.argtypes = [c_char_p, c_int]
        #self.lib.nsm_announce.restype = None

        #self.lib.nsm_reply_open.argtypes = None
        #self.lib.nsm_reply_open.restype = None

        #self.lib.nsm_reply_save.argtypes = None
        #self.lib.nsm_reply_save.restype = None

    def get_extended_license_text(self):
        return self.lib.carla_get_extended_license_text()

    def get_engine_driver_count(self):
        return self.lib.carla_get_engine_driver_count()

    def get_engine_driver_name(self, index):
        return self.lib.carla_get_engine_driver_name(index)

    def get_internal_plugin_count(self):
        return self.lib.carla_get_internal_plugin_count()

    def get_internal_plugin_info(self, internalPluginId):
        return structToDict(self.lib.carla_get_internal_plugin_info(internalPluginId).contents)

    def engine_init(self, driverName, clientName):
        return self.lib.carla_engine_init(driverName.encode("utf-8"), clientName.encode("utf-8"))

    def engine_close(self):
        return self.lib.carla_engine_close()

    def is_engine_running(self):
        return self.lib.carla_is_engine_running()

    def add_plugin(self, btype, ptype, filename, name, label, extraStuff):
        cname = name.encode("utf-8") if name else c_nullptr
        return self.lib.carla_add_plugin(btype, ptype, filename.encode("utf-8"), cname, label.encode("utf-8"), cast(extraStuff, c_void_p))

    def remove_plugin(self, pluginId):
        return self.lib.carla_remove_plugin(pluginId)

    def get_plugin_info(self, pluginId):
        return structToDict(self.lib.carla_get_plugin_info(pluginId).contents)

    def get_audio_port_count_info(self, pluginId):
        return structToDict(self.lib.carla_get_audio_port_count_info(pluginId).contents)

    def get_midi_port_count_info(self, pluginId):
        return structToDict(self.lib.carla_get_midi_port_count_info(pluginId).contents)

    def get_parameter_count_info(self, pluginId):
        return structToDict(self.lib.carla_get_parameter_count_info(pluginId).contents)

    def get_parameter_info(self, pluginId, parameterId):
        return structToDict(self.lib.carla_get_parameter_info(pluginId, parameterId).contents)

    def get_parameter_scalepoint_info(self, pluginId, parameterId, scalePointId):
        return structToDict(self.lib.carla_get_parameter_scalepoint_info(pluginId, parameterId, scalePointId).contents)

    def get_parameter_data(self, pluginId, parameterId):
        return structToDict(self.lib.carla_get_parameter_data(pluginId, parameterId).contents)

    def get_parameter_ranges(self, pluginId, parameterId):
        return structToDict(self.lib.carla_get_parameter_ranges(pluginId, parameterId).contents)

    def get_midi_program_data(self, pluginId, midiProgramId):
        return structToDict(self.lib.carla_get_midi_program_data(pluginId, midiProgramId).contents)

    #def get_custom_data(self, pluginId, customDataId):
        #return structToDict(self.lib.carla_get_custom_data(pluginId, customDataId).contents)

    #def get_chunk_data(self, pluginId):
        #return self.lib.carla_get_chunk_data(pluginId)

    #def get_gui_info(self, pluginId):
        #return structToDict(self.lib.carla_get_gui_info(pluginId).contents)

    #def get_parameter_count(self, pluginId):
        #return self.lib.carla_get_parameter_count(pluginId)

    #def get_program_count(self, pluginId):
        #return self.lib.carla_get_program_count(pluginId)

    #def get_midi_program_count(self, pluginId):
        #return self.lib.carla_get_midi_program_count(pluginId)

    #def get_custom_data_count(self, pluginId):
        #return self.lib.carla_get_custom_data_count(pluginId)

    #def get_parameter_text(self, pluginId, parameterId):
        #return self.lib.carla_get_parameter_text(pluginId, parameterId)

    #def get_program_name(self, pluginId, programId):
        #return self.lib.carla_get_program_name(pluginId, programId)

    #def get_midi_program_name(self, pluginId, midiProgramId):
        #return self.lib.carla_get_midi_program_name(pluginId, midiProgramId)

    #def get_real_plugin_name(self, pluginId):
        #return self.lib.carla_get_real_plugin_name(pluginId)

    #def get_current_program_index(self, pluginId):
        #return self.lib.carla_get_current_program_index(pluginId)

    #def get_current_midi_program_index(self, pluginId):
        #return self.lib.carla_get_current_midi_program_index(pluginId)

    #def get_default_parameter_value(self, pluginId, parameterId):
        #return self.lib.carla_get_default_parameter_value(pluginId, parameterId)

    #def get_current_parameter_value(self, pluginId, parameterId):
        #return self.lib.carla_get_current_parameter_value(pluginId, parameterId)

    #def get_input_peak_value(self, pluginId, portId):
        #return self.lib.carla_get_input_peak_value(pluginId, portId)

    #def get_output_peak_value(self, pluginId, portId):
        #return self.lib.carla_get_output_peak_value(pluginId, portId)

    #def set_active(self, pluginId, onOff):
        #self.lib.carla_set_active(pluginId, onOff)

    #def set_drywet(self, pluginId, value):
        #self.lib.carla_set_drywet(pluginId, value)

    #def set_volume(self, pluginId, value):
        #self.lib.carla_set_volume(pluginId, value)

    #def set_balance_left(self, pluginId, value):
        #self.lib.carla_set_balance_left(pluginId, value)

    #def set_balance_right(self, pluginId, value):
        #self.lib.carla_set_balance_right(pluginId, value)

    #def set_parameter_value(self, pluginId, parameterId, value):
        #self.lib.carla_set_parameter_value(pluginId, parameterId, value)

    #def set_parameter_midi_cc(self, pluginId, parameterId, cc):
        #self.lib.carla_set_parameter_midi_cc(pluginId, parameterId, cc)

    #def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        #self.lib.carla_set_parameter_midi_channel(pluginId, parameterId, channel)

    #def set_program(self, pluginId, programId):
        #self.lib.carla_set_program(pluginId, programId)

    #def set_midi_program(self, pluginId, midiProgramId):
        #self.lib.carla_set_midi_program(pluginId, midiProgramId)

    #def set_custom_data(self, pluginId, type_, key, value):
        #self.lib.carla_set_custom_data(pluginId, type_.encode("utf-8"), key.encode("utf-8"), value.encode("utf-8"))

    #def set_chunk_data(self, pluginId, chunkData):
        #self.lib.carla_set_chunk_data(pluginId, chunkData.encode("utf-8"))

    #def set_gui_container(self, pluginId, guiAddr):
        #self.lib.carla_set_gui_container(pluginId, guiAddr)

    #def show_gui(self, pluginId, yesNo):
        #self.lib.show_gui(pluginId, yesNo)

    #def idle_guis(self):
        #self.lib.idle_guis()

    #def send_midi_note(self, pluginId, channel, note, velocity):
        #self.lib.send_midi_note(pluginId, channel, note, velocity)

    #def prepare_for_save(self, pluginId):
        #self.lib.prepare_for_save(pluginId)

    #def set_callback_function(self, func):
        #self.callback = CallbackFunc(func)
        #self.lib.carla_set_callback_function(self.callback)

    #def set_option(self, option, value, valueStr):
        #self.lib.carla_set_option(option, value, valueStr.encode("utf-8"))

    #def get_last_error(self):
        #return self.lib.carla_get_last_error()

    #def get_host_osc_url(self):
        #return self.lib.carla_get_host_osc_url()

    #def get_buffer_size(self):
        #return self.lib.carla_get_buffer_size()

    #def get_sample_rate(self):
        #return self.lib.carla_get_sample_rate()

    #def nsm_announce(self, url, pid):
        #self.lib.nsm_announce(url.encode("utf-8"), pid)

    #def nsm_reply_open(self):
        #self.lib.nsm_reply_open()

    #def nsm_reply_save(self):
        #self.lib.nsm_reply_save()

Carla.host = Host(None)

# Test available drivers
driverCount = Carla.host.get_engine_driver_count()
driverList  = []
for i in range(driverCount):
    driver = cString(Carla.host.get_engine_driver_name(i))
    if driver:
        driverList.append(driver)
        print(i, driver)

# Test available internal plugins
pluginCount = Carla.host.get_internal_plugin_count()
for i in range(pluginCount):
    plugin = Carla.host.get_internal_plugin_info(i)
    print(plugin)
