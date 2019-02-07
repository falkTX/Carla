/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

namespace water {
class MemoryOutputStream;
class XmlDocument;
}

CARLA_BACKEND_START_NAMESPACE

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
     * JACK engine type.
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
     * @see CarlaEngineAudioPort
     */
    kEnginePortTypeAudio = 1,

    /*!
     * CV port type.
     * @see CarlaEngineCVPort
     */
    kEnginePortTypeCV = 2,

    /*!
     * Event port type (Control or MIDI).
     * @see CarlaEngineEventPort
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
     * @see EngineControlEvent
     */
    kEngineEventTypeControl = 1,

    /*!
     * MIDI event type.
     * @see EngineMidiEvent
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
     * Parameter event type.
     * @note Value uses a normalized range of 0.0f<->1.0f.
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
struct CARLA_API EngineControlEvent {
    EngineControlEventType type; //!< Control-Event type.
    uint16_t param;              //!< Parameter Id, midi bank or midi program.
    float    value;              //!< Parameter value, normalized to 0.0f<->1.0f.

    /*!
     * Convert this control event into MIDI data.
     * Returns size.
     */
    uint8_t convertToMidiData(const uint8_t channel, uint8_t data[3]) const noexcept;
};

/*!
 * Engine MIDI event.
 */
struct CARLA_API EngineMidiEvent {
    static const uint8_t kDataSize = 4; //!< Size of internal data

    uint8_t port; //!< Port offset (usually 0)
    uint8_t size; //!< Number of bytes used

    /*!
     * MIDI data, without channel bit.
     * If size > kDataSize, dataExt is used (otherwise NULL).
     */
    uint8_t        data[kDataSize];
    const uint8_t* dataExt;
};

/*!
 * Engine event.
 */
struct CARLA_API EngineEvent {
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
     * Fill this event from MIDI data.
     */
    void fillFromMidiData(const uint8_t size, const uint8_t* const data, const uint8_t midiPortOffset) noexcept;
};

// -----------------------------------------------------------------------

/*!
 * Engine options.
 */
struct CARLA_API EngineOptions {
    EngineProcessMode   processMode;
    EngineTransportMode transportMode;
    const char*         transportExtra;

    bool forceStereo;
    bool preferPluginBridges;
    bool preferUiBridges;
    bool uisAlwaysOnTop;

    uint maxParameters;
    uint uiBridgesTimeout;
    uint audioBufferSize;
    uint audioSampleRate;
    bool audioTripleBuffer;
    const char* audioDevice;

    const char* pathLADSPA;
    const char* pathDSSI;
    const char* pathLV2;
    const char* pathVST2;
    const char* pathVST3;
    const char* pathSF2;
    const char* pathSFZ;

    const char* binaryDir;
    const char* resourceDir;

    bool preventBadBehaviour;
    uintptr_t frontendWinId;

#ifndef CARLA_OS_WIN
    struct Wine {
        const char* executable;

        bool autoPrefix;
        const char* fallbackPrefix;

        bool rtPrio;
        int baseRtPrio;
        int serverRtPrio;

        Wine() noexcept;
        ~Wine() noexcept;
        CARLA_DECLARE_NON_COPY_STRUCT(Wine)
    } wine;
#endif

#ifndef DOXYGEN
    EngineOptions() noexcept;
    ~EngineOptions() noexcept;
    CARLA_DECLARE_NON_COPY_STRUCT(EngineOptions)
#endif
};

/*!
 * Engine BBT Time information.
 */
struct CARLA_API EngineTimeInfoBBT {
    bool valid;

    int32_t bar;  //!< current bar
    int32_t beat; //!< current beat-within-bar
    double  tick; //!< current tick-within-beat
    double  barStartTick;

    float beatsPerBar; //!< time signature "numerator"
    float beatType;    //!< time signature "denominator"

    double ticksPerBeat;
    double beatsPerMinute;

    /*!
     * Clear.
     */
    void clear() noexcept;

#ifndef DOXYGEN
    EngineTimeInfoBBT() noexcept;
    EngineTimeInfoBBT(const EngineTimeInfoBBT&) noexcept;
#endif
};

/*!
 * Engine Time information.
 */
struct CARLA_API EngineTimeInfo {
    bool playing;
    uint64_t frame;
    uint64_t usecs;
    EngineTimeInfoBBT bbt;

