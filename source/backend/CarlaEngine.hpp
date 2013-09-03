/*
 * Carla Engine API
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_ENGINE_HPP_INCLUDED
#define CARLA_ENGINE_HPP_INCLUDED

#include "CarlaBackend.hpp"
#include "CarlaMIDI.h"
#include "CarlaString.hpp"

#ifdef BUILD_BRIDGE
struct CarlaOscData;
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

/*!
 * @defgroup CarlaEngineAPI Carla Engine API
 *
 * The Carla Engine API.
 * @{
 */

/*!
 * The type of an engine.
 */
enum EngineType {
    /*!
    * Null engine type.
    */
    kEngineTypeNull = 0,

    /*!
    * JACK engine type.\n
    * Provides all processing modes.
    */
    kEngineTypeJack = 1,

    /*!
    * Juce engine type, used to provide Native Audio and MIDI support.
    */
    kEngineTypeJuce = 2,

    /*!
    * RtAudio engine type, used to provide Native Audio and MIDI support.
    */
    kEngineTypeRtAudio = 3,

    /*!
    * Plugin engine type, used to export the engine as a plugin.
    */
    kEngineTypePlugin = 4,

    /*!
    * Bridge engine type, used in BridgePlugin class.
    */
    kEngineTypeBridge = 5
};

/*!
 * The type of an engine port.
 */
enum EnginePortType {
    /*!
    * Null port type.
    */
    kEnginePortTypeNull = 0,

    /*!
    * Audio port type.
    * \see CarlaEngineAudioPort
    */
    kEnginePortTypeAudio = 1,

    /*!
    * CV port type.
    * \see CarlaEngineCVPort
    */
    kEnginePortTypeCV = 2,

    /*!
    * Event port type.
    ** \see CarlaEngineEventPort
    */
    kEnginePortTypeEvent = 3
};

/*!
 * The type of an engine event.
 */
enum EngineEventType {
    /*!
    * Null port type.
    */
    kEngineEventTypeNull = 0,

    /*!
    * Control event type.
    * \see EngineControlEvent
    */
    kEngineEventTypeControl = 1,

    /*!
    * MIDI event type.
    * \see EngineMidiEvent
    */
    kEngineEventTypeMidi = 2
};

/*!
 * The type of an engine control event.
 */
enum EngineControlEventType {
    /*!
    * Null event type.
    */
    kEngineControlEventTypeNull = 0,

    /*!
    * Parameter event type.\n
    * \note Value uses a normalized range of 0.0f<->1.0f.
    */
    kEngineControlEventTypeParameter = 1,

    /*!
    * MIDI Bank event type.
    */
    kEngineControlEventTypeMidiBank = 2,

    /*!
    * MIDI Program change event type.
    */
    kEngineControlEventTypeMidiProgram = 3,

    /*!
    * All sound off event type.
    */
    kEngineControlEventTypeAllSoundOff = 4,

    /*!
    * All notes off event type.
    */
    kEngineControlEventTypeAllNotesOff = 5
};

/*!
 * Engine control event.
 */
struct EngineControlEvent {
    EngineControlEventType type; //!< Control-Event type.
    uint16_t param;              //!< Parameter Id, midi bank or midi program.
    float    value;              //!< Parameter value, normalized to 0.0f<->1.0f.

    void clear() noexcept
    {
        type  = kEngineControlEventTypeNull;
        param = 0;
        value = 0.0f;
    }
};

/*!
 * Engine MIDI event.
 */
struct EngineMidiEvent {
    uint8_t port;    //!< Port offset (usually 0)
    uint8_t data[4]; //!< MIDI data, without channel bit
    uint8_t size;    //!< Number of bytes used

    void clear() noexcept
    {
        port    = 0;
        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 0;
        size    = 0;
    }
};

/*!
 * Engine event.
 */
struct EngineEvent {
    EngineEventType type; //!< Event Type; either Control or MIDI
    uint32_t time;        //!< Time offset in frames
    uint8_t  channel;     //!< Channel, used for MIDI-related events

    union {
        EngineControlEvent ctrl;
        EngineMidiEvent midi;
    };

    EngineEvent() noexcept
    {
        clear();
    }

    void clear() noexcept
    {
        type = kEngineEventTypeNull;
        time = 0;
        channel = 0;
    }
};

/*!
 * Engine options.
 */
