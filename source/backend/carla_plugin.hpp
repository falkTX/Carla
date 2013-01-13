/*
 * Carla Plugin
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_PLUGIN_HPP
#define CARLA_PLUGIN_HPP

#include "carla_midi.h"
#include "carla_engine.hpp"
#include "carla_osc_utils.hpp"
#include "carla_plugin_thread.hpp"

#ifdef BUILD_BRIDGE
# include "carla_bridge_osc.hpp"
#endif

// common includes
#include <cmath>
#include <vector>
#include <QtCore/QMutex>
#include <QtGui/QMainWindow>

#ifdef Q_WS_X11
# include <QtGui/QX11EmbedContainer>
typedef QX11EmbedContainer GuiContainer;
#else
# include <QtGui/QWidget>
typedef QWidget GuiContainer;
#endif

typedef struct _PluginDescriptor PluginDescriptor;

CARLA_BACKEND_START_NAMESPACE

/*!
 * @defgroup CarlaBackendPlugin Carla Backend Plugin
 *
 * The Carla Backend Plugin.
 * @{
 */

#define CARLA_PROCESS_CONTINUE_CHECK if (! m_enabled) { x_engine->callback(CALLBACK_DEBUG, m_id, m_enabled, 0, 0.0, nullptr); return; }

const unsigned short MAX_MIDI_EVENTS = 512;
const unsigned short MAX_POST_EVENTS = 152;

#ifndef BUILD_BRIDGE
enum PluginBridgeInfoType {
    PluginBridgeAudioCount,
    PluginBridgeMidiCount,
    PluginBridgeParameterCount,
    PluginBridgeProgramCount,
    PluginBridgeMidiProgramCount,
    PluginBridgePluginInfo,
    PluginBridgeParameterInfo,
    PluginBridgeParameterData,
    PluginBridgeParameterRanges,
    PluginBridgeProgramInfo,
    PluginBridgeMidiProgramInfo,
    PluginBridgeConfigure,
    PluginBridgeSetParameterValue,
    PluginBridgeSetDefaultValue,
    PluginBridgeSetProgram,
    PluginBridgeSetMidiProgram,
    PluginBridgeSetCustomData,
    PluginBridgeSetChunkData,
    PluginBridgeUpdateNow,
    PluginBridgeError
};
#endif

enum PluginPostEventType {
    PluginPostEventNull,
    PluginPostEventDebug,
    PluginPostEventParameterChange,   // param, N, value
    PluginPostEventProgramChange,     // index
    PluginPostEventMidiProgramChange, // index
    PluginPostEventNoteOn,            // channel, note, velo
    PluginPostEventNoteOff,           // channel, note
    PluginPostEventCustom
};

struct PluginAudioData {
    uint32_t count;
    uint32_t* rindexes;
    CarlaEngineAudioPort** ports;

    PluginAudioData()
        : count(0),
          rindexes(nullptr),
          ports(nullptr) {}
};

struct PluginMidiData {
    CarlaEngineMidiPort* portMin;
    CarlaEngineMidiPort* portMout;

    PluginMidiData()
        : portMin(nullptr),
          portMout(nullptr) {}
};

struct PluginParameterData {
    uint32_t count;
    ParameterData* data;
    ParameterRanges* ranges;
    CarlaEngineControlPort* portCin;
    CarlaEngineControlPort* portCout;

    PluginParameterData()
        : count(0),
          data(nullptr),
          ranges(nullptr),
          portCin(nullptr),
          portCout(nullptr) {}
};

struct PluginProgramData {
    uint32_t count;
    int32_t current;
    const char** names;

    PluginProgramData()
        : count(0),
          current(-1),
          names(nullptr) {}
};

struct PluginMidiProgramData {
    uint32_t count;
    int32_t current;
    MidiProgramData* data;

    PluginMidiProgramData()
        : count(0),
          current(-1),
          data(nullptr) {}
};

struct PluginPostEvent {
    PluginPostEventType type;
    int32_t value1;
    int32_t value2;
    double  value3;
    const void* cdata;

    PluginPostEvent()
        : type(PluginPostEventNull),
          value1(-1),
          value2(-1),
          value3(0.0),
          cdata(nullptr) {}
};

struct ExternalMidiNote {
    int8_t channel; // invalid = -1
    uint8_t note;
    uint8_t velo;