    /*!
     * Clear.
     */
    void clear() noexcept;

#ifndef DOXYGEN
    EngineTimeInfo() noexcept;
    EngineTimeInfo(const EngineTimeInfo&) noexcept;
    EngineTimeInfo& operator=(const EngineTimeInfo&) noexcept;

    // fast comparison, doesn't check all values
    bool compareIgnoringRollingFrames(const EngineTimeInfo& timeInfo, const uint32_t maxFrames) const noexcept;

    // quick operator, doesn't check all values
    bool operator==(const EngineTimeInfo& timeInfo) const noexcept;
    bool operator!=(const EngineTimeInfo& timeInfo) const noexcept;
#endif
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine port (Abstract).
 * This is the base class for all Carla Engine ports.
 */
class CARLA_API CarlaEnginePort
{
protected:
    /*!
     * The constructor.
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEnginePort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept;

public:
    /*!
     * The destructor.
     */
    virtual ~CarlaEnginePort() noexcept;

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
        return kIsInput;
    }

    /*!
     * Get this ports' engine client.
     */
    const CarlaEngineClient& getEngineClient() const noexcept
    {
        return kClient;
    }

#ifndef DOXYGEN
protected:
    const CarlaEngineClient& kClient;
    const bool     kIsInput;
    const uint32_t kIndexOffset;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEnginePort)
#endif
};

/*!
 * Carla Engine Audio port.
 */
class CARLA_API CarlaEngineAudioPort : public CarlaEnginePort
{
public:
    /*!
     * The constructor.
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineAudioPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept;

    /*!
     * The destructor.
     */
    ~CarlaEngineAudioPort() noexcept override;

    /*!
     * Get the type of the port, in this case kEnginePortTypeAudio.
     */
    EnginePortType getType() const noexcept final
    {
        return kEnginePortTypeAudio;
    }

    /*!
     * Initialize the port's internal buffer.
     */
    void initBuffer() noexcept override;

    /*!
     * Direct access to the port's audio buffer.
     * May be null.
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
class CARLA_API CarlaEngineCVPort : public CarlaEnginePort
{
public:
    /*!
     * The constructor.
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineCVPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept;

    /*!
     * The destructor.
     */
    ~CarlaEngineCVPort() noexcept override;

    /*!
     * Get the type of the port, in this case kEnginePortTypeCV.
     */
    EnginePortType getType() const noexcept final
    {
        return kEnginePortTypeCV;
    }

    /*!
     * Initialize the port's internal buffer.
     */
    void initBuffer() noexcept override;

    /*!
     * Direct access to the port's CV buffer.
     * May be null.
     */
    float* getBuffer() const noexcept
    {
        return fBuffer;
    }

#ifndef DOXYGEN
protected:
    float* fBuffer;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineCVPort)
#endif
};

/*!
 * Carla Engine Event port.
 */
class CARLA_API CarlaEngineEventPort : public CarlaEnginePort
{
public:
    /*!
     * The constructor.
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineEventPort(const CarlaEngineClient& client, const bool isInputPort, const uint32_t indexOffset) noexcept;

    /*!
     * The destructor.
     */
    ~CarlaEngineEventPort() noexcept override;

    /*!
     * Get the type of the port, in this case kEnginePortTypeEvent.
     */
    EnginePortType getType() const noexcept final
    {
        return kEnginePortTypeEvent;
    }

    /*!
     * Initialize the port's internal buffer for @a engine.
     */
    void initBuffer() noexcept override;

    /*!
     * Get the number of events present in the buffer.
     * @note You must only call this for input ports.
     */
    virtual uint32_t getEventCount() const noexcept;

    /*!
     * Get the event at @a index.
     * @note You must only call this for input ports.
     */
    virtual const EngineEvent& getEvent(const uint32_t index) const noexcept;

    /*!
     * Get the event at @a index, faster unchecked version.
     */
    virtual const EngineEvent& getEventUnchecked(const uint32_t index) const noexcept;

