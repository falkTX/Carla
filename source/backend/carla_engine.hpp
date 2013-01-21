/*
 * Carla Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __CARLA_ENGINE_HPP__
#define __CARLA_ENGINE_HPP__

#include "carla_backend.hpp"
#include "carla_juce_utils.hpp"

#ifndef BUILD_BRIDGE
class QProcessEnvironment;
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

/*!
 * @defgroup CarlaBackendEngine Carla Backend Engine
 *
 * The Carla Backend Engine
 * @{
 */

/*!
 * The type of an engine.
 */
enum CarlaEngineType {
    /*!
    * Null engine type.
    */
    CarlaEngineTypeNull = 0,

    /*!
    * JACK engine type.\n
    * Provides all processing modes.
    */
    CarlaEngineTypeJack = 1,

    /*!
    * RtAudio engine type, used to provide Native Audio and MIDI support.
    */
    CarlaEngineTypeRtAudio = 2,

    /*!
    * Plugin engine type, used to export the engine as a plugin (DSSI, LV2 and VST) via the DISTRHO Plugin Toolkit.
    */
    CarlaEngineTypePlugin = 3
};

/*!
 * The type of an engine port.
 */
enum CarlaEnginePortType {
    /*!
    * Null engine port type.
    */
    CarlaEnginePortTypeNull = 0,

    /*!
    * Audio port.
    */
    CarlaEnginePortTypeAudio = 1,

    /*!
    * Control port.\n
    * These are MIDI ports on some engine types, by handling MIDI-CC as control.
    */
    CarlaEnginePortTypeControl = 2,

    /*!
    * MIDI port.
    */
    CarlaEnginePortTypeMIDI = 3
};

/*!
 * The type of an engine control event.
 */
enum CarlaEngineControlEventType {
    /*!
    * Null control event type.
    */
    CarlaEngineControlEventTypeNull = 0,

    /*!
    * Parameter change event.\n
    * \note Value uses a range of 0.0<->1.0.
    */
    CarlaEngineControlEventTypeParameterChange = 1,

    /*!
    * MIDI Bank change event.
    */
    CarlaEngineControlEventTypeMidiBankChange = 2,

    /*!
    * MIDI Program change event.
    */
    CarlaEngineControlEventTypeMidiProgramChange = 3,

    /*!
    * All sound off event.
    */
    CarlaEngineControlEventTypeAllSoundOff = 4,

    /*!
    * All notes off event.
    */
    CarlaEngineControlEventTypeAllNotesOff = 5
};

/*!
 * Engine control event.
 */
struct CarlaEngineControlEvent {
    CarlaEngineControlEventType type;
    uint32_t time;      //!< frame offset
    uint8_t  channel;   //!< channel, used for MIDI-related events and ports
    uint16_t parameter; //!< parameter, used for parameter changes only
    double   value;

    CarlaEngineControlEvent()
    {
        clear();
    }

    void clear()
    {
        type = CarlaEngineControlEventTypeNull;
        time = 0;
        channel = 0;
        parameter = 0;
        value = 0.0;
    }
};

/*!
 * Engine MIDI event.
 */
struct CarlaEngineMidiEvent {
    uint32_t time;
    uint8_t  size;
    uint8_t  data[3];

    CarlaEngineMidiEvent()
    {
        clear();
    }

    void clear()
    {
        time = 0;
        size = 0;
        data[0] = data[1] = data[2] = 0;
    }
};

/*!
 * Engine options.
 */
struct CarlaEngineOptions {
    ProcessMode processMode;
    bool processHighPrecision;

    bool forceStereo;
    bool preferPluginBridges;
    bool preferUiBridges;
#ifdef WANT_DSSI
    bool useDssiVstChunks;
#endif

    uint maxParameters;
    uint oscUiTimeout;
    uint preferredBufferSize;
    uint preferredSampleRate;

#ifndef BUILD_BRIDGE
    CarlaString bridge_native;
    CarlaString bridge_posix32;
    CarlaString bridge_posix64;
    CarlaString bridge_win32;
    CarlaString bridge_win64;
#endif
#ifdef WANT_LV2
    CarlaString bridge_lv2gtk2;
    CarlaString bridge_lv2gtk3;
    CarlaString bridge_lv2qt4;
    CarlaString bridge_lv2qt5;
    CarlaString bridge_lv2cocoa;
    CarlaString bridge_lv2win;
    CarlaString bridge_lv2x11;
#endif
#ifdef WANT_VST
    CarlaString bridge_vstcocoa;
    CarlaString bridge_vsthwnd;
    CarlaString bridge_vstx11;
#endif