struct EngineOptions {
    ProcessMode   processMode;
    TransportMode transportMode;

    bool forceStereo;
    bool preferPluginBridges;
    bool preferUiBridges;
    bool uisAlwaysOnTop;
#ifdef WANT_DSSI
    bool useDssiVstChunks;
#endif

    unsigned int maxParameters;
    unsigned int uiBridgesTimeout;

#ifdef WANT_RTAUDIO
    unsigned int rtaudioNumPeriods;
    unsigned int rtaudioBufferSize;
    unsigned int rtaudioSampleRate;
    CarlaString  rtaudioDevice;
#endif

    CarlaString resourceDir;

#ifndef BUILD_BRIDGE
    CarlaString bridge_native;
    CarlaString bridge_posix32;
    CarlaString bridge_posix64;
    CarlaString bridge_win32;
    CarlaString bridge_win64;
#endif
#ifdef WANT_LV2
    CarlaString bridge_lv2Extrn;
    CarlaString bridge_lv2Gtk2;
    CarlaString bridge_lv2Gtk3;
    CarlaString bridge_lv2Qt4;
    CarlaString bridge_lv2Qt5;
    CarlaString bridge_lv2Cocoa;
    CarlaString bridge_lv2Win;
    CarlaString bridge_lv2X11;
#endif
#ifdef WANT_VST
    CarlaString bridge_vstMac;
    CarlaString bridge_vstHWND;
    CarlaString bridge_vstX11;
#endif

    EngineOptions()
#if defined(CARLA_OS_LINUX)
        : processMode(PROCESS_MODE_MULTIPLE_CLIENTS),
          transportMode(TRANSPORT_MODE_JACK),
#else
        : processMode(PROCESS_MODE_CONTINUOUS_RACK),
          transportMode(TRANSPORT_MODE_INTERNAL),
#endif
          forceStereo(false),
          preferPluginBridges(false),
          preferUiBridges(true),
          uisAlwaysOnTop(true),
#ifdef WANT_DSSI
          useDssiVstChunks(false),
#endif
          maxParameters(MAX_DEFAULT_PARAMETERS),
          uiBridgesTimeout(4000),
#ifdef WANT_RTAUDIO
          rtaudioNumPeriods(2),
          rtaudioBufferSize(512),
          rtaudioSampleRate(44100),
#endif
          resourceDir() {}
};

/*!
 * Engine BBT Time information.
 */
struct EngineTimeInfoBBT {
    int32_t bar;  //!< current bar
    int32_t beat; //!< current beat-within-bar
    int32_t tick; //!< current tick-within-beat
    double barStartTick;

    float beatsPerBar; //!< time signature "numerator"
    float beatType;    //!< time signature "denominator"

    double ticksPerBeat;
    double beatsPerMinute;

    EngineTimeInfoBBT() noexcept
        : bar(0),
          beat(0),
          tick(0),
          barStartTick(0.0),
          beatsPerBar(0.0f),
          beatType(0.0f),
          ticksPerBeat(0.0),
          beatsPerMinute(0.0) {}
};

/*!
 * Engine Time information.
 */
struct EngineTimeInfo {
    static const uint32_t ValidBBT = 0x1;

    bool playing;
    uint64_t frame;
    uint64_t usecs;
    uint32_t valid;
    EngineTimeInfoBBT bbt;

    EngineTimeInfo() noexcept
    {
        clear();
    }

    void clear() noexcept
    {
        playing = false;
        frame   = 0;
        usecs   = 0;
        valid   = 0x0;
    }

    // quick operator, doesn't check all values
    bool operator==(const EngineTimeInfo& timeInfo) const noexcept
    {
        if (timeInfo.playing != playing || timeInfo.frame != frame)
            return false;
        if ((timeInfo.valid & ValidBBT) != (valid & ValidBBT))
            return false;
        if (timeInfo.bbt.beatsPerMinute != bbt.beatsPerMinute)
            return false;
        return true;
    }

    bool operator!=(const EngineTimeInfo& timeInfo) const noexcept
    {
        return !operator==(timeInfo);
    }
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine port (Abstract).\n
 * This is the base class for all Carla Engine ports.
 */
class CarlaEnginePort
{
public:
    /*!
     * The constructor.\n
     * Param \a isInput defines wherever this is an input port or not (output otherwise).\n
     * Input/output and process mode are constant for the lifetime of the port.
     */
    CarlaEnginePort(const CarlaEngine& engine, const bool isInput);