    ExternalMidiNote()
        : channel(-1),
          note(0),
          velo(0) {}
};

/*!
 * \class CarlaPlugin
 *
 * \brief Carla Backend base plugin class
 *
 * This is the base class for all available plugin types available in Carla Backend.\n
 * All virtual calls are implemented in this class as fallback, so it's safe to only override needed calls.
 *
 * \see PluginType
 */
class CarlaPlugin
{
public:
    /*!
     * This is the constructor of the base plugin class.
     *
     * \param engine The engine which this plugin belongs to, must not be null
     * \param id     The 'id' of this plugin, must between 0 and CarlaEngine::maxPluginNumber()
     */
    CarlaPlugin(CarlaEngine* const engine, const unsigned short id);

    /*!
     * This is the de-constructor of the base plugin class.
     */
    virtual ~CarlaPlugin();

    // -------------------------------------------------------------------
    // Information (base)

    /*!
     * Get the plugin's type (ie, a subclass of CarlaPlugin).
     *
     * \note Plugin bridges will return their respective plugin type, there is no plugin type such as "bridge".\n
     *       To check if a plugin is a bridge use:
     * \code
     * if (m_hints & PLUGIN_IS_BRIDGE)
     *     ...
     * \endcode
     */
    PluginType type() const;

    /*!
     * Get the plugin's id (as passed in the constructor).
     *
     * \see setId()
     */
    unsigned short id() const;

    /*!
     * Get the plugin's hints.
     *
     * \see PluginHints
     */
    unsigned int hints() const;

    /*!
     * Check if the plugin is enabled.
     *
     * \see setEnabled()
     */
    bool enabled() const;

    /*!
     * Get the plugin's internal name.\n
     * This name is unique within all plugins in an engine.
     *
     * \see getRealName()
     */
    const char* name() const;

    /*!
     * Get the currently loaded DLL filename for this plugin.\n
     * (Sound kits return their exact filename).
     */
    const char* filename() const;

    /*!
     * Get the plugin's category (delay, filter, synth, etc).
     */
    virtual PluginCategory category();

    /*!
     * Get the plugin's native unique Id.\n
     * May return 0 on plugin types that don't support Ids.
     */
    virtual long uniqueId();

    // -------------------------------------------------------------------
    // Information (count)

    /*!
     * Get the number of audio inputs.
     */
    virtual uint32_t audioInCount();

    /*!
     * Get the number of audio outputs.
     */
    virtual uint32_t audioOutCount();

    /*!
     * Get the number of MIDI inputs.
     */
    virtual uint32_t midiInCount();

    /*!
     * Get the number of MIDI outputs.
     */
    virtual uint32_t midiOutCount();

    /*!
     * Get the number of parameters.\n
     * To know the number of parameter inputs and outputs separately use getParameterCountInfo() instead.
     */
    uint32_t parameterCount() const;

    /*!
     * Get the number of scalepoints for parameter \a parameterId.
     */
    virtual uint32_t parameterScalePointCount(const uint32_t parameterId);

    /*!
     * Get the number of programs.
     */
    uint32_t programCount() const;

    /*!
     * Get the number of MIDI programs.
     */
    uint32_t midiProgramCount() const;

    /*!
     * Get the number of custom data sets.
     */
    size_t customDataCount() const;

    // -------------------------------------------------------------------
    // Information (current data)

    /*!
     * Get the current program number (-1 if unset).
     *
     * \see setProgram()
     */
    int32_t currentProgram() const;

    /*!
     * Get the current MIDI program number (-1 if unset).
     *
     * \see setMidiProgram()
     * \see setMidiProgramById()
     */
    int32_t currentMidiProgram() const;

    /*!
     * Get the parameter data of \a parameterId.
     */
    const ParameterData* parameterData(const uint32_t parameterId) const;

    /*!
     * Get the parameter ranges of \a parameterId.
     */
    const ParameterRanges* parameterRanges(const uint32_t parameterId) const;

    /*!
     * Check if parameter \a parameterId is of output type.
     */
    bool parameterIsOutput(const uint32_t parameterId) const;

    /*!
     * Get the MIDI program at \a index.
     *
     * \see getMidiProgramName()
     */
    const MidiProgramData* midiProgramData(const uint32_t index) const;