    CarlaEngineOptions()
        : processMode(PROCESS_MODE_CONTINUOUS_RACK),
          processHighPrecision(false),
          forceStereo(false),
          preferPluginBridges(false),
          preferUiBridges(true),
      #ifdef WANT_DSSI
          useDssiVstChunks(false),
      #endif
          maxParameters(MAX_DEFAULT_PARAMETERS),
          oscUiTimeout(4000),
          preferredBufferSize(512),
          preferredSampleRate(44100) {}
};

/*!
 * Engine BBT Time information.
 */
struct CarlaEngineTimeInfoBBT {
    int32_t bar;  //!< current bar
    int32_t beat; //!< current beat-within-bar
    int32_t tick; //!< current tick-within-beat
    double barStartTick;

    float beatsPerBar; //!< time signature "numerator"
    float beatType;    //!< time signature "denominator"

    double ticksPerBeat;
    double beatsPerMinute;

    CarlaEngineTimeInfoBBT()
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
struct CarlaEngineTimeInfo {
    static const uint32_t ValidBBT = 0x1;

    bool playing;
    uint32_t frame;
    uint32_t time;
    uint32_t valid;
    CarlaEngineTimeInfoBBT bbt;

    CarlaEngineTimeInfo()
        : playing(false),
          frame(0),
          time(0),
          valid(0x0) {}
};

// -----------------------------------------------------------------------

/*!
 * Engine port (Abstract).\n
 * This is the base class for all Carla Engine ports.
 */
class CarlaEnginePort
{
public:
    /*!
     * The contructor.\n
     * Param \a isInput defines wherever this is an input port or not (output otherwise).\n
     * Input/output state and process mode are constant for the lifetime of the port.
     */
    CarlaEnginePort(const bool isInput, const ProcessMode processMode);

    /*!
     * The decontructor.
     */
    virtual ~CarlaEnginePort();

    /*!
     * Get the type of the port, as provided by the respective subclasses.
     */
    virtual CarlaEnginePortType type() const = 0;

    /*!
     * Initialize the port's internal buffer for \a engine.
     */
    virtual void initBuffer(CarlaEngine* const engine) = 0;

protected:
    const bool isInput;
    const ProcessMode processMode;
    void* buffer;

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEnginePort)
};

// -----------------------------------------------------------------------

/*!
 * Engine Audio port.
 */
class CarlaEngineAudioPort : public CarlaEnginePort
{
public:
    /*!
     * The contructor.\n
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineAudioPort(const bool isInput, const ProcessMode processMode);

    /*!
     * The decontructor.
     */
    virtual ~CarlaEngineAudioPort();

    /*!
     * Get the type of the port, in this case CarlaEnginePortTypeAudio.
     */
    CarlaEnginePortType type() const
    {
        return CarlaEnginePortTypeAudio;
    }

    /*!
     * Initialize the port's internal buffer for \a engine.
     */
    virtual void initBuffer(CarlaEngine* const engine);

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineAudioPort)
};

// -----------------------------------------------------------------------

/*!
 * Engine Control port.
 */
class CarlaEngineControlPort : public CarlaEnginePort
{
public:
    /*!
     * The contructor.\n
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineControlPort(const bool isInput, const ProcessMode processMode);

    /*!
     * The decontructor.
     */
    virtual ~CarlaEngineControlPort();

    /*!
     * Get the type of the port, in this case CarlaEnginePortTypeControl.
     */
    CarlaEnginePortType type() const
    {
        return CarlaEnginePortTypeControl;
    }

    /*!
     * Initialize the port's internal buffer for \a engine.
     */
    virtual void initBuffer(CarlaEngine* const engine);

    /*!
     * Get the number of control events present in the buffer.
     * \note You must only call this for input ports.
     */
    virtual uint32_t getEventCount();

