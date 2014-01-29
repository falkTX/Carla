/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBackend.h"

#ifdef BUILD_BRIDGE
struct CarlaOscData;
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* Fix editor indentation */
#endif

// -----------------------------------------------------------------------

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
    * Event port type (Control or MIDI).
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

// -----------------------------------------------------------------------

/*!
 * Engine control event.
 */
struct EngineControlEvent {
    EngineControlEventType type; //!< Control-Event type.
    uint16_t param;              //!< Parameter Id, midi bank or midi program.
    float    value;              //!< Parameter value, normalized to 0.0f<->1.0f.

    /*!
     * Dump control event as MIDI data.
     */
    void dumpToMidiData(const uint8_t channel, uint8_t& size, uint8_t data[3]) const noexcept;
};

/*!
 * Engine MIDI event.
 */
struct EngineMidiEvent {
    static const uint8_t kDataSize = 4; //!< Size of internal data

    uint8_t port; //!< Port offset (usually 0)
    uint8_t size; //!< Number of bytes used

    /*!
     * MIDI data, without channel bit.
     * If size > kDataSize, dataExt is used.
     */
    uint8_t        data[kDataSize];
    const uint8_t* dataExt;
};

/*!
 * Engine event.
 */
struct EngineEvent {
    EngineEventType type; //!< Event Type; either Control or MIDI
    uint32_t time;        //!< Time offset in frames
    uint8_t  channel;     //!< Channel, used for MIDI-related events

    /*!
     * Event specific data.
     */
    union {
        EngineControlEvent ctrl;
        EngineMidiEvent midi;
    };

    /*!
     * Fill event from MIDI data.
     */
    void fillFromMidiData(const uint8_t size, const uint8_t* const data) noexcept;
};

// -----------------------------------------------------------------------

/*!
 * Engine options.
 */
struct EngineOptions {
    EngineProcessMode   processMode;
    EngineTransportMode transportMode;

    bool forceStereo;
    bool preferPluginBridges;
    bool preferUiBridges;
    bool uisAlwaysOnTop;

    unsigned int maxParameters;
    unsigned int uiBridgesTimeout;
    unsigned int audioNumPeriods;
    unsigned int audioBufferSize;
    unsigned int audioSampleRate;
    const char*  audioDevice;

    const char* binaryDir;
    const char* resourceDir;

#ifndef DOXYGEN
    EngineOptions() noexcept;
    ~EngineOptions();
#endif
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

#ifndef DOXYGEN
    EngineTimeInfoBBT() noexcept;
#endif
};

/*!
 * Engine Time information.
 */
struct EngineTimeInfo {
    static const uint kValidBBT = 0x1;

    bool playing;
    uint64_t frame;
    uint64_t usecs;
    uint     valid;
    EngineTimeInfoBBT bbt;

    /*!
     * Clear.
     */
    void clear() noexcept;

#ifndef DOXYGEN
    EngineTimeInfo() noexcept;

    // quick operator, doesn't check all values
    bool operator==(const EngineTimeInfo& timeInfo) const noexcept;
    bool operator!=(const EngineTimeInfo& timeInfo) const noexcept;
#endif
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine port (Abstract).\n
 * This is the base class for all Carla Engine ports.
 */
class CarlaEnginePort
{
protected:
    /*!
     * The constructor.\n
     * Param \a isInputPort defines wherever this is an input port or not (output otherwise).\n
     * Input/output and process mode are constant for the lifetime of the port.
     */
    CarlaEnginePort(const CarlaEngine& engine, const bool isInputPort);

public:
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
    virtual void initBuffer() noexcept = 0;

    /*!
     * Check if this port is an input.
     */
    bool isInput() const noexcept
    {
        return fIsInput;
    }

#ifndef DOXYGEN
protected:
    const CarlaEngine& fEngine;
    const bool fIsInput;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEnginePort)
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
    CarlaEngineAudioPort(const CarlaEngine& engine, const bool isInputPort);

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
    virtual void initBuffer() noexcept override;

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

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineAudioPort)
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
    CarlaEngineCVPort(const CarlaEngine& engine, const bool isInputPort);

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
    virtual void initBuffer() noexcept override;

    /*!
     * Set a new buffer size.
     */
    void setBufferSize(const uint32_t bufferSize);