    /*!
     * Write a control event into the buffer.
     * @note You must only call this for output ports.
     */
    bool writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEvent& ctrl) noexcept;

    /*!
     * Write a control event into the buffer.
     * Arguments are the same as in the EngineControlEvent struct.
     * @note You must only call this for output ports.
     */
    virtual bool writeControlEvent(const uint32_t time, const uint8_t channel, const EngineControlEventType type, const uint16_t param, const float value = 0.0f) noexcept;

    /*!
     * Write a MIDI event into the buffer.
     * @note You must only call this for output ports.
     */
    bool writeMidiEvent(const uint32_t time, const uint8_t size, const uint8_t* const data) noexcept;

    /*!
     * Write a MIDI event into the buffer.
     * @note You must only call this for output ports.
     */
    bool writeMidiEvent(const uint32_t time, const uint8_t channel, const EngineMidiEvent& midi) noexcept;

    /*!
     * Write a MIDI event into the buffer.
     * Arguments are the same as in the EngineMidiEvent struct.
     * @note You must only call this for output ports.
     */
    virtual bool writeMidiEvent(const uint32_t time, const uint8_t channel, const uint8_t size, const uint8_t* const data) noexcept;

#ifndef DOXYGEN
protected:
    EngineEvent* fBuffer;
    const EngineProcessMode kProcessMode;
    friend class CarlaPluginInstance;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineEventPort)
#endif
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine client.
 * Each plugin requires one client from the engine (created via CarlaEngine::addClient()).
 * @note This is a virtual class, some engine types provide custom funtionality.
 */
class CARLA_API CarlaEngineClient
{
public:
    /*!
     * The constructor, protected.
     * All constructor parameters are constant and will never change in the lifetime of the client.
     * Client starts in deactivated state.
     */
    CarlaEngineClient(const CarlaEngine& engine);

    /*!
     * The destructor.
     */
    virtual ~CarlaEngineClient() noexcept;

    /*!
     * Activate this client.
     * Client must be deactivated before calling this function.
     */
    virtual void activate() noexcept;

    /*!
     * Deactivate this client.
     * Client must be activated before calling this function.
     */
    virtual void deactivate() noexcept;

    /*!
     * Check if the client is activated.
     */
    virtual bool isActive() const noexcept;

    /*!
     * Check if the client is ok.
     * Plugins will refuse to instantiate if this returns false.
     * @note This is always true in rack and patchbay processing modes.
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
     * Add a new port of type @a portType.
     * @note This function does nothing in rack processing mode since ports are static there.
     */
    virtual CarlaEnginePort* addPort(const EnginePortType portType, const char* const name, const bool isInput, const uint32_t indexOffset);

    /*!
     * Get this client's engine.
     */
    const CarlaEngine& getEngine() const noexcept;

    /*!
     * Get the engine's process mode.
     */
    EngineProcessMode getProcessMode() const noexcept;

    /*!
     * Get an audio port name.
     */
    const char* getAudioPortName(const bool isInput, const uint index) const noexcept;

    /*!
     * Get a CV port name.
     */
    const char* getCVPortName(const bool isInput, const uint index) const noexcept;

    /*!
     * Get an event port name.
     */
    const char* getEventPortName(const bool isInput, const uint index) const noexcept;

#ifndef DOXYGEN
protected:
    /*!
     * Internal data, for CarlaEngineClient subclasses only.
     */
    struct ProtectedData;
    ProtectedData* const pData;

    void _addAudioPortName(const bool, const char* const);
    void _addCVPortName(const bool, const char* const);
    void _addEventPortName(const bool, const char* const);
    const char* _getUniquePortName(const char* const);
    void _clearPorts();

    CARLA_DECLARE_NON_COPY_CLASS(CarlaEngineClient)
#endif
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine.
 * @note This is a virtual class for all available engine types available in Carla.
 */
class CARLA_API CarlaEngine
{
protected:
    /*!
     * The constructor, protected.
     * @note This only initializes engine data, it doesn't actually start the engine.
     */
    CarlaEngine();

public:
    /*!
     * The destructor.
     * The engine must have been closed before this happens.
     */
    virtual ~CarlaEngine();

    // -------------------------------------------------------------------
    // Static calls

    /*!
     * Get the number of available engine drivers.
     */
    static uint getDriverCount();

    /*!
     * Get the name of the engine driver at @a index.
     */
    static const char* getDriverName(const uint index);

    /*!
     * Get the device names of the driver at @a index.
     */
    static const char* const* getDriverDeviceNames(const uint index);

    /*!
     * Get device information about the driver at @a index and name @a driverName.
     */
    static const EngineDriverDeviceInfo* getDriverDeviceInfo(const uint index, const char* const driverName);

