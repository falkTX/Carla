/*
 * Carla Engine
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef CARLA_ENGINE_HPP
#define CARLA_ENGINE_HPP

#include "carla_engine_osc.hpp"
#include "carla_engine_thread.hpp"

#include <QtCore/QProcessEnvironment>

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendEngine Carla Backend Engine
 *
 * The Carla Backend Engine
 * @{
 */

/*!
 * @defgroup TimeInfoValidHints TimeInfo Valid Hints
 *
 * Various hints used for CarlaTimeInfo::valid.
 * @{
 */
const uint32_t CarlaEngineTimeBBT = 0x1;
/**@}*/

/*!
 * The type of an engine.
 */
enum CarlaEngineType {
    /*!
    * Null engine type.
    */
    CarlaEngineTypeNull = 0,

    /*!
    * Jack engine type.\n
    * Provides single, multi-client, and rack processing modes.
    */
    CarlaEngineTypeJack = 1,

    /*!
    * RtAudio engine type, used to provide ALSA, PulseAudio, DirectSound, ASIO and CoreAudio/Midi support.\n
    * Provides rack mode processing only.
    */
    CarlaEngineTypeRtAudio = 2,

    /*!
    * Plugin engine type, used to export the engine as a plugin (DSSI, LV2 and VST) via the DISTRHO Plugin Toolkit.\n
    * Works in rack mode only.
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
 * The type of a control event.
 */
enum CarlaEngineControlEventType {
    /*!
    * Null event type.
    */
    CarlaEngineNullEvent = 0,

    /*!
    * Parameter change event.\n
    * \note Value uses a range of 0.0<->1.0.
    */
    CarlaEngineParameterChangeEvent = 1,

    /*!
    * MIDI Bank change event.
    */
    CarlaEngineMidiBankChangeEvent = 2,

    /*!
    * MIDI Program change event.
    */
    CarlaEngineMidiProgramChangeEvent = 3,

    /*!
    * All sound off event.
    */
    CarlaEngineAllSoundOffEvent = 4,

    /*!
    * All notes off event.
    */
    CarlaEngineAllNotesOffEvent = 5
};

/*!
 * Engine control event.
 */
struct CarlaEngineControlEvent {
    CarlaEngineControlEventType type;
    uint32_t time;
    uint8_t  channel;
    uint16_t parameter;
    double   value;

    CarlaEngineControlEvent()
        : type(CarlaEngineNullEvent),
          time(0),
          channel(0),
          parameter(0),
          value(0.0) {}
};

/*!
 * Engine MIDI event.
 */
struct CarlaEngineMidiEvent {
    uint32_t time;
    uint8_t  size;
    uint8_t  data[3];

    CarlaEngineMidiEvent()
        : time(0),
          size(0),
          data{0} {}
};

/*!
 * Engine BBT Time information.
 */
struct CarlaEngineTimeInfoBBT {
    int32_t bar;
    int32_t beat;
    int32_t tick;
    double bar_start_tick;
    float  beats_per_bar;
    float  beat_type;
    double ticks_per_beat;
    double beats_per_minute;

    CarlaEngineTimeInfoBBT()
        : bar(0),
          beat(0),
          tick(0),
          bar_start_tick(0.0),
          beats_per_bar(0.0f),
          beat_type(0.0f),
          ticks_per_beat(0.0),
          beats_per_minute(0.0) {}
};

/*!
 * Engine Time information.
 */
struct CarlaEngineTimeInfo {
    bool playing;
    uint32_t frame;
    uint32_t time;
    uint32_t valid;
    CarlaEngineTimeInfoBBT bbt;

    CarlaEngineTimeInfo()
        : playing(false),
          frame(0),
          time(0),
          valid(0) {}
};

/*!
 * Engine options.
 */
struct CarlaEngineOptions {
    ProcessMode processMode;
    bool processHighPrecision;

    uint maxParameters;
    uint preferredBufferSize;
    uint preferredSampleRate;

    bool forceStereo;
    bool useDssiVstChunks;

    bool preferPluginBridges;
    bool preferUiBridges;
    uint oscUiTimeout;