    /*!
     * Get the custom data set at \a index.
     *
     * \see setCustomData()
     */
    const CustomData* customData(const size_t index) const;

    /*!
     * Get the complete plugin chunk data into \a dataPtr.
     *
     * \return The size of the chunk or 0 if invalid.
     *
     * \note Make sure to verify the plugin supports chunks before calling this function!
     *
     * \see setChunkData()
     */
    virtual int32_t chunkData(void** const dataPtr);

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    /*!
     * Get the current parameter value of \a parameterId.
     */
    virtual double getParameterValue(const uint32_t parameterId);

    /*!
     * Get the scalepoint \a scalePointId value of the parameter \a parameterId.
     */
    virtual double getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId);

    /*!
     * Get the plugin's label (URI for PLUGIN_LV2).
     */
    virtual void getLabel(char* const strBuf);

    /*!
     * Get the plugin's maker.
     */
    virtual void getMaker(char* const strBuf);

    /*!
     * Get the plugin's copyright/license.
     */
    virtual void getCopyright(char* const strBuf);

    /*!
     * Get the plugin's (real) name.
     *
     * \see name()
     */
    virtual void getRealName(char* const strBuf);

    /*!
     * Get the name of the parameter \a parameterId.
     */
    virtual void getParameterName(const uint32_t parameterId, char* const strBuf);

    /*!
     * Get the symbol of the parameter \a parameterId.
     */
    virtual void getParameterSymbol(const uint32_t parameterId, char* const strBuf);

    /*!
     * Get the custom text of the parameter \a parameterId.
     */
    virtual void getParameterText(const uint32_t parameterId, char* const strBuf);

    /*!
     * Get the unit of the parameter \a parameterId.
     */
    virtual void getParameterUnit(const uint32_t parameterId, char* const strBuf);

    /*!
     * Get the scalepoint \a scalePointId label of the parameter \a parameterId.
     */
    virtual void getParameterScalePointLabel(const uint32_t parameterId, const uint32_t scalePointId, char* const strBuf);

    /*!
     * Get the name of the program at \a index.
     */
    void getProgramName(const uint32_t index, char* const strBuf);

    /*!
     * Get the name of the MIDI program at \a index.
     *
     * \see getMidiProgramInfo()
     */
    void getMidiProgramName(const uint32_t index, char* const strBuf);

    /*!
     * Get information about the plugin's parameter count.\n
     * This is used to check how many input, output and total parameters are available.\n
     *
     * \note Some parameters might not be input or output (ie, invalid).
     *
     * \see parameterCount()
     */
    void getParameterCountInfo(uint32_t* const ins, uint32_t* const outs, uint32_t* const total);

    /*!
     * Get information about the plugin's custom GUI, if provided.
     */
    virtual void getGuiInfo(GuiType* const type, bool* const resizable);

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    /*!
     * Set the plugin's id to \a id.
     *
     * \see id()
     */
    void setId(const unsigned short id);

    /*!
     * Enable or disable the plugin according to \a yesNo.
     *
     * When a plugin is disabled, it will never be processed or managed in any way.\n
     * To 'bypass' a plugin use setActive() instead.
     *
     * \see enabled()
     */
    void setEnabled(const bool yesNo);

    /*!
     * Set plugin as active according to \a active.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setActive(const bool active, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's dry/wet signal value to \a value.\n
     * \a value must be between 0.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setDryWet(double value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's output volume to \a value.\n
     * \a value must be between 0.0 and 1.27.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setVolume(double value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's output left balance value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setBalanceLeft(double value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's output right balance value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setBalanceRight(double value, const bool sendOsc, const bool sendCallback);

#ifndef BUILD_BRIDGE
    /*!
     * BridgePlugin call used to set internal data.
     */
    virtual int setOscBridgeInfo(const PluginBridgeInfoType type, const int argc, const lo_arg* const* const argv, const char* const types);