#if 0
    // TESTING: I should remove this
    /*!
     * Write buffer back into the engine.
     */
    virtual void writeBuffer(const uint32_t frames, const uint32_t timeOffset);
#endif

    /*!
     * Direct access to the port's buffer.
     */
    float* getBuffer() const noexcept
    {
        return fBuffer;
    }

#ifndef DOXYGEN
protected:
    float* fBuffer;
    const EngineProcessMode fProcessMode;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineCVPort)
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
    CarlaEngineEventPort(const CarlaEngine& engine, const bool isInputPort);

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
    virtual void initBuffer() noexcept override;

    /*!
     * Get the number of events present in the buffer.
     * \note You must only call this for input ports.
     */
    virtual uint32_t getEventCount() const noexcept;

    /*!
     * Get the event at \a index.
     * \note You must only call this for input ports.
     */
    virtual const EngineEvent& getEvent(const uint32_t index) const noexcept;

    /*!
     * Get the event at \a index, faster unchecked version.
     */
    virtual const EngineEvent& getEventUnchecked(const uint32_t index) const noexcept;

    /*!
     * Write a control event into the buffer.\n
     * Arguments are the same as in the EngineControlEvent struct.
     * \note You must only call this for output ports.
     */
    virtual bool writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value = 0.0f) noexcept;

    /*!
     * Write a control event into the buffer, overloaded call.
     */
    bool writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEvent& ctrl) noexcept;

    /*!
     * Write a MIDI event into the buffer.\n
     * Arguments are the same as in the EngineMidiEvent struct.
     * \note You must only call this for output ports.
     */
    virtual bool writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t port, const uint8_t size, const uint8_t* const data) noexcept;

    /*!
     * Write a MIDI event into the buffer, overloaded call.
     */
    bool writeMidiEvent(const uint32_t time, const uint8_t size, const uint8_t* const data) noexcept;

    /*!
     * Write a MIDI event into the buffer, overloaded call.
     */
    bool writeMidiEvent(const uint32_t time, const uint8_t channel, const EngineMidiEvent& midi) noexcept;

#ifndef DOXYGEN
protected:
    EngineEvent* fBuffer;
    const EngineProcessMode fProcessMode;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineEventPort)