    /*!
     * The destructor.
     */
    virtual ~CarlaEnginePort();

    /*!
     * Get the type of the port, as provided by the respective subclasses.
     */
    virtual EnginePortType getType() const noexcept = 0;

    /*!
     * Initialize the port's internal buffer.
     */
    virtual void initBuffer() = 0;

#ifndef DOXYGEN
protected:
    const CarlaEngine& fEngine;
    const bool fIsInput;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEnginePort)
#endif
};

/*!
 * Carla Engine Audio port.
 */
class CarlaEngineAudioPort : public CarlaEnginePort
{
public:
    /*!
     * The constructor.\n
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineAudioPort(const CarlaEngine& engine, const bool isInput);

    /*!
     * The destructor.
     */
    virtual ~CarlaEngineAudioPort() override;

    /*!
     * Get the type of the port, in this case kEnginePortTypeAudio.
     */
    EnginePortType getType() const noexcept override
    {
        return kEnginePortTypeAudio;
    }

    /*!
     * Initialize the port's internal buffer.
     */
    virtual void initBuffer() override;

    /*!
     * Direct access to the port's audio buffer.
     */
    float* getBuffer() const noexcept
    {
        return fBuffer;
    }

#ifndef DOXYGEN
protected:
    float* fBuffer;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineAudioPort)
#endif
};

/*!
 * Carla Engine CV port.
 */
class CarlaEngineCVPort : public CarlaEnginePort
{
public:
    /*!
     * The constructor.\n
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineCVPort(const CarlaEngine& engine, const bool isInput);

    /*!
     * The destructor.
     */
    virtual ~CarlaEngineCVPort() override;

    /*!
     * Get the type of the port, in this case kEnginePortTypeCV.
     */
    EnginePortType getType() const noexcept override
    {
        return kEnginePortTypeCV;
    }

    /*!
     * Initialize the port's internal buffer for \a engine.
     */
    virtual void initBuffer() override;

    /*!
     * Write buffer back into the engine.
     */
    virtual void writeBuffer(const uint32_t frames, const uint32_t timeOffset);

    /*!
     * Set a new buffer size.
     */
    void setBufferSize(const uint32_t bufferSize);

    /*!
     * Direct access to the port's audio buffer.
     */
    float* getBuffer() const noexcept
    {
        return fBuffer;
    }

#ifndef DOXYGEN
protected:
    float* fBuffer;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineCVPort)
#endif
};

/*!
 * Carla Engine Event port.
 */
class CarlaEngineEventPort : public CarlaEnginePort
{
public:
    /*!
     * The constructor.\n
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineEventPort(const CarlaEngine& engine, const bool isInput);

    /*!
     * The destructor.
     */
    virtual ~CarlaEngineEventPort() override;

    /*!
     * Get the type of the port, in this case kEnginePortTypeEvent.
     */
    EnginePortType getType() const noexcept override
    {
        return kEnginePortTypeEvent;
    }

    /*!
     * Initialize the port's internal buffer for \a engine.
     */
    virtual void initBuffer() override;

    /*!
     * Get the number of events present in the buffer.
     * \note You must only call this for input ports.
     */
    virtual uint32_t getEventCount() const;

    /*!
     * Get the event at \a index.
     * \note You must only call this for input ports.
     */
    virtual const EngineEvent& getEvent(const uint32_t index);

    /*!
     * Write a control event into the buffer.\n
     * Arguments are the same as in the EngineControlEvent struct.
     * \note You must only call this for output ports.
     */
    virtual void writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value = 0.0f);

    /*!
     * Write a control event into the buffer, overloaded call.
     */
    void writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEvent& ctrl)
    {
        writeControlEvent(time, channel, ctrl.type, ctrl.param, ctrl.value);
    }

    /*!
     * Write a MIDI event into the buffer.\n
     * Arguments are the same as in the EngineMidiEvent struct.
     * \note You must only call this for output ports.
     */
    virtual void writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t* const data, const uint8_t size);

    /*!
     * Write a MIDI event into the buffer, overloaded call.
     */
    void writeMidiEvent(const uint32_t time, const uint8_t* const data, const uint8_t size)
    {
        writeMidiEvent(time, MIDI_GET_CHANNEL_FROM_DATA(data), 0, data, size);
    }

    /*!
     * Write a MIDI event into the buffer, overloaded call.
     */
    void writeMidiEvent(const uint32_t time, const uint8_t channel, const EngineMidiEvent& midi)
    {
        writeMidiEvent(time, channel, midi.port, midi.data, midi.size);
    }