#endif

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    /*!
     * Set a plugin's parameter value.
     *
     * \param parameterId The parameter to change
     * \param value The new parameter value, must be within the parameter's range
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     *
     * \see getParameterValue()
     */
    virtual void setParameterValue(const uint32_t parameterId, double value, const bool sendGui, const bool sendOsc, const bool sendCallback);

    /*!
     * Set a plugin's parameter value, including internal parameters.\n
     * \a rindex can be negative to allow internal parameters change (as defined in InternalParametersIndex).
     *
     * \see setParameterValue()
     * \see setActive()
     * \see setDryWet()
     * \see setVolume()
     * \see setBalanceLeft()
     * \see setBalanceRight()
     */
    void setParameterValueByRIndex(const int32_t rindex, const double value, const bool sendGui, const bool sendOsc, const bool sendCallback);

    /*!
     * Set parameter's \a parameterId MIDI channel to \a channel.\n
     * \a channel must be between 0 and 15.
     */
    void setParameterMidiChannel(const uint32_t parameterId, uint8_t channel, const bool sendOsc, const bool sendCallback);

    /*!
     * Set parameter's \a parameterId MIDI CC to \a cc.\n
     * \a cc must be between 0 and 95 (0x5F), or -1 for invalid.
     */
    void setParameterMidiCC(const uint32_t parameterId, int16_t cc, const bool sendOsc, const bool sendCallback);

    /*!
     * Add a custom data set.\n
     * If \a key already exists, its current value will be swapped with \a value.
     *
     * \param type Type of data used in \a value.
     * \param key A key identifing this data set.
     * \param value The value of the data set, of type \a type.
     * \param sendGui Send message change to plugin's custom GUI, if any
     *
     * \see customData()
     */
    virtual void setCustomData(const char* const type, const char* const key, const char* const value, const bool sendGui);

    /*!
     * Set the complete chunk data as \a stringData.\n
     * \a stringData must a base64 encoded string of binary data.
     *
     * \see chunkData()
     *
     * \note Make sure to verify the plugin supports chunks before calling this function!
     */
    virtual void setChunkData(const char* const stringData);

    /*!
     * Change the current plugin program to \a index.
     *
     * If \a index is negative the plugin's program will be considered unset.\n
     * The plugin's default parameter values will be updated when this function is called.
     *
     * \param index New program index to use
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     * \param block Block the audio callback
     */
    virtual void setProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block);

    /*!
     * Change the current MIDI plugin program to \a index.
     *
     * If \a index is negative the plugin's program will be considered unset.\n
     * The plugin's default parameter values will be updated when this function is called.
     *
     * \param index New program index to use
     * \param sendGui Send message change to plugin's custom GUI, if any
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     * \param block Block the audio callback
     */
    virtual void setMidiProgram(int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block);

    /*!
     * This is an overloaded call to setMidiProgram().\n
     * It changes the current MIDI program using \a bank and \a program values instead of index.
     */
    void setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback, const bool block);

    // -------------------------------------------------------------------
    // Set gui stuff

    /*!
     * Set the plugin's custom GUI container.\n
     *
     * \note This function must be always called from the main thread.
     */
    virtual void setGuiContainer(GuiContainer* const container);

    /*!
     * Show (or hide) the plugin's custom GUI according to \a yesNo.
     *
     * \note This function must be always called from the main thread.
     */
    virtual void showGui(const bool yesNo);

    /*!
     * Idle the plugin's custom GUI.
     *
     * \note This function must be always called from the main thread.
     */
    virtual void idleGui();

    // -------------------------------------------------------------------
    // Plugin state

    /*!
     * Reload the plugin's entire state (including programs).\n
     * The plugin will be disabled during this call.
     */
    virtual void reload();

    /*!
     * Reload the plugin's programs state.
     */
    virtual void reloadPrograms(const bool init);

    /*!
     * Tell the plugin to prepare for save.
     */
    virtual void prepareForSave();

    // -------------------------------------------------------------------
    // Plugin processing

    /*!
     * Plugin process callback.
     */
    virtual void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t framesOffset = 0);

    /*!
     * Tell the plugin the current buffer size has changed.
     */
    virtual void bufferSizeChanged(const uint32_t newBufferSize);

    /*!
     * Recreate latency audio buffers.
     */
    void recreateLatencyBuffers();

    // -------------------------------------------------------------------
    // OSC stuff

    /*!
     * Register this plugin to the engine's OSC client (controller or bridge).
     */
    void registerToOscClient();

    /*!
     * Update the plugin's internal OSC data according to \a source and \a url.\n
     * This is used for OSC-GUI bridges.
     */
    void updateOscData(const lo_address source, const char* const url);

    /*!
     * Free the plugin's internal OSC memory data.
     */
    void freeOscData();

    /*!
     * Show the plugin's OSC based GUI.\n
     * This is a handy function that waits for the GUI to respond and automatically asks it to show itself.
     */
    bool waitForOscGuiShow();

    // -------------------------------------------------------------------
    // MIDI events

    /*!
     * Send a single midi note to be processed in the next audio callback.\n
     * A note with 0 velocity means note-off.
     */
    void sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback);

    /*!
     * Send all midi notes off for the next audio callback.\n
     * This doesn't send the actual MIDI All-Notes-Off event, but 128 note-offs instead.
     */
    void sendMidiAllNotesOff();

    // -------------------------------------------------------------------
    // Post-poned events

    /*!
     * Post pone an event of type \a type.\n
     * The event will be processed later, but as soon as possible.
     */
    void postponeEvent(const PluginPostEventType type, const int32_t value1, const int32_t value2, const double value3, const void* const cdata = nullptr);

    /*!
     * Process all the post-poned events.
     * This function must be called from the main thread (ie, idleGui()) if PLUGIN_USES_SINGLE_THREAD is set.
     */
    void postEventsRun();

    /*!
     * Handle custom post event.\n
     * Implementation depends on plugin type.
     */
    virtual void postEventHandleCustom(const int32_t value1, const int32_t value2, const double value3, const void* const cdata);

    /*!
     * Tell the UI a parameter has changed.
     */
    virtual void uiParameterChange(const uint32_t index, const double value);

    /*!
     * Tell the UI the current program has changed.
     */
    virtual void uiProgramChange(const uint32_t index);

    /*!
     * Tell the UI the current midi program has changed.
     */
    virtual void uiMidiProgramChange(const uint32_t index);

    /*!
     * Tell the UI a note has been pressed.
     */
    virtual void uiNoteOn(const uint8_t channel, const uint8_t note, const uint8_t velo);

    /*!
     * Tell the UI a note has been released.
     */
    virtual void uiNoteOff(const uint8_t channel, const uint8_t note);

    // -------------------------------------------------------------------
    // Cleanup

    /*!
     * Clear the engine client ports of the plugin.
     */
    virtual void removeClientPorts();

    /*!
     * Initialize all RT buffers of the plugin.
     */
    virtual void initBuffers();

    /*!
     * Delete all temporary buffers of the plugin.
     */
    virtual void deleteBuffers();

    // -------------------------------------------------------------------
    // Library functions

    /*!
     * Open the DLL \a filename.
     */
    bool libOpen(const char* const filename);

    /*!
     * Close the DLL previously loaded in libOpen().
     */
    bool libClose();

    /*!
     * Get the symbol entry \a symbol of the currently loaded DLL.
     */
    void* libSymbol(const char* const symbol);

    /*!
     * Get the last DLL related error.
     */
    const char* libError(const char* const filename);

    // -------------------------------------------------------------------
    // Locks

    void engineProcessLock();
    void engineProcessUnlock();
    void engineMidiLock();
    void engineMidiUnlock();

    // -------------------------------------------------------------------
    // Plugin initializers

    struct initializer {
        CarlaEngine* const engine;
        const char* const filename;
        const char* const name;
        const char* const label;
    };