#endif
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine client.\n
 * Each plugin requires one client from the engine (created via CarlaEngine::addClient()).\n
 * \note This is a virtual class, some engine types provide custom funtionality.
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
    virtual void activate() noexcept;

    /*!
     * Deactivate this client.\n
     * Client must be activated before calling this function.
     */
    virtual void deactivate() noexcept;

    /*!
     * Check if the client is activated.
     */
    virtual bool isActive() const noexcept;

    /*!
     * Check if the client is ok.\n
     * Plugins will refuse to instantiate if this returns false.
     * \note This is always true in rack and patchbay processing modes.
     */
    virtual bool isOk() const noexcept;

    /*!
     * Get the current latency, in samples.
     */
    virtual uint32_t getLatency() const noexcept;

    /*!
     * Change the client's latency.
     */
    virtual void setLatency(const uint32_t samples) noexcept;

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

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineClient)
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
     * \note This only initializes engine data, it doesn't start the engine (audio).
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
     * Get the device names of driver at \a index.
     */
    static const char* const* getDriverDeviceNames(const unsigned int index);

    /*!
     * Get the device names of driver at \a index.
     */
    static const EngineDriverDeviceInfo* getDriverDeviceInfo(const unsigned int index, const char* const driverName);

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
    virtual unsigned int getMaxClientNameSize() const noexcept;

    /*!
     * Maximum port name size.
     */
    virtual unsigned int getMaxPortNameSize() const noexcept;

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
     * Get the currently used driver name.
     */
    virtual const char* getCurrentDriverName() const noexcept = 0;

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
    bool removeAllPlugins();

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
     * Returned variable must be deleted if non-NULL.
     */
    const char* getUniquePluginName(const char* const name) const;

    // -------------------------------------------------------------------
    // Project management

    /*!
     * Load a file of any type.\n
     * This will try to load a generic file as a plugin,
     * either by direct handling (GIG, SF2 and SFZ) or by using an internal plugin (like Audio and MIDI).
     */
    bool loadFile(const char* const filename);

    /*!
     * Load a project file.
     * @note Already loaded plugins are not removed; call removeAllPlugins() first if needed.
     */
    bool loadProject(const char* const filename);

    /*!
     * Save current project to a file.
     */
    bool saveProject(const char* const filename);

    // -------------------------------------------------------------------
    // Information (base)

    /*!
     * Get the current engine driver hints.
     */
    unsigned int getHints() const noexcept;

    /*!
     * Get the current buffer size.
     */
    uint32_t getBufferSize() const noexcept;

    /*!
     * Get the current sample rate.
     */
    double getSampleRate() const noexcept;

    /*!
     * Get the current engine name.
     */
    const char* getName() const noexcept;

    /*!
     * Get the current engine proccess mode.
     */
    EngineProcessMode getProccessMode() const noexcept;

    /*!
     * Get the current engine options (read-only).
     */
    const EngineOptions& getOptions() const noexcept;

    /*!
     * Get the current Time information (read-only).
     */
    const EngineTimeInfo& getTimeInfo() const noexcept;

    // -------------------------------------------------------------------
    // Information (peaks)

    /*!
     * TODO.
     */
    float getInputPeak(const unsigned int pluginId, const bool isLeft) const noexcept;

    /*!
     * TODO.
     */
    float getOutputPeak(const unsigned int pluginId, const bool isLeft) const noexcept;

    // -------------------------------------------------------------------
    // Callback

    /*!
     * TODO.
     */
    void callback(const EngineCallbackOpcode action, const unsigned int pluginId, const int value1, const int value2, const float value3, const char* const valueStr) noexcept;

    /*!
     * TODO.
     */
    void setCallback(const EngineCallbackFunc func, void* const ptr) noexcept;

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // Patchbay

    /*!
     * Connect patchbay ports \a portA and \a portB.
     */
    virtual bool patchbayConnect(const int portA, const int portB);

    /*!
     * Disconnect patchbay connection \a connectionId.
     */
    virtual bool patchbayDisconnect(const uint connectionId);

    /*!
     * Force the engine to resend all patchbay clients, ports and connections again.
     */
    virtual bool patchbayRefresh();
#endif

    // -------------------------------------------------------------------
    // Transport

    /*!
     * Start playback of the engine transport.
     */
    virtual void transportPlay() noexcept;

    /*!
     * Pause the engine transport.
     */
    virtual void transportPause() noexcept;

    /*!
     * Relocate the engine transport to \a frames.
     */
    virtual void transportRelocate(const uint64_t frame) noexcept;

    // -------------------------------------------------------------------
    // Error handling

    /*!
     * Get last error.
     */
    const char* getLastError() const noexcept;

    /*!
     * Set last error.
     */
    void setLastError(const char* const error) const;

    // -------------------------------------------------------------------
    // Misc

    /*!
     * Tell the engine it's about to close.\n
     * This is used to prevent the engine thread(s) from reactivating.
     */
    void setAboutToClose() noexcept;

    // -------------------------------------------------------------------
    // Options

    /*!
     * Set the engine option \a option.
     */
    void setOption(const EngineOption option, const int value, const char* const valueStr);

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
    void idleOsc() const noexcept;

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
    void setOscBridgeData(const CarlaOscData* const oscData) const noexcept;
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
     * This is needed so we can receive OSC events for a plugin while it initializes.
     */
    void registerEnginePlugin(const unsigned int id, CarlaPlugin* const plugin) noexcept;

    // -------------------------------------------------------------------