#ifndef DOXYGEN
protected:
    EngineEvent* fBuffer;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineEventPort)
#endif
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine client.\n
 * Each plugin requires one client from the engine (created via CarlaEngine::addPort()).\n
 * \note This is a virtual class, each engine type provides its own funtionality.
 */
class CarlaEngineClient
{
public:
    /*!
     * The constructor, protected.\n
     * All constructor parameters are constant and will never change in the lifetime of the client.\n
     * Client starts in deactivated state.
     */
    CarlaEngineClient(const CarlaEngine& engine);

    /*!
     * The destructor.
     */
    virtual ~CarlaEngineClient();

    /*!
     * Activate this client.\n
     * Client must be deactivated before calling this function.
     */
    virtual void activate();

    /*!
     * Deactivate this client.\n
     * Client must be activated before calling this function.
     */
    virtual void deactivate();

    /*!
     * Check if the client is activated.
     */
    virtual bool isActive() const;

    /*!
     * Check if the client is ok.\n
     * Plugins will refuse to instantiate if this returns false.
     * \note This is always true in rack and patchbay processing modes.
     */
    virtual bool isOk() const;

    /*!
     * Get the current latency, in samples.
     */
    virtual uint32_t getLatency() const;

    /*!
     * Change the client's latency.
     */
    virtual void setLatency(const uint32_t samples);

    /*!
     * Add a new port of type \a portType.
     * \note This function does nothing in rack processing mode since ports are static there (2 audio + 1 event for both input and output).
     */
    virtual CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput);

#ifndef DOXYGEN
protected:
    const CarlaEngine& fEngine;

    bool     fActive;
    uint32_t fLatency;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineClient)
#endif
};

// -----------------------------------------------------------------------

/*!
 * Protected data used in CarlaEngine and subclasses.\n
 * Non-engine code MUST NEVER have direct access to this.
 */
struct CarlaEngineProtectedData;

/*!
 * Carla Engine.
 * \note This is a virtual class for all available engine types available in Carla.
 */
class CarlaEngine
{
protected:
    /*!
     * The constructor, protected.\n
     * \note This only initializes engine data, it doesn't initialize the engine itself.
     */
    CarlaEngine();

public:
    /*!
     * The decontructor.
     * The engine must have been closed before this happens.
     */
    virtual ~CarlaEngine();

    // -------------------------------------------------------------------
    // Static calls

    /*!
     * Get the number of available engine drivers.
     */
    static unsigned int getDriverCount();

    /*!
     * Get the name of the engine driver at \a index.
     */
    static const char* getDriverName(const unsigned int index);

    /*!
     * Get the device names of driver at \a index (for use in non-JACK drivers).\n
     * May return NULL.
     */
    static const char** getDriverDeviceNames(const unsigned int index);

    /*!
     * Create a new engine, using driver \a driverName. \n
     * Returned variable must be deleted when no longer needed.
     * \note This only initializes engine data, it doesn't initialize the engine itself.
     */
    static CarlaEngine* newDriverByName(const char* const driverName);

    // -------------------------------------------------------------------
    // Constant values

    /*!
     * Maximum client name size.
     */
    virtual unsigned int getMaxClientNameSize() const;

    /*!
     * Maximum port name size.
     */
    virtual unsigned int getMaxPortNameSize() const;

    /*!
     * Current number of plugins loaded.
     */
    unsigned int getCurrentPluginCount() const noexcept;

    /*!
     * Maximum number of loadable plugins allowed.
     * \note This function returns 0 if engine is not started.
     */
    unsigned int getMaxPluginNumber() const noexcept;

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    /*!
     * Initialize engine, using \a clientName.
     */
    virtual bool init(const char* const clientName);

    /*!
     * Close engine.
     */
    virtual bool close();

    /*!
     * Idle engine.
     */
    virtual void idle();

    /*!
     * Check if engine is running.
     */
    virtual bool isRunning() const noexcept = 0;

    /*!
     * Check if engine is running offline (aka freewheel mode).
     */
    virtual bool isOffline() const noexcept = 0;

    /*!
     * Get engine type.
     */
    virtual EngineType getType() const noexcept = 0;