    /*!
     * Create a new engine, using driver @a driverName.
     * Returned value must be deleted when no longer needed.
     * @note This only initializes engine data, it doesn't actually start the engine.
     */
    static CarlaEngine* newDriverByName(const char* const driverName);

    // -------------------------------------------------------------------
    // Constant values

    /*!
     * Maximum client name size.
     */
    virtual uint getMaxClientNameSize() const noexcept;

    /*!
     * Maximum port name size.
     */
    virtual uint getMaxPortNameSize() const noexcept;

    /*!
     * Current number of plugins loaded.
     */
    uint getCurrentPluginCount() const noexcept;

    /*!
     * Maximum number of loadable plugins allowed.
     * This function returns 0 if engine is not started.
     */
    uint getMaxPluginNumber() const noexcept;

    // -------------------------------------------------------------------
    // Virtual, per-engine type calls

    /*!
     * Initialize/start the engine, using @a clientName.
     * When the engine is intialized, you need to call idle() at regular intervals.
     */
    virtual bool init(const char* const clientName) = 0;

    /*!
     * Close engine.
     * This function always closes the engine even if it returns false.
     * In other words, even when something goes wrong when closing the engine it still be closed nonetheless.
     */
    virtual bool close();

    /*!
     * Idle engine.
     */
    virtual void idle() noexcept;

    /*!
     * Check if engine is running.
     */
    virtual bool isRunning() const noexcept = 0;

    /*!
     * Check if engine is running offline (aka freewheel mode).
     */
    virtual bool isOffline() const noexcept = 0;

    /*!
     * Check if engine runs on a constant buffer size value.
     * Default implementation returns true.
     */
    virtual bool usesConstantBufferSize() const noexcept;

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
     * @note This function must only be called within a plugin class.
     */
    virtual CarlaEngineClient* addClient(CarlaPlugin* const plugin);

    // -------------------------------------------------------------------
    // Plugin management

    /*!
     * Add new plugin.
     * @see ENGINE_CALLBACK_PLUGIN_ADDED
     */
    bool addPlugin(const BinaryType btype, const PluginType ptype,
                   const char* const filename, const char* const name, const char* const label, const int64_t uniqueId,
                   const void* const extra, const uint options);

    /*!
     * Add new plugin, using native binary type and default options.
     * @see ENGINE_CALLBACK_PLUGIN_ADDED
     */
    bool addPlugin(const PluginType ptype,
                   const char* const filename, const char* const name, const char* const label, const int64_t uniqueId,
                   const void* const extra);

    /*!
     * Remove plugin with id @a id.
     * @see ENGINE_CALLBACK_PLUGIN_REMOVED
     */
    bool removePlugin(const uint id);

    /*!
     * Remove all plugins.
     */
    bool removeAllPlugins();

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * Rename plugin with id @a id to @a newName.
     * Returns the new name, or null if the operation failed.
     * Returned variable must be deleted if non-null.
     * @see ENGINE_CALLBACK_PLUGIN_RENAMED
     */
    virtual const char* renamePlugin(const uint id, const char* const newName);

    /*!
     * Clone plugin with id @a id.
     */
    bool clonePlugin(const uint id);

    /*!
     * Prepare replace of plugin with id @a id.
     * The next call to addPlugin() will use this id, replacing the selected plugin.
     * @note This function requires addPlugin() to be called afterwards, as soon as possible.
     */
    bool replacePlugin(const uint id) noexcept;

    /*!
     * Switch plugins with id @a idA and @a idB.
     */
    bool switchPlugins(const uint idA, const uint idB) noexcept;
#endif

    /*!
     * Get plugin with id @a id.
     */
    CarlaPlugin* getPlugin(const uint id) const noexcept;

    /*!
     * Get plugin with id @a id, faster unchecked version.
     */
    CarlaPlugin* getPluginUnchecked(const uint id) const noexcept;

    /*!
     * Get a unique plugin name within the engine.
     * Returned variable must be deleted if non-null.
     */
    const char* getUniquePluginName(const char* const name) const;

    // -------------------------------------------------------------------
    // Project management

    /*!
     * Load a file of any type.
     * This will try to load a generic file as a plugin,
     * either by direct handling (SF2 and SFZ) or by using an internal plugin (like Audio and MIDI).
     */
    bool loadFile(const char* const filename);