//#ifndef BUILD_BRIDGE
    static CarlaPlugin* newNative(const initializer& init);
//#endif
    static CarlaPlugin* newLADSPA(const initializer& init, const void* const extra);
    static CarlaPlugin* newDSSI(const initializer& init, const void* const extra);
    static CarlaPlugin* newLV2(const initializer& init);
    static CarlaPlugin* newVST(const initializer& init);
//#ifndef BUILD_BRIDGE
    static CarlaPlugin* newGIG(const initializer& init);
    static CarlaPlugin* newSF2(const initializer& init);
    static CarlaPlugin* newSFZ(const initializer& init);
    static CarlaPlugin* newBridge(const initializer& init, const BinaryType btype, const PluginType ptype, const void* const extra);
//#endif

    static size_t getNativePluginCount();
    static const PluginDescriptor* getNativePluginDescriptor(const size_t index);

    // -------------------------------------------------------------------

    /*!
     * \class ScopedDisabler
     *
     * \brief Carla plugin scoped disabler
     *
     * This is a handy class that temporarily disables a plugin during a function scope.\n
     * It should be used when the plugin needs reload or state change, something like this:
     * \code
     * {
     *      const CarlaPlugin::ScopedDisabler m(plugin);
     *      plugin->setChunkData(data);
     * }
     * \endcode
     */
    class ScopedDisabler
    {
    public:
        /*!
         * Disable plugin \a plugin if \a disable is true.
         * The plugin is re-enabled in the deconstructor of this class if \a disable is true.
         *
         * \param plugin The plugin to disable
         * \param disable Wherever to disable the plugin or not, true by default
         */
        ScopedDisabler(CarlaPlugin* const plugin, const bool disable = true)
            : m_plugin(plugin),
              m_disable(disable)
        {
            if (m_disable)
            {
                m_plugin->engineProcessLock();
                m_plugin->setEnabled(false);
                m_plugin->engineProcessUnlock();
            }
        }

        ~ScopedDisabler()
        {
            if (m_disable)
            {
                m_plugin->engineProcessLock();
                m_plugin->setEnabled(true);
                m_plugin->engineProcessUnlock();
            }
        }

    private:
        CarlaPlugin* const m_plugin;
        const bool m_disable;
    };

    // -------------------------------------------------------------------