    /*!
     * Add new engine client.
     * \note This function must only be called within a plugin class.
     */
    virtual CarlaEngineClient* addClient(CarlaPlugin* const plugin);

    // -------------------------------------------------------------------
    // Plugin management

    /*!
     * Add new plugin.
     */
    bool addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, const void* const extra = nullptr);

    /*!
     * Add new plugin, using native binary type.
     */
    bool addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, const void* const extra = nullptr)
    {
        return addPlugin(BINARY_NATIVE, ptype, filename, name, label, extra);
    }

    /*!
     * Remove plugin with id \a id.
     */
    bool removePlugin(const unsigned int id);

    /*!
     * Remove all plugins.
     */
    void removeAllPlugins();

    /*!
     * Rename plugin with id \a id to \a newName.\n
     * Returns the new name, or nullptr if the operation failed.
     */
    virtual const char* renamePlugin(const unsigned int id, const char* const newName);

    /*!
     * Clone plugin with id \a id.
     */
    bool clonePlugin(const unsigned int id);

    /*!
     * Prepare replace of plugin with id \a id.\n
     * The next call to addPlugin() will use this id, replacing the selected plugin.
     * \note This function requires addPlugin() to be called afterwards, as soon as possible.
     */
    bool replacePlugin(const unsigned int id);

    /*!
     * Switch plugins with id \a idA and \a idB.
     */
    bool switchPlugins(const unsigned int idA, const unsigned int idB);

    /*!
     * Get plugin with id \a id.
     */
    CarlaPlugin* getPlugin(const unsigned int id) const;

    /*!
     * Get plugin with id \a id, faster unchecked version.
     */
    CarlaPlugin* getPluginUnchecked(const unsigned int id) const noexcept;

    /*!
     * Get a unique plugin name within the engine.\n
     * Returned variable must NOT be free'd.
     */
    const char* getUniquePluginName(const char* const name);

    // -------------------------------------------------------------------
    // Project management

    /*!
     * Load \a filename of any type.\n
     * This will try to load a generic file as a plugin,
     * either by direct handling (GIG, SF2 and SFZ) or by using an internal plugin (like Audio and MIDI).
     */
    bool loadFilename(const char* const filename);

    /*!
     * Load \a filename project file.\n
     * (project files have *.carxp extension)
     * \note Already loaded plugins are not removed; call removeAllPlugins() first if needed.
     */
    bool loadProject(const char* const filename);

    /*!
     * Save current project to \a filename.\n
     * (project files have *.carxp extension)
     */
    bool saveProject(const char* const filename);

    // -------------------------------------------------------------------
    // Information (base)

    /*!
     * Get current buffer size.
     */
    uint32_t getBufferSize() const noexcept
    {
        return fBufferSize;
    }

    /*!
     * Get current sample rate.
     */
    double getSampleRate() const noexcept
    {
        return fSampleRate;
    }

    /*!
     * Get engine name.
     */
    const char* getName() const noexcept
    {
        return (const char*)fName;
    }

    /*!
     * Get the engine options (read-only).
     */
    const EngineOptions& getOptions() const noexcept
    {
        return fOptions;
    }

    /*!
     * Get the engine proccess mode.
     */
    ProcessMode getProccessMode() const noexcept
    {
        return fOptions.processMode;
    }

    /*!
     * Get current Time information (read-only).
     */
    const EngineTimeInfo& getTimeInfo() const noexcept
    {
        return fTimeInfo;
    }

    // -------------------------------------------------------------------
    // Information (peaks)

    /*!
     * TODO.
     * \a id must be either 1 or 2.
     */
    float getInputPeak(const unsigned int pluginId, const unsigned short id) const;

    /*!
     * TODO.
     * \a id must be either 1 or 2.
     */
    float getOutputPeak(const unsigned int pluginId, const unsigned short id) const;

    // -------------------------------------------------------------------
    // Callback

    /*!
     * TODO.
     */
    void callback(const CallbackType action, const unsigned int pluginId, const int value1, const int value2, const float value3, const char* const valueStr);

    /*!
     * TODO.
     */
    void setCallback(const CallbackFunc func, void* const ptr);

    // -------------------------------------------------------------------
    // Patchbay

    /*!
     * Connect patchbay ports \a portA and \a portB.
     */
    virtual bool patchbayConnect(int portA, int portB);

    /*!
     * Disconnect patchbay connection \a connectionId.
     */
    virtual bool patchbayDisconnect(int connectionId);

    /*!
     * Force the engine to resend all patchbay clients, ports and connections again.
     */
    virtual void patchbayRefresh();

    // -------------------------------------------------------------------
    // Transport

    /*!
     * Start playback of the engine transport.
     */
    virtual void transportPlay();

    /*!
     * Pause the engine transport.
     */
    virtual void transportPause();

    /*!
     * Relocate the engine transport to \a frames.
     */
    virtual void transportRelocate(const uint32_t frame);

    // -------------------------------------------------------------------
    // Error handling

    /*!
     * Get last error.
     */
    const char* getLastError() const noexcept;

    /*!
     * Set last error.
     */
    void setLastError(const char* const error);

    // -------------------------------------------------------------------
    // Misc

    /*!
     * Tell the engine it's about to close.\n
     * This is used to prevent the engine thread(s) from reactivating.
     */
    void setAboutToClose();

    // -------------------------------------------------------------------
    // Options

    /*!
     * Set the engine option \a option.
     */
    void setOption(const OptionsType option, const int value, const char* const valueStr);

    // -------------------------------------------------------------------
    // OSC Stuff