    /*!
     * Load a project file.
     * @note Already loaded plugins are not removed; call removeAllPlugins() first if needed.
     */
    bool loadProject(const char* const filename, const bool setAsCurrentProject);

    /*!
     * Save current project to a file.
     */
    bool saveProject(const char* const filename, const bool setAsCurrentProject);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * Get the currently set project filename.
     */
    const char* getCurrentProjectFilename() const noexcept;

    /*!
     * Clear the currently set project filename.
     */
    void clearCurrentProjectFilename() noexcept;
#endif

    // -------------------------------------------------------------------
    // Information (base)

    /*!
     * Get the current engine driver hints.
     * @see EngineDriverHints
     */
    uint getHints() const noexcept;

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
    virtual EngineTimeInfo getTimeInfo() const noexcept;

    // -------------------------------------------------------------------
    // Information (peaks)

    /*!
     * Get a plugin's peak values.
     */
    float* getPeaks(const uint pluginId) const noexcept;

    /*!
     * Get a plugin's input peak value.
     */
    float getInputPeak(const uint pluginId, const bool isLeft) const noexcept;

    /*!
     * Get a plugin's output peak value.
     */
    float getOutputPeak(const uint pluginId, const bool isLeft) const noexcept;

    // -------------------------------------------------------------------
    // Callback

    /*!
     * Call the main engine callback, if set.
     * May be called by plugins.
     */
    virtual void callback(const EngineCallbackOpcode action, const uint pluginId, const int value1, const int value2, const float value3, const char* const valueStr) noexcept;

    /*!
     * Set the main engine callback to @a func.
     */
    void setCallback(const EngineCallbackFunc func, void* const ptr) noexcept;

    // -------------------------------------------------------------------
    // Callback

    /*!
     * Call the file callback, if set.
     * May be called by plugins.
     */
    const char* runFileCallback(const FileCallbackOpcode action, const bool isDir, const char* const title, const char* const filter) noexcept;

    /*!
     * Set the file callback to @a func.
     */
    void setFileCallback(const FileCallbackFunc func, void* const ptr) noexcept;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // -------------------------------------------------------------------
    // Patchbay

    /*!
     * Connect two patchbay ports.
     */
    virtual bool patchbayConnect(const uint groupA, const uint portA, const uint groupB, const uint portB);

    /*!
     * Remove a patchbay connection.
     */
    virtual bool patchbayDisconnect(const uint connectionId);

    /*!
     * Force the engine to resend all patchbay clients, ports and connections again.
     */
    virtual bool patchbayRefresh(const bool external);
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
     * Set the engine transport bpm to @a bpm.
     */
    virtual void transportBPM(const double bpm) noexcept;

    /*!
     * Relocate the engine transport to @a frames.
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
    void setLastError(const char* const error) const noexcept;

    // -------------------------------------------------------------------
    // Misc

    /*!
     * Check if the engine is about to close.
     */
    bool isAboutToClose() const noexcept;

    /*!
     * Tell the engine it's about to close.
     * This is used to prevent the engine thread(s) from reactivating.
     * Returns true if there's no pending engine events.
     */
    bool setAboutToClose() noexcept;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * TODO.
     */
    bool isLoadingProject() const noexcept;
#endif

    /*!
     * Tell the engine to stop the current cancelable action.
     * @see ENGINE_CALLBACK_CANCELABLE_ACTION
     */
    void setActionCanceled(const bool canceled) noexcept;

    /*!
     * Check wherever the last cancelable action was indeed canceled or not.
     */
    bool wasActionCanceled() const noexcept;

    // -------------------------------------------------------------------
    // Options

    /*!
     * Set the engine option @a option to @a value or @a valueStr.
     */
    virtual void setOption(const EngineOption option, const int value, const char* const valueStr) noexcept;

    // -------------------------------------------------------------------
    // OSC Stuff

#ifndef BUILD_BRIDGE
    /*!
     * Check if OSC controller is registered.
     */
    bool isOscControlRegistered() const noexcept;

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
#endif

    // -------------------------------------------------------------------
    // Helper functions