protected:
    unsigned short m_id;
    CarlaEngine* const x_engine;
    CarlaEngineClient* x_client;
    double x_dryWet, x_volume;
    double x_balanceLeft, x_balanceRight;

    PluginType m_type;
    unsigned int m_hints;

    bool m_active;
    bool m_activeBefore;
    bool m_enabled;

    void* m_lib;
    const char* m_name;
    const char* m_filename;

    // options
    int8_t m_ctrlInChannel;
    bool   m_fixedBufferSize;
    bool   m_processHighPrecision;

    // latency
    uint32_t m_latency;
    float**  m_latencyBuffers;

    // -------------------------------------------------------------------
    // Storage Data

    PluginAudioData aIn;
    PluginAudioData aOut;
    PluginMidiData midi;
    PluginParameterData param;
    PluginProgramData prog;
    PluginMidiProgramData midiprog;
    std::vector<CustomData> custom;

    // -------------------------------------------------------------------
    // Extra

    struct {
        CarlaOscData data;
        CarlaPluginThread* thread;
    } osc;

    struct {
        QMutex mutex;
        PluginPostEvent data[MAX_POST_EVENTS];
    } postEvents;

    ExternalMidiNote extMidiNotes[MAX_MIDI_EVENTS];

    // -------------------------------------------------------------------
    // Utilities

    static double fixParameterValue(double& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
        return value;
    }

    static float fixParameterValue(float& value, const ParameterRanges& ranges)
    {
        if (value < ranges.min)
            value = ranges.min;
        else if (value > ranges.max)
            value = ranges.max;
        return value;
    }

    friend class CarlaEngine; // FIXME
    friend class CarlaEngineJack;
};

/*!
 * \class CarlaPluginGUI
 *
 * \brief Carla Backend gui plugin class
 *
 * \see CarlaPlugin
 */
class CarlaPluginGUI : public QMainWindow
{
public:
    /*!
     * \class Callback
     *
     * \brief Carla plugin GUI callback
     */
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void guiClosedCallback() = 0;
    };

    // -------------------------------------------------------------------
    // Constructor and destructor

    /*!
     * TODO
     */
    CarlaPluginGUI(QWidget* const parent, Callback* const callback);

    /*!
     * TODO
     */
    ~CarlaPluginGUI();

    // -------------------------------------------------------------------
    // Get data

    /*!
     * TODO
     */
    GuiContainer* getContainer() const;

    /*!
     * TODO
     */
    WId getWinId() const;

    // -------------------------------------------------------------------
    // Set data

    /*!
     * TODO
     */
    void setNewSize(const int width, const int height);

    /*!
     * TODO
     */
    void setResizable(const bool resizable);

    /*!
     * TODO
     */
    void setTitle(const char* const title);

    /*!
     * TODO
     */
    void setVisible(const bool yesNo);

    // -------------------------------------------------------------------

private:
    Callback* const m_callback;
    GuiContainer*   m_container;

    QByteArray m_geometry;
    bool m_resizable;

    void hideEvent(QHideEvent* const event);
    void closeEvent(QCloseEvent* const event);
};

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_HPP