#ifdef BUILD_BRIDGE
    /*!
     * Check if OSC bridge is registered.
     */
    bool isOscBridgeRegistered() const noexcept;
#else
    /*!
     * Check if OSC controller is registered.
     */
    bool isOscControlRegistered() const noexcept;
#endif

    /*!
     * Idle OSC.
     */
    void idleOsc();

    /*!
     * Get OSC TCP server path.
     */
    const char* getOscServerPathTCP() const noexcept;

    /*!
     * Get OSC UDP server path.
     */
    const char* getOscServerPathUDP() const noexcept;

#ifdef BUILD_BRIDGE
    /*!
     * Set OSC bridge data.
     */
    void setOscBridgeData(const CarlaOscData* const oscData) noexcept;
#endif

    // -------------------------------------------------------------------
    // Helper functions

    /*!
     * Return internal data, needed for EventPorts when used in Rack and Bridge modes.
     * \note RT call
     */
    EngineEvent* getInternalEventBuffer(const bool isInput) const noexcept;

    /*!
     * Force register a plugin into slot \a id. \n
     * This is needed so that we can receive OSC events for the plugin while it initializes.
     */
    void registerEnginePlugin(const unsigned int id, CarlaPlugin* const plugin);

    // -------------------------------------------------------------------

protected:
    /*!
     * Current buffer size.
     * \see getBufferSize()
     */
    uint32_t fBufferSize;

    /*!
     * Current sample rate.
     * \see getSampleRate()
     */
    double fSampleRate;

    /*!
     * Engine name.
     * \see getName()
     */
    CarlaString fName;

    /*!
     * Engine options.
     * \see getOptions() and setOption()
     */
    EngineOptions fOptions;

    /*!
     * Current time-pos information.
     * \see getTimeInfo()
     */
    EngineTimeInfo fTimeInfo;

    /*!
     * Internal data, for CarlaEngine subclasses only.
     */
    CarlaEngineProtectedData* const pData;
    friend struct CarlaEngineProtectedData;

    // -------------------------------------------------------------------
    // Internal stuff

    /*!
     * Report to all plugins about buffer size change.
     */
    void bufferSizeChanged(const uint32_t newBufferSize);

    /*!
     * Report to all plugins about sample rate change.\n
     * This is not supported on all plugin types, in which case they will have to be re-initiated.
     */
    void sampleRateChanged(const double newSampleRate);

    /*!
     * Report to all plugins about offline mode change.
     */
    void offlineModeChanged(const bool isOffline);

    /*!
     * Run any pending RT events.\n
     * Must always be called at the end of audio processing.
     * \note RT call
     */
    void runPendingRtEvents();

    /*!
     * Set a plugin (stereo) peak values.
     * \note RT call
     */
    void setPluginPeaks(const unsigned int pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept;

#ifndef BUILD_BRIDGE
    /*!
     * Proccess audio buffer in rack mode.
     * \note RT call
     */
    void processRack(float* inBuf[2], float* outBuf[2], const uint32_t frames);

    /*!
     * Proccess audio buffer in patchbay mode.
     * In \a bufCount, [0]=inBufCount and [1]=outBufCount
     * \note RT call
     */
    void processPatchbay(float** inBuf, float** outBuf, const uint32_t bufCount[2], const uint32_t frames);
#endif

    // -------------------------------------------------------------------
    // Engine initializers

#ifdef BUILD_BRIDGE
public:
    static CarlaEngine* newBridge(const char* const audioBaseName, const char* const controlBaseName);
#endif

private:
    static CarlaEngine* newJack();

    static CarlaEngine* newJuce();
    static size_t       getJuceApiCount();
    static const char*  getJuceApiName(const unsigned int index);
    static const char** getJuceApiDeviceNames(const unsigned int index);

#ifdef WANT_RTAUDIO
    enum RtAudioApi {
        RTAUDIO_DUMMY        = 0,
        RTAUDIO_LINUX_ALSA   = 1,
        RTAUDIO_LINUX_PULSE  = 2,
        RTAUDIO_LINUX_OSS    = 3,
        RTAUDIO_UNIX_JACK    = 4,
        RTAUDIO_MACOSX_CORE  = 5,
        RTAUDIO_WINDOWS_ASIO = 6,
        RTAUDIO_WINDOWS_DS   = 7
    };

    static CarlaEngine* newRtAudio(const RtAudioApi api);
    static size_t       getRtAudioApiCount();
    static const char*  getRtAudioApiName(const unsigned int index);
    static const char** getRtAudioApiDeviceNames(const unsigned int index);
#endif

    // -------------------------------------------------------------------
    // Bridge/Controller OSC stuff

public:
#ifdef BUILD_BRIDGE
    void oscSend_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total);
    void oscSend_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total);
    void oscSend_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total);
    void oscSend_bridge_program_count(const int32_t count);
    void oscSend_bridge_midi_program_count(const int32_t count);
    void oscSend_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId);
    void oscSend_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit);
    void oscSend_bridge_parameter_data(const int32_t index, const int32_t type, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC);
    void oscSend_bridge_parameter_ranges(const int32_t index, const float def, const float min, const float max, const float step, const float stepSmall, const float stepLarge);
    void oscSend_bridge_program_info(const int32_t index, const char* const name);
    void oscSend_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label);
    void oscSend_bridge_configure(const char* const key, const char* const value);
    void oscSend_bridge_set_parameter_value(const int32_t index, const float value);
    void oscSend_bridge_set_default_value(const int32_t index, const float value);
    void oscSend_bridge_set_program(const int32_t index);
    void oscSend_bridge_set_midi_program(const int32_t index);
    void oscSend_bridge_set_custom_data(const char* const type, const char* const key, const char* const value);
    void oscSend_bridge_set_chunk_data(const char* const chunkFile);
    void oscSend_bridge_set_peaks();
