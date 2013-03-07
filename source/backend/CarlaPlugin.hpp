/*
 * Carla Plugin API
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_PLUGIN_HPP__
#define __CARLA_PLUGIN_HPP__

#include "CarlaBackend.hpp"
#include "CarlaString.hpp"

#ifndef DOXYGEN
// Avoid including extra libs here
struct LADSPA_RDF_Descriptor;
typedef void* lo_address;
typedef struct _PluginDescriptor PluginDescriptor;
#endif

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

#if 1//ndef BUILD_BRIDGE
/*!
 * TODO.
 */
enum PluginBridgeInfoType {
    kPluginBridgeAudioCount,
    kPluginBridgeMidiCount,
    kPluginBridgeParameterCount,
    kPluginBridgeProgramCount,
    kPluginBridgeMidiProgramCount,
    kPluginBridgePluginInfo,
    kPluginBridgeParameterInfo,
    kPluginBridgeParameterData,
    kPluginBridgeParameterRanges,
    kPluginBridgeProgramInfo,
    kPluginBridgeMidiProgramInfo,
    kPluginBridgeConfigure,
    kPluginBridgeSetParameterValue,
    kPluginBridgeSetDefaultValue,
    kPluginBridgeSetProgram,
    kPluginBridgeSetMidiProgram,
    kPluginBridgeSetCustomData,
    kPluginBridgeSetChunkData,
    kPluginBridgeUpdateNow,
    kPluginBridgeError
};
#endif

/*!
 * TODO.
 */
enum PluginPostRtEventType {
    kPluginPostRtEventNull,
    kPluginPostRtEventDebug,
    kPluginPostRtEventParameterChange,   // param, N, value
    kPluginPostRtEventProgramChange,     // index
    kPluginPostRtEventMidiProgramChange, // index
    kPluginPostRtEventNoteOn,            // channel, note, velo
    kPluginPostRtEventNoteOff            // channel, note
};

// -----------------------------------------------------------------------

/*!
 * Save state data.
 */
struct SaveState;

/*!
 * Protected data used in CarlaPlugin and subclasses.\n
 * Non-plugin code MUST NEVER have direct access to this.
 */
struct CarlaPluginProtectedData;

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
protected:
    /*!
     * This is the constructor of the base plugin class.
     *
     * \param engine The engine which this plugin belongs to, must not be null
     * \param id     The 'id' of this plugin, must be between 0 and CarlaEngine::maxPluginNumber()
     */
    CarlaPlugin(CarlaEngine* const engine, const unsigned int id);

