/*
 * Carla Tests
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPlugin.hpp"
#include "CarlaEngine.hpp"

CARLA_BACKEND_START_NAMESPACE

// 1 - utils
// 2 - engine
// 3 - plugin
// 4 - standalone
#define ANSI_TEST_N 2

#if ANSI_TEST_N != 3
struct SaveState {};

// -----------------------------------------------------------------------
// Fallback data

static const ParameterData   kParameterDataNull;
static const ParameterRanges kParameterRangesNull;
static const MidiProgramData kMidiProgramDataNull;
static const CustomData      kCustomDataNull;

// -----------------------------------------------------------------------
// CarlaPlugin

CarlaPlugin::CarlaPlugin(CarlaEngine* const, const unsigned int id)
  : fId(id), pData(nullptr) {}
CarlaPlugin::~CarlaPlugin() {}

uint32_t CarlaPlugin::getLatencyInFrames()                        const noexcept { return 0; }
uint32_t CarlaPlugin::getAudioInCount()                           const noexcept { return 0; }
uint32_t CarlaPlugin::getAudioOutCount()                          const noexcept { return 0; }
uint32_t CarlaPlugin::getMidiInCount()                            const noexcept { return 0; }
uint32_t CarlaPlugin::getMidiOutCount()                           const noexcept { return 0; }
uint32_t CarlaPlugin::getParameterCount()                         const noexcept { return 0; }
uint32_t CarlaPlugin::getParameterScalePointCount(const uint32_t) const          { return 0; }
uint32_t CarlaPlugin::getProgramCount()                           const noexcept { return 0; }
uint32_t CarlaPlugin::getMidiProgramCount()                       const noexcept { return 0; }
uint32_t CarlaPlugin::getCustomDataCount()                        const noexcept { return 0; }
int32_t  CarlaPlugin::getCurrentProgram()                         const noexcept { return 0; }
int32_t  CarlaPlugin::getCurrentMidiProgram()                     const noexcept { return 0; }

const ParameterData&   CarlaPlugin::getParameterData(const uint32_t)   const { return kParameterDataNull; }
const ParameterRanges& CarlaPlugin::getParameterRanges(const uint32_t) const { return kParameterRangesNull; }
bool                   CarlaPlugin::isParameterOutput(const uint32_t)  const { return false; }
const MidiProgramData& CarlaPlugin::getMidiProgramData(const uint32_t) const { return kMidiProgramDataNull; }
const CustomData&      CarlaPlugin::getCustomData(const uint32_t)      const { return kCustomDataNull; }
int32_t                CarlaPlugin::getChunkData(void** const)         const { return 0; }
unsigned int           CarlaPlugin::getAvailableOptions()              const { return 0x0; }

float CarlaPlugin::getParameterValue(const uint32_t)                           const { return 0.0f; }
float CarlaPlugin::getParameterScalePointValue(const uint32_t, const uint32_t) const { return 0.0f; }

void CarlaPlugin::getLabel(char* const)     const noexcept {}
void CarlaPlugin::getMaker(char* const)     const noexcept {}
void CarlaPlugin::getCopyright(char* const) const noexcept {}
void CarlaPlugin::getRealName(char* const)  const noexcept {}
void CarlaPlugin::getParameterName(const uint32_t, char* const)   const {}
void CarlaPlugin::getParameterSymbol(const uint32_t, char* const) const {}
void CarlaPlugin::getParameterText(const uint32_t, const float, char* const)   const {}
void CarlaPlugin::getParameterUnit(const uint32_t, char* const)   const {}
void CarlaPlugin::getParameterScalePointLabel(const uint32_t, const uint32_t, char* const) const {}
void CarlaPlugin::getProgramName(const uint32_t, char* const)                              const {}
void CarlaPlugin::getMidiProgramName(const uint32_t, char* const)                          const {}
void CarlaPlugin::getParameterCountInfo(uint32_t* const, uint32_t* const, uint32_t* const) const {}

void             CarlaPlugin::prepareForSave() {}
const SaveState& CarlaPlugin::getSaveState()   { static SaveState saveState; return saveState; }

void CarlaPlugin::loadSaveState(const SaveState&) {}
bool CarlaPlugin::saveStateToFile(const char* const)   { return false; }
bool CarlaPlugin::loadStateFromFile(const char* const) { return false; }

void CarlaPlugin::setId(const unsigned int newId) noexcept  { fId = newId; }
void CarlaPlugin::setName(const char* const newName)        { fName = newName; }
void CarlaPlugin::setEnabled(const bool yesNo)              { fEnabled = yesNo; }

void CarlaPlugin::setOption(const unsigned int, const bool) {}
void CarlaPlugin::setActive(const bool, const bool, const bool) {}
void CarlaPlugin::setDryWet(const float, const bool, const bool) {}
void CarlaPlugin::setVolume(const float, const bool, const bool) {}
void CarlaPlugin::setBalanceLeft(const float, const bool, const bool) {}
void CarlaPlugin::setBalanceRight(const float, const bool, const bool) {}
void CarlaPlugin::setPanning(const float, const bool, const bool) {}
void CarlaPlugin::setCtrlChannel(const int8_t, const bool, const bool) {}
void CarlaPlugin::setParameterValue(const uint32_t, const float, const bool, const bool, const bool) {}
void CarlaPlugin::setParameterValueByRealIndex(const int32_t, const float, const bool, const bool, const bool) {}
void CarlaPlugin::setParameterMidiChannel(const uint32_t, uint8_t, const bool, const bool) {}
void CarlaPlugin::setParameterMidiCC(const uint32_t, int16_t, const bool, const bool) {}
void CarlaPlugin::setCustomData(const char* const, const char* const, const char* const, const bool) {}
void CarlaPlugin::setChunkData(const char* const) {}
void CarlaPlugin::setProgram(int32_t, const bool, const bool, const bool) {}
void CarlaPlugin::setMidiProgram(int32_t, const bool, const bool, const bool) {}
void CarlaPlugin::setMidiProgramById(const uint32_t, const uint32_t, const bool, const bool, const bool) {}
void CarlaPlugin::showGui(const bool) {}
void CarlaPlugin::idleGui() {}
void CarlaPlugin::reloadPrograms(const bool) {}
void CarlaPlugin::activate() {}
void CarlaPlugin::deactivate() {}
void CarlaPlugin::bufferSizeChanged(const uint32_t) {}
void CarlaPlugin::sampleRateChanged(const double) {}
void CarlaPlugin::offlineModeChanged(const bool) {}
bool CarlaPlugin::tryLock() { return false; }
void CarlaPlugin::unlock() {}
void CarlaPlugin::initBuffers() {}
void CarlaPlugin::clearBuffers() {}
void CarlaPlugin::registerToOscClient() {}
void CarlaPlugin::updateOscData(const lo_address&, const char* const) {}
//void CarlaPlugin::freeOscData() {}
bool CarlaPlugin::waitForOscGuiShow() { return false; }
void CarlaPlugin::sendMidiSingleNote(const uint8_t, const uint8_t, const uint8_t, const bool, const bool, const bool) {}
void CarlaPlugin::sendMidiAllNotesOffToCallback() {}
void CarlaPlugin::postRtEventsRun() {}
void CarlaPlugin::uiParameterChange(const uint32_t, const float) {}
void CarlaPlugin::uiProgramChange(const uint32_t) {}
void CarlaPlugin::uiMidiProgramChange(const uint32_t) {}
void CarlaPlugin::uiNoteOn(const uint8_t, const uint8_t, const uint8_t) {}
void CarlaPlugin::uiNoteOff(const uint8_t, const uint8_t) {}

size_t CarlaPlugin::getNativePluginCount() { return 0; }
const PluginDescriptor* CarlaPlugin::getNativePluginDescriptor(const size_t) { return nullptr; }
#endif

#if ANSI_TEST_N != 2
// -----------------------------------------------------------------------
// CarlaEngine

CarlaEngine::CarlaEngine()
    : fBufferSize(0),
      fSampleRate(0.0),
      pData(nullptr)        {}
CarlaEngine::~CarlaEngine() {}

unsigned int CarlaEngine::getDriverCount()                         { return 0; }
const char*  CarlaEngine::getDriverName(const unsigned int)        { return nullptr; }
const char** CarlaEngine::getDriverDeviceNames(const unsigned int) { return nullptr; }
CarlaEngine* CarlaEngine::newDriverByName(const char* const)       { return nullptr; }

unsigned int CarlaEngine::getMaxClientNameSize()  const noexcept { return 0; }
unsigned int CarlaEngine::getMaxPortNameSize()    const noexcept { return 0; }
unsigned int CarlaEngine::getCurrentPluginCount() const noexcept { return 0; }
unsigned int CarlaEngine::getMaxPluginNumber()    const noexcept { return 0; }

bool CarlaEngine::init(const char* const) { return false; }
bool CarlaEngine::close() { return false; }
void CarlaEngine::idle() {}
CarlaEngineClient* CarlaEngine::addClient(CarlaPlugin* const) { return nullptr; }

void CarlaEngine::removeAllPlugins() {}
bool CarlaEngine::addPlugin(const BinaryType, const PluginType, const char* const, const char* const, const char* const,
                            const void* const)      { return false; }
bool CarlaEngine::removePlugin(const unsigned int)  { return false; }
bool CarlaEngine::clonePlugin(const unsigned int)   { return false; }
bool CarlaEngine::replacePlugin(const unsigned int) { return false; }
bool CarlaEngine::switchPlugins(const unsigned int, const unsigned int) { return false; }

const char*  CarlaEngine::renamePlugin(const unsigned int, const char* const)   { return nullptr; }
CarlaPlugin* CarlaEngine::getPlugin(const unsigned int)          const          { return nullptr; }
CarlaPlugin* CarlaEngine::getPluginUnchecked(const unsigned int) const noexcept { return nullptr; }
const char*  CarlaEngine::getUniquePluginName(const char* const)                { return nullptr; }

bool CarlaEngine::loadFilename(const char* const) { return false; }
bool CarlaEngine::loadProject(const char* const)  { return false; }
bool CarlaEngine::saveProject(const char* const)  { return false; }

float CarlaEngine::getInputPeak(const unsigned int, const unsigned short) const  { return 0.0f; }
float CarlaEngine::getOutputPeak(const unsigned int, const unsigned short) const { return 0.0f; }

void CarlaEngine::callback(const CallbackType, const unsigned int, const int, const int, const float, const char* const) {}
void CarlaEngine::setCallback(const CallbackFunc, void* const) {}
bool CarlaEngine::patchbayConnect(int, int) { return false; }
bool CarlaEngine::patchbayDisconnect(int)   { return false; }
void CarlaEngine::patchbayRefresh() {}
void CarlaEngine::transportPlay()   {}
void CarlaEngine::transportPause()  {}
void CarlaEngine::transportRelocate(const uint32_t) {}

const char* CarlaEngine::getLastError()                              const noexcept { return nullptr; }
void        CarlaEngine::setLastError(const char* const)                            {}
void        CarlaEngine::setAboutToClose()                                          {}
void        CarlaEngine::setOption(const OptionsType, const int, const char* const) {}
bool        CarlaEngine::isOscControlRegistered()     const noexcept { return false; }
void        CarlaEngine::idleOsc()                                   {}
const char* CarlaEngine::getOscServerPathTCP()        const noexcept { return nullptr; }
const char* CarlaEngine::getOscServerPathUDP()        const noexcept { return nullptr; }
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