    /*!
     * Get the control event at \a index.
     ** \note You must only call this for input ports.
     */
    virtual const CarlaEngineControlEvent* getEvent(const uint32_t index);

    /*!
     * Write a control event to the buffer.\n
     * Arguments are the same as in the CarlaEngineControlEvent struct.
     ** \note You must only call this for output ports.
     */
    virtual void writeEvent(const CarlaEngineControlEventType type, const uint32_t time, const uint8_t channel, const uint16_t parameter, const double value);

private:
    const uint32_t m_maxEventCount;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineControlPort)
};

// -----------------------------------------------------------------------

/*!
 * Engine MIDI port.
 */
class CarlaEngineMidiPort : public CarlaEnginePort
{
public:
    /*!
     * The contructor.\n
     * All constructor parameters are constant and will never change in the lifetime of the port.
     */
    CarlaEngineMidiPort(const bool isInput, const ProcessMode processMode);

    /*!
     * The decontructor.
     */
    virtual ~CarlaEngineMidiPort();

    /*!
     * Get the type of the port, in this case CarlaEnginePortTypeMIDI.
     */
    CarlaEnginePortType type() const
    {
        return CarlaEnginePortTypeMIDI;
    }

    /*!
     * Initialize the port's internal buffer for \a engine.
     */
    virtual void initBuffer(CarlaEngine* const engine);

    /*!
     * Get the number of MIDI events present in the buffer.
     * \note You must only call this for input ports.
     */
    virtual uint32_t getEventCount();

    /*!
     * Get the MIDI event at \a index.
     ** \note You must only call this for input ports.
     */
    virtual const CarlaEngineMidiEvent* getEvent(const uint32_t index);

    /*!
     * Write a MIDI event to the buffer.\n
     * Arguments are the same as in the CarlaEngineMidiEvent struct.
     ** \note You must only call this for output ports.
     */
    virtual void writeEvent(const uint32_t time, const uint8_t* const data, const uint8_t size);

private:
    const uint32_t m_maxEventCount;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineMidiPort)
};

// -----------------------------------------------------------------------

/*!
 * Engine client (Abstract).\n
 * Each plugin requires one client from the engine (created via CarlaEngine::addPort()).\n
 * \note This is a virtual class, each engine type provides its own funtionality.
 */
class CarlaEngineClient
{
protected:
    /*!
     * The constructor, protected.\n
     * All constructor parameters are constant and will never change in the lifetime of the client.\n
     * Client starts in deactivated state.
     */
    CarlaEngineClient(const CarlaEngineType engineType, const ProcessMode processMode);

public:
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
     * \note This function does nothing in rack processing mode since ports are static there (2 audio, 1 control and 1 midi for both input and output).
     */
    virtual const CarlaEnginePort* addPort(const CarlaEnginePortType portType, const char* const name, const bool isInput) = 0;

protected:
    const CarlaEngineType engineType;
    const ProcessMode processMode;

private:
    bool     m_active;
    uint32_t m_latency;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineClient)
};

// -----------------------------------------------------------------------

struct CarlaEnginePrivateData;

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
    // Static values and calls

    /*!
     * Get the number of available engine drivers.
     */
    static unsigned int getDriverCount();

    /*!
     * Get the name of the engine driver at \a index.
     */
    static const char* getDriverName(unsigned int index);

    /*!
     * Create a new engine, using driver \a driverName.\n
     * Returned variable must be deleted when no longer needed.
     */
    static CarlaEngine* newDriverByName(const char* const driverName);


    // -------------------------------------------------------------------
    // Maximum values

    /*!
     * Maximum client name size.
     */
    virtual int maxClientNameSize();

    /*!
     * Maximum port name size.
     */
    virtual int maxPortNameSize();

    /*!
     * Maximum number of loadable plugins.
     * \note This function returns 0 if engine is not started.
     */
    unsigned int maxPluginNumber() const;

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
     * Check if engine is running.
     */
    virtual bool isRunning() const = 0;

    /*!
     * Check if engine is running offline (aka freewheel mode).
     */
    virtual bool isOffline() const = 0;

    /*!
     * Get engine type.
     */
    virtual CarlaEngineType type() const = 0;

    /*!
     * Add new engine client.
     * \note This must only be called within a plugin class.
     */
    virtual CarlaEngineClient* addClient(CarlaPlugin* const plugin) = 0;

    // -------------------------------------------------------------------
    // Plugin management

    /*!
     * Get next available plugin id.\n
     * Returns -1 if no more plugins can be loaded.
     */
    int getNewPluginId() const;