protected:
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
    void runPendingRtEvents() noexcept;

    /*!
     * Set a plugin (stereo) peak values.
     * \note RT call
     */
    void setPluginPeaks(const unsigned int pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept;

    // -------------------------------------------------------------------
    // Patchbay stuff

    /*!
     * Virtual functions for handling MIDI ports in the rack patchbay.
     */
    virtual bool connectRackMidiInPort(const int)     { return false; }
    virtual bool connectRackMidiOutPort(const int)    { return false; }
    virtual bool disconnectRackMidiInPort(const int)  { return false; }
    virtual bool disconnectRackMidiOutPort(const int) { return false; }

    /*!
     * Virtual functions for handling patchbay state.
     */
    virtual const char* const* getPatchbayConnections() const;
    virtual void restorePatchbayConnection(const char* const sourcePort, const char* const targetPort);

    // -------------------------------------------------------------------

private:
    /*!
     * Native audio APIs.
     */
    enum AudioApi {
        AUDIO_API_NULL  = 0,
        // common
        AUDIO_API_JACK  = 1,
        // linux
        AUDIO_API_ALSA  = 2,
        AUDIO_API_OSS   = 3,
        AUDIO_API_PULSE = 4,
        // macos
        AUDIO_API_CORE  = 5,
        // windows
        AUDIO_API_ASIO  = 6,
        AUDIO_API_DS    = 7
    };

    // -------------------------------------------------------------------
    // Engine initializers

    // JACK
    static CarlaEngine*       newJack();

    // RtAudio
    static CarlaEngine*       newRtAudio(const AudioApi api);
    static unsigned int       getRtAudioApiCount();
    static const char*        getRtAudioApiName(const unsigned int index);
    static const char* const* getRtAudioApiDeviceNames(const unsigned int index);
    static const EngineDriverDeviceInfo* getRtAudioDeviceInfo(const unsigned int index, const char* const deviceName);

    // Juce
    static CarlaEngine*       newJuce(const AudioApi api);
    static unsigned int       getJuceApiCount();
    static const char*        getJuceApiName(const unsigned int index);
    static const char* const* getJuceApiDeviceNames(const unsigned int index);
    static const EngineDriverDeviceInfo* getJuceDeviceInfo(const unsigned int index, const char* const deviceName);

#ifdef BUILD_BRIDGE
public:
    // Bridge
    static CarlaEngine* newBridge(const char* const audioBaseName, const char* const controlBaseName);

    // -------------------------------------------------------------------
    // Bridge/Controller OSC stuff

    void oscSend_bridge_plugin_info1(const PluginCategory category, const uint hints, const long uniqueId) const noexcept;
    void oscSend_bridge_plugin_info2(const char* const realName, const char* const label, const char* const maker, const char* const copyright) const noexcept;
    void oscSend_bridge_audio_count(const uint32_t ins, const uint32_t outs) const noexcept;
    void oscSend_bridge_midi_count(const uint32_t ins, const uint32_t outs) const noexcept;
    void oscSend_bridge_parameter_count(const uint32_t ins, const uint32_t outs) const noexcept;
    void oscSend_bridge_program_count(const uint32_t count) const noexcept;
    void oscSend_bridge_midi_program_count(const uint32_t count) const noexcept;
    void oscSend_bridge_parameter_data(const uint32_t index, const int32_t rindex, const ParameterType type, const uint hints, const char* const name, const char* const unit) const noexcept;
    void oscSend_bridge_parameter_ranges1(const uint32_t index, const float def, const float min, const float max) const noexcept;
    void oscSend_bridge_parameter_ranges2(const uint32_t index, const float step, const float stepSmall, const float stepLarge) const noexcept;
    void oscSend_bridge_parameter_midi_cc(const uint32_t index, const int16_t cc) const noexcept;
    void oscSend_bridge_parameter_midi_channel(const uint32_t index, const uint8_t channel) const noexcept;
    void oscSend_bridge_parameter_value(const int32_t index, const float value) const noexcept; // may be used for internal params (< 0)
    void oscSend_bridge_default_value(const uint32_t index, const float value) const noexcept;
    void oscSend_bridge_current_program(const int32_t index) const noexcept;
    void oscSend_bridge_current_midi_program(const int32_t index) const noexcept;
    void oscSend_bridge_program_name(const uint32_t index, const char* const name) const noexcept;
    void oscSend_bridge_midi_program_data(const uint32_t index, const uint32_t bank, const uint32_t program, const char* const name) const noexcept;
    void oscSend_bridge_configure(const char* const key, const char* const value) const noexcept;
    void oscSend_bridge_set_custom_data(const char* const type, const char* const key, const char* const value) const noexcept;
    void oscSend_bridge_set_chunk_data(const char* const chunkFile) const noexcept;
    void oscSend_bridge_set_peaks() const noexcept;
#else
public:
    void oscSend_control_add_plugin_start(const uint pluginId, const char* const pluginName) const noexcept;
    void oscSend_control_add_plugin_end(const uint pluginId) const noexcept;
    void oscSend_control_remove_plugin(const uint pluginId) const noexcept;
    void oscSend_control_set_plugin_info1(const uint pluginId, const PluginType type, const PluginCategory category, const uint hints, const long uniqueId) const noexcept;
    void oscSend_control_set_plugin_info2(const uint pluginId, const char* const realName, const char* const label, const char* const maker, const char* const copyright) const noexcept;
    void oscSend_control_set_audio_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept;
    void oscSend_control_set_midi_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept;
    void oscSend_control_set_parameter_count(const uint pluginId, const uint32_t ins, const uint32_t outs) const noexcept;
    void oscSend_control_set_program_count(const uint pluginId, const uint32_t count) const noexcept;
    void oscSend_control_set_midi_program_count(const uint pluginId, const uint32_t count) const noexcept;
    void oscSend_control_set_parameter_data(const uint pluginId, const uint32_t index, const ParameterType type, const uint hints, const char* const name, const char* const unit) const noexcept;
    void oscSend_control_set_parameter_ranges1(const uint pluginId, const uint32_t index, const float def, const float min, const float max) const noexcept;
    void oscSend_control_set_parameter_ranges2(const uint pluginId, const uint32_t index, const float step, const float stepSmall, const float stepLarge) const noexcept;
    void oscSend_control_set_parameter_midi_cc(const uint pluginId, const uint32_t index, const int16_t cc) const noexcept;
    void oscSend_control_set_parameter_midi_channel(const uint pluginId, const uint32_t index, const uint8_t channel) const noexcept;
    void oscSend_control_set_parameter_value(const uint pluginId, const int32_t index, const float value) const noexcept; // may be used for internal params (< 0)
    void oscSend_control_set_default_value(const uint pluginId, const uint32_t index, const float value) const noexcept;
    void oscSend_control_set_current_program(const uint pluginId, const int32_t index) const noexcept;
    void oscSend_control_set_current_midi_program(const uint pluginId, const int32_t index) const noexcept;
    void oscSend_control_set_program_name(const uint pluginId, const uint32_t index, const char* const name) const noexcept;
    void oscSend_control_set_midi_program_data(const uint pluginId, const uint32_t index, const uint32_t bank, const uint32_t program, const char* const name) const noexcept;
    void oscSend_control_note_on(const uint pluginId, const uint8_t channel, const uint8_t note, const uint8_t velo) const noexcept;
    void oscSend_control_note_off(const uint pluginId, const uint8_t channel, const uint8_t note) const noexcept;
    void oscSend_control_set_peaks(const uint pluginId) const noexcept;
    void oscSend_control_exit() const noexcept;
#endif

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngine)
};

/**@}*/

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_HPP_INCLUDED