    CarlaString bridge_posix32;
    CarlaString bridge_posix64;
    CarlaString bridge_win32;
    CarlaString bridge_win64;
    CarlaString bridge_lv2gtk2;
    CarlaString bridge_lv2gtk3;
    CarlaString bridge_lv2qt4;
    CarlaString bridge_lv2qt5;
    CarlaString bridge_lv2cocoa;
    CarlaString bridge_lv2win;
    CarlaString bridge_lv2x11;
    CarlaString bridge_vstcocoa;
    CarlaString bridge_vsthwnd;
    CarlaString bridge_vstx11;

    CarlaEngineOptions()
        : processMode(PROCESS_MODE_CONTINUOUS_RACK),
          processHighPrecision(false),
          maxParameters(MAX_PARAMETERS),
          preferredBufferSize(512),
          preferredSampleRate(44100),
          forceStereo(false),
          useDssiVstChunks(false),
          preferPluginBridges(false),
          preferUiBridges(true),
          oscUiTimeout(4000/100) {}
};

// -----------------------------------------------------------------------

/*!
 * Engine port (Base).\n
 * This is the base class for all Carla engine ports.
 */
class CarlaEngineBasePort
{
public:
    /*!
     * The contructor.\n
     * Param \a isInput defines wherever this is an input port or not (output otherwise).\n
     * Input/output state is constant for the lifetime of the port.
     */
    CarlaEngineBasePort(const bool isInput, const ProcessMode processMode);

    /*!
     * The decontructor.
     */
    virtual ~CarlaEngineBasePort();

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
};

// -----------------------------------------------------------------------

/*!
 * Engine port (Audio).
 */
class CarlaEngineAudioPort : public CarlaEngineBasePort
{
public:
    /*!
     * The contructor.\n
     * Param \a isInput defines wherever this is an input port or not (output otherwise).\n
     * Input/output state is constant for the lifetime of the port.
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
};

// -----------------------------------------------------------------------

/*!
 * Engine port (Control).
 */
class CarlaEngineControlPort : public CarlaEngineBasePort
{
public:
    /*!
     * The contructor.\n
     * Param \a isInput defines wherever this is an input port or not (output otherwise).\n
     * Input/output state is constant for the lifetime of the port.
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
};

// -----------------------------------------------------------------------

/*!
 * Engine port (MIDI).
 */
class CarlaEngineMidiPort : public CarlaEngineBasePort
{
public:
    /*!
     * The contructor.\n
     * Param \a isInput defines wherever this is an input port or not (output otherwise).\n
     * Input/output state is constant for the lifetime of the port.
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
};

// -----------------------------------------------------------------------

/*!
 * Engine client.\n
 * Each plugin requires one client from the engine (created via CarlaEngine::addPort()).\n
 * \note This is a virtual class, each engine type provides its own funtionality.
 */
class CarlaEngineClient
{
public:
    /*!
     * The contructor.\n
     * All constructor parameters are constant and will never change in the lifetime of the client.\n
     * Client starts in deactivated state.
     */
    CarlaEngineClient(const CarlaEngineType engineType, const ProcessMode processMode);

    /*!
     * The decontructor.
     */
    virtual ~CarlaEngineClient();

    /*!
     * Activate this client.\n
     * \note Client must be deactivated before calling this function.
     */
    virtual void activate();

    /*!
     * Deactivate this client.\n
     * \note Client must be activated before calling this function.
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
     * \note This function does nothing in rack processing mode since its ports are static (2 audio, 1 midi and 1 control for both input and output).
     */
    virtual const CarlaEngineBasePort* addPort(const CarlaEnginePortType portType, const char* const name, const bool isInput) = 0;

protected:
    const CarlaEngineType engineType;
    const ProcessMode processMode;

private:
    bool     m_active;
    uint32_t m_latency;
};

// -----------------------------------------------------------------------

/*!
 * Carla Engine.
 * \note This is a virtual class for all available engine types available in Carla.
 */
class CarlaEngine
{
public:
    /*!
     * The decontructor.
     * The engine must have been closed before this happens.
     */
    virtual ~CarlaEngine();