#if 0
#if 0
    /*!
     * Get plugin with id \a id.
     */
    CarlaPlugin* getPlugin(const unsigned short id) const;

    /*!
     * Get plugin with id \a id, faster unchecked version.
     */
    CarlaPlugin* getPluginUnchecked(const unsigned short id) const;
#endif

    /*!
     * Get a unique plugin name within the engine.\n
     * Returned variable must be free'd when no longer needed.
     */
    const char* getUniquePluginName(const char* const name);

    /*!
     * Add new plugin.\n
     * Returns the id of the plugin, or -1 if the operation failed.
     */
    short addPlugin(const BinaryType btype, const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra = nullptr);

    /*!
     * Add new plugin, using native binary type.\n
     * Returns the id of the plugin, or -1 if the operation failed.
     */
    short addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra = nullptr)
    {
        return addPlugin(BINARY_NATIVE, ptype, filename, name, label, extra);
    }

    /*!
     * Remove plugin with id \a id.
     */
    bool removePlugin(const unsigned short id);

    /*!
     * Remove all plugins.
     */
    void removeAllPlugins();

    /*!
     * Idle all plugins GUIs.
     */
    void idlePluginGuis();

    // bridge, internal use only
    // TODO - find a better way for this
    void __bridgePluginRegister(const unsigned short id, CarlaPlugin* const plugin);
    //{
    //    m_carlaPlugins[id] = plugin;
    //}

    // -------------------------------------------------------------------
    // Information (base)

    /*!
     * Get engine name.
     */
    const char* getName() const
    {
        return (const char*)name;
    }

    /*!
     * Get current buffer size.
     */
    uint32_t getBufferSize() const
    {
        return bufferSize;
    }

    /*!
     * Get current sample rate.
     */
    double getSampleRate() const
    {
        return sampleRate;
    }

    /*!
     * Get the engine options (read-only).
     */
    const CarlaEngineOptions& getOptions() const
    {
        return options;
    }

    /*!
     * Get current Time information (read-only).
     */
    const CarlaEngineTimeInfo& getTimeInfo() const
    {
        return timeInfo;
    }

    /*!
     * Tell the engine it's about to close.\n
     * This is used to prevent the engine thread from reactivating.
     */
    void aboutToClose();

    // -------------------------------------------------------------------
    // Information (audio peaks)

    double getInputPeak(const unsigned short pluginId, const unsigned short id) const;
    double getOutputPeak(const unsigned short pluginId, const unsigned short id) const;
    void   setInputPeak(const unsigned short pluginId, const unsigned short id, double value);
    void   setOutputPeak(const unsigned short pluginId, const unsigned short id, double value);

    // -------------------------------------------------------------------
    // Callback

    void callback(const CallbackType action, const unsigned short pluginId, const int value1, const int value2, const double value3, const char* const valueStr);
    void setCallback(const CallbackFunc func, void* const ptr);

    // -------------------------------------------------------------------
    // Error handling

    /*!
     * Get last error.
     */
    const char* getLastError() const;

    /*!
     * Set last error.
     */
    void setLastError(const char* const error);

    // -------------------------------------------------------------------
    // Options

#ifndef BUILD_BRIDGE
    /*!
     * Get the engine options as process environment.
     */
    const QProcessEnvironment& getOptionsAsProcessEnvironment() const;

    /*!
     * Set the engine option \a option.
     */
    void setOption(const OptionsType option, const int value, const char* const valueStr);
#endif

    // -------------------------------------------------------------------
    // Mutex locks

    /*!
     * Lock processing.
     */
    void processLock();

    /*!
     * Try Lock processing.
     */
    void processTryLock();

    /*!
     * Unlock processing.
     */
    void processUnlock();

    /*!
     * Lock MIDI.
     */
    void midiLock();

    /*!
     * Try Lock MIDI.
     */
    void midiTryLock();

    /*!
     * Unlock MIDI.
     */
    void midiUnlock();

    // -------------------------------------------------------------------
    // OSC Stuff