    /*!
     * Return internal data, needed for EventPorts when used in Rack, Patchbay and Bridge modes.
     * @note RT call
     */
    EngineEvent* getInternalEventBuffer(const bool isInput) const noexcept;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * Virtual functions for handling external graph ports.
     */
    virtual bool connectExternalGraphPort(const uint, const uint, const char* const);
    virtual bool disconnectExternalGraphPort(const uint, const uint, const char* const);
#endif

    // -------------------------------------------------------------------

protected:
    /*!
     * Internal data, for CarlaEngine subclasses only.
     */
    struct ProtectedData;
    ProtectedData* const pData;

    /*!
     * Some internal classes read directly from pData or call protected functions.
     */
    friend class CarlaPluginInstance;
    friend class EngineInternalGraph;
    friend class PendingRtEventsRunner;
    friend class ScopedActionLock;
    friend class ScopedEngineEnvironmentLocker;
    friend class ScopedThreadStopper;
    friend class PatchbayGraph;
    friend struct RackGraph;

    // -------------------------------------------------------------------
    // Internal stuff

    /*!
     * Report to all plugins about buffer size change.
     */
    void bufferSizeChanged(const uint32_t newBufferSize);

    /*!
     * Report to all plugins about sample rate change.
     * This is not supported on all plugin types, in which case they will have to be re-initiated.
     */
    void sampleRateChanged(const double newSampleRate);

    /*!
     * Report to all plugins about offline mode change.
     */
    void offlineModeChanged(const bool isOffline);

    /*!
     * Set a plugin (stereo) peak values.
     * @note RT call
     */
    void setPluginPeaks(const uint pluginId, float const inPeaks[2], float const outPeaks[2]) noexcept;

    /*!
     * Common save project function for main engine and plugin.
     */
    void saveProjectInternal(water::MemoryOutputStream& outStrm) const;

    /*!
     * Common load project function for main engine and plugin.
     */
    bool loadProjectInternal(water::XmlDocument& xmlDoc);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    // -------------------------------------------------------------------
    // Patchbay stuff

    /*!
     * Virtual functions for handling patchbay state.
     * Do not free returned data.
     */
    virtual const char* const* getPatchbayConnections(const bool external) const;
    virtual void restorePatchbayConnection(const bool external, const char* const sourcePort, const char* const targetPort, const bool sendCallback);
#endif

    // -------------------------------------------------------------------

public:
    /*!
     * Native audio APIs.
     */
    enum AudioApi {
        AUDIO_API_NULL,
        // common
        AUDIO_API_JACK,
        AUDIO_API_OSS,
        // linux
        AUDIO_API_ALSA,
        AUDIO_API_PULSEAUDIO,
        // macos
        AUDIO_API_COREAUDIO,
        // windows
        AUDIO_API_ASIO,
        AUDIO_API_DIRECTSOUND,
        AUDIO_API_WASAPI
    };

    // -------------------------------------------------------------------
    // Engine initializers

    // JACK
    static CarlaEngine* newJack();

#ifdef BUILD_BRIDGE
    // Bridge
    static CarlaEngine* newBridge(const char* const audioPoolBaseName,
                                  const char* const rtClientBaseName,
                                  const char* const nonRtClientBaseName,
                                  const char* const nonRtServerBaseName);
#else
# ifdef USING_JUCE
    // Juce
    static CarlaEngine*       newJuce(const AudioApi api);
    static uint               getJuceApiCount();
    static const char*        getJuceApiName(const uint index);
    static const char* const* getJuceApiDeviceNames(const uint index);
    static const EngineDriverDeviceInfo* getJuceDeviceInfo(const uint index, const char* const deviceName);
# else
    // RtAudio
    static CarlaEngine*       newRtAudio(const AudioApi api);
    static uint               getRtAudioApiCount();
    static const char*        getRtAudioApiName(const uint index);
    static const char* const* getRtAudioApiDeviceNames(const uint index);
    static const EngineDriverDeviceInfo* getRtAudioDeviceInfo(const uint index, const char* const deviceName);
# endif
#endif

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // OSC Controller stuff

    void oscSend_control_add_plugin_start(const uint pluginId, const char* const pluginName) const noexcept;
    void oscSend_control_add_plugin_end(const uint pluginId) const noexcept;
    void oscSend_control_remove_plugin(const uint pluginId) const noexcept;
    void oscSend_control_set_plugin_info1(const uint pluginId, const PluginType type, const PluginCategory category, const uint hints, const int64_t uniqueId) const noexcept;
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