public:
    /*!
     * This is the destructor of the base plugin class.
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
     * if (hints() & PLUGIN_IS_BRIDGE)
     *     ...
     * \endcode
     */
    virtual PluginType type() const = 0;

    /*!
     * Get the plugin's id (as passed in the constructor).
     *
     * \see setId()
     */
    unsigned int id() const
    {
        return fId;
    }

    /*!
     * Get the plugin's hints.
     *
     * \see PluginHints
     */
    unsigned int hints() const
    {
        return fHints;
    }

    /*!
     * Get the plugin's options.
     *
     * \see PluginOptions, availableOptions() and setOption()
     */
    unsigned int options() const
    {
        return fOptions;
    }

    /*!
     * Check if the plugin is enabled.\n
     * When a plugin is disabled, it will never be processed or managed in any way.\n
     * To 'bypass' a plugin use setActive() instead.
     *
     * \see setEnabled()
     */
    bool enabled() const
    {
        return fEnabled;
    }

    /*!
     * Get the plugin's internal name.\n
     * This name is unique within all plugins in an engine.
     *
     * \see getRealName()
     */
    const char* name() const
    {
        return (const char*)fName;
    }

    /*!
     * Get the currently loaded DLL filename for this plugin.\n
     * (Sound kits return their exact filename).
     */
    const char* filename() const
    {
        return (const char*)fFilename;
    }

    /*!
     * Get the plugin's category (delay, filter, synth, etc).
     */
    virtual PluginCategory category() const
    {
        return PLUGIN_CATEGORY_NONE;
    }

    /*!
     * Get the plugin's native unique Id.\n
     * May return 0 on plugin types that don't support Ids.
     */
    virtual long uniqueId() const
    {
        return 0;
    }

    /*!
     * Get the plugin's latency, in sample frames.
     */
    uint32_t latency() const;

    // -------------------------------------------------------------------
    // Information (count)

    /*!
     * Get the number of audio inputs.
     */
    virtual uint32_t audioInCount() const;

    /*!
     * Get the number of audio outputs.
     */
    virtual uint32_t audioOutCount() const;

    /*!
     * Get the number of MIDI inputs.
     */
    virtual uint32_t midiInCount() const;

    /*!
     * Get the number of MIDI outputs.
     */
    virtual uint32_t midiOutCount() const;

    /*!
     * Get the number of parameters.\n
     * To know the number of parameter inputs and outputs separately use getParameterCountInfo() instead.
     */
    uint32_t parameterCount() const;

    /*!
     * Get the number of scalepoints for parameter \a parameterId.
     */
    virtual uint32_t parameterScalePointCount(const uint32_t parameterId) const;

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
    uint32_t customDataCount() const;

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
    const ParameterData& parameterData(const uint32_t parameterId) const;

    /*!
     * Get the parameter ranges of \a parameterId.
     */
    const ParameterRanges& parameterRanges(const uint32_t parameterId) const;

    /*!
     * Check if parameter \a parameterId is of output type.
     */
    bool parameterIsOutput(const uint32_t parameterId) const;

    /*!
     * Get the MIDI program at \a index.
     *
     * \see getMidiProgramName()
     */
    const MidiProgramData& midiProgramData(const uint32_t index) const;

    /*!
     * Get the custom data set at \a index.
     *
     * \see setCustomData()
     */
    const CustomData& customData(const size_t index) const;

    /*!
     * Get the complete plugin chunk data into \a dataPtr.
     *
     * \note Make sure to verify the plugin supports chunks before calling this function!
     * \return The size of the chunk or 0 if invalid.
     *
     * \see setChunkData()
     */
    virtual int32_t chunkData(void** const dataPtr);

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    /*!
     * Get the plugin available options.
     *
     * \see PluginOptions
     */
    virtual unsigned int availableOptions();

    /*!
     * Get the current parameter value of \a parameterId.
     */
    virtual float getParameterValue(const uint32_t parameterId);

    /*!
     * Get the scalepoint \a scalePointId value of the parameter \a parameterId.
     */
    virtual float getParameterScalePointValue(const uint32_t parameterId, const uint32_t scalePointId);

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

    // -------------------------------------------------------------------
    // Set data (state)

    /*!
     * Tell the plugin to prepare for save.
     */
    virtual void prepareForSave();

    /*!
     * Get the plugin's save state.\n
     * The plugin will automatically call prepareForSave() as needed.
     *
     * \see loadSaveState()
     */
    const SaveState& getSaveState();

    /*!
     * Get the plugin's save state.
     *
     * \see getSaveState()
     */
    void loadSaveState(const SaveState& saveState);

    /*!
     * TODO
     */
    bool saveStateToFile(const char* const filename);

    /*!
     * TODO
     */
    bool loadStateFromFile(const char* const filename);

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    /*!
     * Set the plugin's id to \a id.
     *
     * \see id()
     */
    void setId(const unsigned int id);

    /*!
     * Set a plugin's option.
     *
     * \see options()
     */
    void setOption(const unsigned int option, const bool yesNo);

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
    void setDryWet(const float value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's output volume to \a value.\n
     * \a value must be between 0.0 and 1.27.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setVolume(const float value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's output left balance value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     *
     * \note Pure-Stereo plugins only!
     */
    void setBalanceLeft(const float value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's output right balance value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     *
     * \note Pure-Stereo plugins only!
     */
    void setBalanceRight(const float value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's output panning value to \a value.\n
     * \a value must be between -1.0 and 1.0.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     *
     * \note Force-Stereo plugins only!
     */
    void setPanning(const float value, const bool sendOsc, const bool sendCallback);

    /*!
     * Set the plugin's midi control channel.
     *
     * \param sendOsc Send message change over OSC
     * \param sendCallback Send message change to registered callback
     */
    void setCtrlChannel(const int8_t channel, const bool sendOsc, const bool sendCallback);

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
    virtual void setParameterValue(const uint32_t parameterId, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback);

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
    void setParameterValueByRIndex(const int32_t rindex, const float value, const bool sendGui, const bool sendOsc, const bool sendCallback);

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
    virtual void setProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback);

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
    virtual void setMidiProgram(const int32_t index, const bool sendGui, const bool sendOsc, const bool sendCallback);

    /*!
     * This is an overloaded call to setMidiProgram().\n
     * It changes the current MIDI program using \a bank and \a program values instead of index.
     */
    void setMidiProgramById(const uint32_t bank, const uint32_t program, const bool sendGui, const bool sendOsc, const bool sendCallback);

    // -------------------------------------------------------------------
    // Set gui stuff

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

    // -------------------------------------------------------------------
    // Plugin processing

    /*!
     * Plugin process callback.
     */
    virtual void process(float** const inBuffer, float** const outBuffer, const uint32_t frames);

    /*!
     * Tell the plugin the current buffer size has changed.
     */
    virtual void bufferSizeChanged(const uint32_t newBufferSize);

    /*!
     * Tell the plugin the current sample rate has changed.
     */
    virtual void sampleRateChanged(const double newSampleRate);

    /*!
     * Recreate latency audio buffers.
     */
    void recreateLatencyBuffers();

    /*!
     * TODO.
     */
    bool tryLock();

    /*!
     * TODO.
     */
    void unlock();

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
    void updateOscData(const lo_address& source, const char* const url);

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
     * \note Non-RT call
     */
    void sendMidiSingleNote(const uint8_t channel, const uint8_t note, const uint8_t velo, const bool sendGui, const bool sendOsc, const bool sendCallback);

    /*!
     * Send all midi notes off for the next audio callback.\n
     * This doesn't send the actual MIDI All-Notes-Off event, but 128 note-offs instead.
     * \note RT call
     */
    void sendMidiAllNotesOff();

    // -------------------------------------------------------------------
    // Post-poned events

    /*!
     * Post pone an event of type \a type.\n
     * The event will be processed later, but as soon as possible.
     * \note RT call
     */
    void postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3);

    /*!
     * Process all the post-poned events.
     * This function must be called from the main thread (ie, idleGui()) if PLUGIN_USES_SINGLE_THREAD is set.
     */
    void postRtEventsRun();

    /*!
     * Tell the UI a parameter has changed.
     */
    virtual void uiParameterChange(const uint32_t index, const float value);

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
    // Plugin initializers

    struct Initializer {
        CarlaEngine* const engine;
        const unsigned int id;
        const char* const filename;
        const char* const name;
        const char* const label;
    };

    static size_t getNativePluginCount();
    static const PluginDescriptor* getNativePluginDescriptor(const size_t index);

    static CarlaPlugin* newNative(const Initializer& init);
    static CarlaPlugin* newBridge(const Initializer& init, const BinaryType btype, const PluginType ptype, const char* const bridgeFilename);

    static CarlaPlugin* newLADSPA(const Initializer& init, const LADSPA_RDF_Descriptor* const rdfDescriptor);
    static CarlaPlugin* newDSSI(const Initializer& init, const char* const guiFilename);
    static CarlaPlugin* newLV2(const Initializer& init);
    static CarlaPlugin* newVST(const Initializer& init);
    static CarlaPlugin* newGIG(const Initializer& init, const bool use16Outs);
    static CarlaPlugin* newSF2(const Initializer& init, const bool use16Outs);
    static CarlaPlugin* newSFZ(const Initializer& init, const bool use16Outs);

    // -------------------------------------------------------------------

protected:
    unsigned int fId;
    unsigned int fHints;
    unsigned int fOptions;

    bool fEnabled;

    CarlaString fName;
    CarlaString fFilename;

    friend struct CarlaPluginProtectedData;
    CarlaPluginProtectedData* const kData;

    // Fully disable plugin in scope and also its engine client
    // May wait-block on constructor for plugin process to end
    class ScopedDisabler
    {
    public:
        ScopedDisabler(CarlaPlugin* const plugin);
        ~ScopedDisabler();

    private:
        CarlaPlugin* const kPlugin;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedDisabler)
    };

    // Lock the plugin's own run/process call
    // Plugin will still work as normal, but output only silence
    // On destructor needsReset flag might be set if the plugin might have missed some events
    class ScopedProcessLocker
    {
    public:
        ScopedProcessLocker(CarlaPlugin* const plugin, const bool block);
        ~ScopedProcessLocker();

    private:
        CarlaPlugin* const kPlugin;
        const bool kBlock;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedProcessLocker)
    };

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPlugin)
};

/**@}*/

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_PLUGIN_HPP__