#else
    void oscSend_control_add_plugin_start(const int32_t pluginId, const char* const pluginName);
    void oscSend_control_add_plugin_end(const int32_t pluginId);
    void oscSend_control_remove_plugin(const int32_t pluginId);
    void oscSend_control_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints, const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId);
    void oscSend_control_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts, const int32_t cIns, const int32_t cOuts, const int32_t cTotals);
    void oscSend_control_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints, const char* const name, const char* const label, const float current);
    void oscSend_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const float min, const float max, const float def, const float step, const float stepSmall, const float stepLarge);
    void oscSend_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc);
    void oscSend_control_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel);
    void oscSend_control_set_parameter_value(const int32_t pluginId, const int32_t index, const float value);
    void oscSend_control_set_default_value(const int32_t pluginId, const int32_t index, const float value);
    void oscSend_control_set_program(const int32_t pluginId, const int32_t index);
    void oscSend_control_set_program_count(const int32_t pluginId, const int32_t count);
    void oscSend_control_set_program_name(const int32_t pluginId, const int32_t index, const char* const name);
    void oscSend_control_set_midi_program(const int32_t pluginId, const int32_t index);
    void oscSend_control_set_midi_program_count(const int32_t pluginId, const int32_t count);
    void oscSend_control_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name);
    void oscSend_control_note_on(const int32_t pluginId, const int32_t channel, const int32_t note, const int32_t velo);
    void oscSend_control_note_off(const int32_t pluginId, const int32_t channel, const int32_t note);
    void oscSend_control_set_peaks(const int32_t pluginId);
    void oscSend_control_exit();
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngine)
};

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_HPP_INCLUDED