#ifndef BUILD_BRIDGE
    /*!
     * Check if OSC controller is registered.
     */
    bool isOscControlRegistered() const;
#else
    /*!
     * Check if OSC bridge is registered.
     */
    bool isOscBridgeRegistered() const;
#endif

    /*!
     * Idle OSC.
     */
    void idleOsc();

    /*!
     * Get OSC TCP server path.
     */
    const char* getOscServerPathTCP() const;

    /*!
     * Get OSC UDP server path.
     */
    const char* getOscServerPathUDP() const;

#ifdef BUILD_BRIDGE
    /*!
     * Set OSC bridge data.
     */
    void setOscBridgeData(const CarlaOscData* const oscData);
#endif

#endif

#ifdef BUILD_BRIDGE
    void osc_send_peaks(CarlaPlugin* const plugin);
#else
    void osc_send_peaks(CarlaPlugin* const plugin, const unsigned short& id);
#endif

#ifdef BUILD_BRIDGE
    void osc_send_bridge_audio_count(const int32_t ins, const int32_t outs, const int32_t total);
    void osc_send_bridge_midi_count(const int32_t ins, const int32_t outs, const int32_t total);
    void osc_send_bridge_parameter_count(const int32_t ins, const int32_t outs, const int32_t total);
    void osc_send_bridge_program_count(const int32_t count);
    void osc_send_bridge_midi_program_count(const int32_t count);
    void osc_send_bridge_plugin_info(const int32_t category, const int32_t hints, const char* const name, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId);
    void osc_send_bridge_parameter_info(const int32_t index, const char* const name, const char* const unit);
    void osc_send_bridge_parameter_data(const int32_t index, const int32_t type, const int32_t rindex, const int32_t hints, const int32_t midiChannel, const int32_t midiCC);
    void osc_send_bridge_parameter_ranges(const int32_t index, const double def, const double min, const double max, const double step, const double stepSmall, const double stepLarge);
    void osc_send_bridge_program_info(const int32_t index, const char* const name);
    void osc_send_bridge_midi_program_info(const int32_t index, const int32_t bank, const int32_t program, const char* const label);
    void osc_send_bridge_configure(const char* const key, const char* const value);
    void osc_send_bridge_set_parameter_value(const int32_t index, const double value);
    void osc_send_bridge_set_default_value(const int32_t index, const double value);
    void osc_send_bridge_set_program(const int32_t index);
    void osc_send_bridge_set_midi_program(const int32_t index);
    void osc_send_bridge_set_custom_data(const char* const type, const char* const key, const char* const value);
    void osc_send_bridge_set_chunk_data(const char* const chunkFile);
    void osc_send_bridge_set_inpeak(const int32_t portId);
    void osc_send_bridge_set_outpeak(const int32_t portId);
#else
    void osc_send_control_add_plugin_start(const int32_t pluginId, const char* const pluginName);
    void osc_send_control_add_plugin_end(const int32_t pluginId);
    void osc_send_control_remove_plugin(const int32_t pluginId);
    void osc_send_control_set_plugin_data(const int32_t pluginId, const int32_t type, const int32_t category, const int32_t hints, const char* const realName, const char* const label, const char* const maker, const char* const copyright, const int64_t uniqueId);
    void osc_send_control_set_plugin_ports(const int32_t pluginId, const int32_t audioIns, const int32_t audioOuts, const int32_t midiIns, const int32_t midiOuts, const int32_t cIns, const int32_t cOuts, const int32_t cTotals);
    void osc_send_control_set_parameter_data(const int32_t pluginId, const int32_t index, const int32_t type, const int32_t hints, const char* const name, const char* const label, const double current);
    void osc_send_control_set_parameter_ranges(const int32_t pluginId, const int32_t index, const double min, const double max, const double def, const double step, const double stepSmall, const double stepLarge);
    void osc_send_control_set_parameter_midi_cc(const int32_t pluginId, const int32_t index, const int32_t cc);
    void osc_send_control_set_parameter_midi_channel(const int32_t pluginId, const int32_t index, const int32_t channel);
    void osc_send_control_set_parameter_value(const int32_t pluginId, const int32_t index, const double value);
    void osc_send_control_set_default_value(const int32_t pluginId, const int32_t index, const double value);
    void osc_send_control_set_program(const int32_t pluginId, const int32_t index);
    void osc_send_control_set_program_count(const int32_t pluginId, const int32_t count);
    void osc_send_control_set_program_name(const int32_t pluginId, const int32_t index, const char* const name);
    void osc_send_control_set_midi_program(const int32_t pluginId, const int32_t index);
    void osc_send_control_set_midi_program_count(const int32_t pluginId, const int32_t count);
    void osc_send_control_set_midi_program_data(const int32_t pluginId, const int32_t index, const int32_t bank, const int32_t program, const char* const name);
    void osc_send_control_note_on(const int32_t pluginId, const int32_t channel, const int32_t note, const int32_t velo);
    void osc_send_control_note_off(const int32_t pluginId, const int32_t channel, const int32_t note);
    void osc_send_control_set_input_peak_value(const int32_t pluginId, const int32_t portId);
    void osc_send_control_set_output_peak_value(const int32_t pluginId, const int32_t portId);
    void osc_send_control_exit();