    // -------------------------------------------------------------------
    // Static values and calls

    /*!
     * Maximum number of peaks per plugin.\n
     * \note There are both input and output peaks.
     */
    static const unsigned short MAX_PEAKS = 2;

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
    unsigned short maxPluginNumber() const;

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
    short getNewPluginId() const;

    /*!
     * Get plugin with id \a id.
     */
    CarlaPlugin* getPlugin(const unsigned short id) const;

    /*!
     * Get plugin with id \a id, faster unchecked version.
     */
    CarlaPlugin* getPluginUnchecked(const unsigned short id) const;

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
    short addPlugin(const PluginType ptype, const char* const filename, const char* const name, const char* const label, void* const extra = nullptr);

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
    void __bridgePluginRegister(const unsigned short id, CarlaPlugin* const plugin)
    {
        m_carlaPlugins[id] = plugin;
    }

    // -------------------------------------------------------------------
    // Information (base)

    /*!
     * Get engine name.
     */
    const char* getName() const;

    /*!
     * Get current sample rate.
     */
    double getSampleRate() const;

    /*!
     * Get current buffer size.
     */
    uint32_t getBufferSize() const;

    /*!
     * Get current Time information.
     */
    const CarlaEngineTimeInfo* getTimeInfo() const;

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

    /*!
     * Get the engine options (read-only).
     */
    const CarlaEngineOptions& getOptions() const
    {
        return options;
    }

#ifndef BUILD_BRIDGE
    /*!
     * Get the engine options as process environment.
     */
    const QProcessEnvironment& getOptionsAsProcessEnvironment() const
    {
        return m_procEnv;
    }

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

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------
    // Rack mode

    static const unsigned short MAX_CONTROL_EVENTS = 512;
    static const unsigned short MAX_MIDI_EVENTS    = 512;
    CarlaEngineControlEvent rackControlEventsIn[MAX_CONTROL_EVENTS];
    CarlaEngineControlEvent rackControlEventsOut[MAX_CONTROL_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsIn[MAX_MIDI_EVENTS];
    CarlaEngineMidiEvent rackMidiEventsOut[MAX_MIDI_EVENTS];
#endif

    // -------------------------------------

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
        ScopedLocker(CarlaEngine* const engine, bool lock = true)
            : mutex(&engine->m_procLock),
              m_lock(lock)
        {
            if (m_lock)
                mutex->lock();
        }

        ~ScopedLocker()
        {
            if (m_lock)
                mutex->unlock();
        }

    private:
        QMutex* const mutex;
        const bool m_lock;
    };

    // -------------------------------------

protected:
    /*!
     * The contructor, protected.\n
     * \note This only initializes engine data, it doesn't initialize the engine itself.
     */
    CarlaEngine();

#ifndef BUILD_BRIDGE
    /*!
     * Proccess audio buffer in rack mode, protected.
     */
    void processRack(float* inBuf[2], float* outBuf[2], const uint32_t frames);
#endif

    CarlaEngineOptions options;

    CarlaString name;
    uint32_t bufferSize;
    double   sampleRate;
    CarlaEngineTimeInfo timeInfo;

    void bufferSizeChanged(const uint32_t newBufferSize);

private:
    CarlaEngineOsc m_osc;
    CarlaEngineThread m_thread;

    const CarlaOscData* m_oscData;

    CallbackFunc m_callback;
    void*        m_callbackPtr;
    CarlaString  m_lastError;

#ifndef BUILD_BRIDGE
    QProcessEnvironment m_procEnv;
#endif

    QMutex m_procLock;
    QMutex m_midiLock;

    CarlaPlugin* m_carlaPlugins[MAX_PLUGINS];
    const char*  m_uniqueNames[MAX_PLUGINS];

    double m_insPeak[MAX_PLUGINS * MAX_PEAKS];
    double m_outsPeak[MAX_PLUGINS * MAX_PEAKS];

    bool m_aboutToClose;
    unsigned short m_maxPluginNumber;

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
    static const char* getRtAudioApiName(unsigned int index);
#endif
};

// -----------------------------------------------------------------------

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_HPP