#endif

    // -------------------------------------

#if 0
    /*!
     * \class ScopedLocker
     *
     * \brief Carla engine scoped locker
     *
     * This is a handy class that temporarily locks an engine during a function scope.
     */
    class ScopedLocker
    {
    public:
        /*!
         * Lock the engine \a engine if \a lock is true.
         * The engine is unlocked in the deconstructor of this class if \a lock is true.
         *
         * \param engine The engine to lock
         * \param lock Wherever to lock the engine or not, true by default
         */
        ScopedLocker(CarlaEngine* const engine, bool lock = true);
        //    : mutex(&engine->m_procLock),
        //      m_lock(lock)
        //{
        //    if (m_lock)
        //        mutex->lock();
        //}

        ~ScopedLocker() {}
        //{
        //    if (m_lock)
        //        mutex->unlock();
        //}

    private:
        QMutex* const mutex;
        const bool m_lock;
    };
#endif

    // -------------------------------------

protected:
    CarlaString name;
    uint32_t bufferSize;
    double   sampleRate;

    CarlaEngineOptions  options;
    CarlaEngineTimeInfo timeInfo;

#ifndef BUILD_BRIDGE
    // Rack mode data
    static const unsigned short MAX_CONTROL_EVENTS = 512;
    static const unsigned short MAX_MIDI_EVENTS    = 512;
    CarlaEngineControlEvent rackControlEventsIn[MAX_CONTROL_EVENTS];
    CarlaEngineControlEvent rackControlEventsOut[MAX_CONTROL_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsIn[MAX_MIDI_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsOut[MAX_MIDI_EVENTS];

    /*!
     * Proccess audio buffer in rack mode.
     */
    void processRack(float* inBuf[2], float* outBuf[2], const uint32_t frames);

    /*!
     * Proccess audio buffer in patchbay mode.
     * In \a bufCount, [0]=inBufCount and [1]=outBufCount
     */
    void processPatchbay(float** inBuf, float** outBuf, const uint32_t bufCount[2], const uint32_t frames);
#endif

    /*!
     * Report to all plugins about buffer size change.
     */
    void bufferSizeChanged(const uint32_t newBufferSize);

    /*!
     * Report to all plugins about sample rate change.
     * This is not supported on all plugin types, on which case they will be re-initiated.
     */
    void sampleRateChanged(const double newSampleRate);

private:
#ifdef CARLA_ENGINE_JACK
    static CarlaEngine* newJack();
#endif
#ifdef CARLA_ENGINE_RTAUDIO
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

    static CarlaEngine* newRtAudio(RtAudioApi api);
    static unsigned int getRtAudioApiCount();
    static const char*  getRtAudioApiName(unsigned int index);
#endif

    ScopedPointer<CarlaEnginePrivateData> const data;

    friend class CarlaEngineControlPort;
    friend class CarlaEngineMidiPort;

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngine)
};

// -----------------------------------------------------------------------

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_ENGINE_HPP__
