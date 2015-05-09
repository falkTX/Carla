/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNativeExtUI.hpp"
#include "CarlaMIDI.h"
#include "CarlaThread.hpp"
#include "LinkedList.hpp"

#include "CarlaMathUtils.hpp"

#include "DSP/FFTwrapper.h"
#include "Misc/Master.h"
#include "Misc/MiddleWare.h"
#include "Misc/Part.h"
#include "Misc/Util.h"

#ifdef HAVE_ZYN_UI_DEPS
# include "UI/Connection.h"
#endif

#include <ctime>
#include <set>
#include <string>

#include "juce_audio_basics.h"
using juce::roundToIntAccurate;
using juce::FloatVectorOperations;

// -----------------------------------------------------------------------

#ifdef HAVE_ZYN_UI_DEPS
namespace GUI {
Fl_Osc_Interface* genOscInterface(MiddleWare*)
{
    return nullptr;
}
void raiseUi(ui_handle_t, const char *)
{
}
#if 0
ui_handle_t createUi(Fl_Osc_Interface*, void *exit)
{
    return nullptr;
}
void destroyUi(ui_handle_t)
{
}
void raiseUi(ui_handle_t, const char *, const char *, ...)
{
}
void tickUi(ui_handle_t)
{
    //usleep(100000);
}
#endif
};
#endif

// -----------------------------------------------------------------------

class ZynAddSubFxPrograms
{
public:
    ZynAddSubFxPrograms() noexcept
        : fInitiated(false),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fRetProgram({0, 0, nullptr}),
#endif
          fPrograms() {}

    ~ZynAddSubFxPrograms() noexcept
    {
        if (! fInitiated)
            return;

        for (LinkedList<const ProgramInfo*>::Itenerator it = fPrograms.begin(); it.valid(); it.next())
        {
            const ProgramInfo* const& pInfo(it.getValue(nullptr));
            delete pInfo;
        }

        fPrograms.clear();
    }

    void initIfNeeded()
    {
        if (fInitiated)
            return;
        fInitiated = true;

        fPrograms.append(new ProgramInfo(0, 0, "default"));

        Master& master(getMasterInstance());

        // refresh banks
        master.bank.rescanforbanks();

        for (uint32_t i=0, size=static_cast<uint32_t>(master.bank.banks.size()); i<size; ++i)
        {
            if (master.bank.banks[i].dir.empty())
                continue;

            master.bank.loadbank(master.bank.banks[i].dir);

            for (uint instrument = 0; instrument < BANK_SIZE; ++instrument)
            {
                const std::string insName(master.bank.getname(instrument));

                if (insName.empty() || insName[0] == '\0' || insName[0] == ' ')
                    continue;

                fPrograms.append(new ProgramInfo(i+1, instrument, insName.c_str()));
            }
        }
    }

    void load(Master* const master, CarlaMutex& mutex, const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        if (bank == 0)
        {
            if (program != 0)
                return;

            const CarlaMutexLocker cml(mutex);

            master->partonoff(channel, 1);
            master->part[channel]->defaults();
            master->part[channel]->applyparameters(false);
            return;
        }

        const Master&      gmaster(getMasterInstance());
        const std::string& bankdir(gmaster.bank.banks[bank-1].dir);

        if (bankdir.empty())
            return;

        const CarlaMutexLocker cml(mutex);

        master->partonoff(channel, 1);
        master->bank.loadbank(bankdir);
        master->bank.loadfromslot(program, master->part[channel]);
        master->part[channel]->applyparameters(false);
    }

    uint32_t getNativeMidiProgramCount() const noexcept
    {
        return static_cast<uint32_t>(fPrograms.count());
    }

    const NativeMidiProgram* getNativeMidiProgramInfo(const uint32_t index) const noexcept
    {
        if (index >= fPrograms.count())
            return nullptr;

        const ProgramInfo* const pInfo(fPrograms.getAt(index, nullptr));
        CARLA_SAFE_ASSERT_RETURN(pInfo != nullptr, nullptr);

        fRetProgram.bank    = pInfo->bank;
        fRetProgram.program = pInfo->prog;
        fRetProgram.name    = pInfo->name;

        return &fRetProgram;
    }

    uint32_t getZynBankCount() const
    {
        const Master& master(getMasterInstance());

        return master.bank.banks.size();
    }

private:
  struct ProgramInfo {
      uint32_t bank;
      uint32_t prog;
      const char* name;

      ProgramInfo(uint32_t b, uint32_t p, const char* n) noexcept
        : bank(b),
          prog(p),
          name(carla_strdup_safe(n)) {}

      ~ProgramInfo() noexcept
      {
          if (name != nullptr)
          {
              delete[] name;
              name = nullptr;
          }
      }

#ifdef CARLA_PROPER_CPP11_SUPPORT
      ProgramInfo() = delete;
      ProgramInfo(ProgramInfo&) = delete;
      ProgramInfo(const ProgramInfo&) = delete;
      ProgramInfo& operator=(ProgramInfo&);
      ProgramInfo& operator=(const ProgramInfo&);
#endif
    };

    bool fInitiated;
    mutable NativeMidiProgram fRetProgram;
    LinkedList<const ProgramInfo*> fPrograms;

    static Master& getMasterInstance()
    {
        static SYNTH_T synth;
        static Master  master(synth);
        return master;
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ZynAddSubFxPrograms)
};

static ZynAddSubFxPrograms sPrograms;

// -----------------------------------------------------------------------

class ZynAddSubFxThread : public CarlaThread
{
public:
    ZynAddSubFxThread(Master* const master, CarlaMutex& mutex) noexcept
        : CarlaThread("ZynAddSubFxThread"),
          fMaster(master),
          fMutex(mutex),
          fChangeProgram(false),
          fNextChannel(0),
          fNextBank(0),
          fNextProgram(0) {}

    void loadProgramLater(const uint8_t channel, const uint32_t bank, const uint32_t program) noexcept
    {
        fNextChannel   = channel;
        fNextBank      = bank;
        fNextProgram   = program;
        fChangeProgram = true;
    }

    void stopLoadProgramLater() noexcept
    {
        fChangeProgram = false;
        fNextChannel   = 0;
        fNextBank      = 0;
        fNextProgram   = 0;
    }

    void setMaster(Master* const master) noexcept
    {
        fMaster = master;
    }

protected:
    void run() override
    {
        while (! shouldThreadExit())
        {
            if (fChangeProgram)
            {
                fChangeProgram = false;
                sPrograms.load(fMaster, fMutex, fNextChannel, fNextBank, fNextProgram);
                fNextChannel = 0;
                fNextBank    = 0;
                fNextProgram = 0;

                carla_msleep(15);
            }
            else
            {
                carla_msleep(30);
            }
        }
    }

private:
    Master* fMaster;
    CarlaMutex& fMutex;

    volatile bool     fChangeProgram;
    volatile uint8_t  fNextChannel;
    volatile uint32_t fNextBank;
    volatile uint32_t fNextProgram;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ZynAddSubFxThread)
};

// -----------------------------------------------------------------------

class ZynAddSubFxPlugin : public NativePluginAndUiClass
{
public:
    enum Parameters {
        kParamFilterCutoff = 0, // Filter Frequency
        kParamFilterQ,          // Filter Resonance
        kParamBandwidth,        // Bandwidth
        kParamModAmp,           // FM Gain
        kParamResCenter,        // Resonance center frequency
        kParamResBandwidth,     // Resonance bandwidth
        kParamCount
    };

    ZynAddSubFxPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "zynaddsubfx-ui"),
          fMiddleWare(nullptr),
          fMaster(nullptr),
          fSynth(),
          fIsActive(false),
          fMutex(),
          fThread(nullptr, fMutex),
          leakDetector_ZynAddSubFxPlugin()
    {
        // init parameters to default
        fParameters[kParamFilterCutoff] = 64.0f;
        fParameters[kParamFilterQ]      = 64.0f;
        fParameters[kParamBandwidth]    = 64.0f;
        fParameters[kParamModAmp]       = 127.0f;
        fParameters[kParamResCenter]    = 64.0f;
        fParameters[kParamResBandwidth] = 64.0f;

        fSynth.buffersize = static_cast<int>(getBufferSize());
        fSynth.samplerate = static_cast<uint>(getSampleRate());

        //if (fSynth.buffersize > 32)
        //    fSynth.buffersize = 32;

        fSynth.alias();

        _initMaster();

        sPrograms.initIfNeeded();
    }

    ~ZynAddSubFxPlugin() override
    {
        _deleteMaster();
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const final
    {
        return kParamCount;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount, nullptr);

        static NativeParameter param;

        int hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_INTEGER|NATIVE_PARAMETER_IS_AUTOMABLE;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 64.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case kParamFilterCutoff:
            param.name = "Filter Cutoff";
            break;
        case kParamFilterQ:
            param.name = "Filter Q";
            break;
        case kParamBandwidth:
            param.name = "Bandwidth";
            break;
        case kParamModAmp:
            param.name = "FM Gain";
            param.ranges.def = 127.0f;
            break;
        case kParamResCenter:
            param.name = "Res Center Freq";
            break;
        case kParamResBandwidth:
            param.name = "Res Bandwidth";
            break;
        }

        param.hints = static_cast<NativeParameterHints>(hints);

        return &param;
    }

    float getParameterValue(const uint32_t index) const final
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount, 0.0f);

        return fParameters[index];
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() const noexcept override
    {
        return sPrograms.getNativeMidiProgramCount();
    }

    const NativeMidiProgram* getMidiProgramInfo(const uint32_t index) const noexcept override
    {
        return sPrograms.getNativeMidiProgramInfo(index);
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) final
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount,);

        const uint zynIndex(getZynParameterFromIndex(index));
        CARLA_SAFE_ASSERT_RETURN(zynIndex != C_NULL,);

        fParameters[index] = std::round(carla_fixValue(0.0f, 127.0f, value));

        for (int npart=0; npart<NUM_MIDI_PARTS; ++npart)
        {
            if (fMaster->part[npart] != nullptr && fMaster->part[npart]->Penabled != 0)
                fMaster->part[npart]->SetController(zynIndex, static_cast<int>(value));
        }
    }

    void setMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        if (bank >= sPrograms.getZynBankCount())
            return;
        if (program >= BANK_SIZE)
            return;

        if (isOffline() || ! fIsActive)
        {
            sPrograms.load(fMaster, fMutex, channel, bank, program);
            return;
        }

        fThread.loadProgramLater(channel, bank, program);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        const CarlaMutexLocker cml(fMutex);

        /**/ if (std::strcmp(key, "CarlaAlternateFile1") == 0) // xmz
        {
            fMaster->defaults();
            fMaster->loadXML(value);
        }
        else if (std::strcmp(key, "CarlaAlternateFile2") == 0) // xiz
        {
            fMaster->part[0]->defaultsinstrument();
            fMaster->part[0]->loadXMLinstrument(value);
        }

        fMaster->applyparameters();
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
        fIsActive = true;
    }

    void deactivate() override
    {
        fIsActive = false;
    }

    void process(float**, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        if (! fMutex.tryLock())
        {
            if (! isOffline())
            {
                FloatVectorOperations::clear(outBuffer[0], static_cast<int>(frames));
                FloatVectorOperations::clear(outBuffer[1], static_cast<int>(frames));
                return;
            }
            fMutex.lock();
        }

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const NativeMidiEvent* const midiEvent(&midiEvents[i]);

            const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);
            const char    channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent->data);

            if (MIDI_IS_STATUS_NOTE_OFF(status))
            {
                const char note = static_cast<char>(midiEvent->data[1]);

                fMaster->noteOff(channel, note);
            }
            else if (MIDI_IS_STATUS_NOTE_ON(status))
            {
                const char note = static_cast<char>(midiEvent->data[1]);
                const char velo = static_cast<char>(midiEvent->data[2]);

                fMaster->noteOn(channel, note, velo);
            }
            else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
            {
                const char note     = static_cast<char>(midiEvent->data[1]);
                const char pressure = static_cast<char>(midiEvent->data[2]);

                fMaster->polyphonicAftertouch(channel, note, pressure);
            }
            else if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
            {
                // skip controls which we map to parameters
                if (getZynParameterFromIndex(midiEvent->data[1]) != C_NULL)
                    continue;

                const int control = midiEvent->data[1];
                const int value   = midiEvent->data[2];

                fMaster->setController(channel, control, value);
            }
            else if (MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status))
            {
                const uint8_t lsb = midiEvent->data[1];
                const uint8_t msb = midiEvent->data[2];
                const int   value = ((msb << 7) | lsb) - 8192;

                fMaster->setController(channel, C_pitchwheel, value);
            }
        }

        fMaster->GetAudioOutSamples(frames, fSynth.samplerate, outBuffer[0], outBuffer[1]);

        fMutex.unlock();
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

#ifdef HAVE_ZYN_UI_DEPS
    void uiShow(const bool show) override
    {
        if (show)
        {
            if (isPipeRunning())
            {
                const CarlaMutexLocker cml(getPipeLock());
                writeMessage("focus\n", 6);
                flushMessages();
                return;
            }

            carla_stdout("Trying to start UI using \"%s\"", getExtUiPath());

            CarlaExternalUI::setData(getExtUiPath(), fMiddleWare->getServerAddress(), getUiName());

            if (! CarlaExternalUI::startPipeServer(true))
            {
                uiClosed();
                hostUiUnavailable();
            }
        }
        else
        {
            CarlaExternalUI::stopPipeServer(2000);
        }
    }

    void uiIdle() override
    {
        NativePluginAndUiClass::uiIdle();

        if (isPipeRunning())
            fMiddleWare->tick();
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        char* data = nullptr;
        fMaster->getalldata(&data);
        return data;
    }

    void setState(const char* const data) override
    {
        fThread.stopLoadProgramLater();

        const CarlaMutexLocker cml(fMutex);

        fMaster->putalldata(const_cast<char*>(data), 0);
        fMaster->applyparameters();
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    void bufferSizeChanged(const uint32_t bufferSize) final
    {
        char* const state(getState());

        _deleteMaster();

        fSynth.buffersize = static_cast<int>(bufferSize);
        fSynth.alias();

        _initMaster();

        if (state != nullptr)
        {
            fMaster->putalldata(state, 0);
            fMaster->applyparameters();
            std::free(state);
        }
    }

    void sampleRateChanged(const double sampleRate) final
    {
        char* const state(getState());

        _deleteMaster();

        fSynth.samplerate = static_cast<uint>(sampleRate);
        fSynth.alias();

        _initMaster();

        if (state != nullptr)
        {
            fMaster->putalldata(state, 0);
            fMaster->applyparameters();
            std::free(state);
        }
    }

    // -------------------------------------------------------------------

private:
    MiddleWare* fMiddleWare;
    Master*     fMaster;
    SYNTH_T     fSynth;

    bool  fIsActive;
    float fParameters[kParamCount];

    CarlaMutex        fMutex;
    ZynAddSubFxThread fThread;

    static uint getZynParameterFromIndex(const uint index)
    {
        switch (index)
        {
        case kParamFilterCutoff:
            return C_filtercutoff;
        case kParamFilterQ:
            return C_filterq;
        case kParamBandwidth:
            return C_bandwidth;
        case kParamModAmp:
            return C_fmamp;
        case kParamResCenter:
            return C_resonance_center;
        case kParamResBandwidth:
            return C_resonance_bandwidth;
        case kParamCount:
            return C_NULL;
        }

        return C_NULL;
    }

    // -------------------------------------------------------------------

    void _initMaster()
    {
        fMiddleWare = new MiddleWare(fSynth);
        fMaster     = fMiddleWare->spawnMaster();
        fThread.setMaster(fMaster);
        fThread.startThread();

        for (int i=0; i<NUM_MIDI_PARTS; ++i)
        {
            fMaster->partonoff(i, 1);
            fMaster->part[i]->SetController(C_filtercutoff,        static_cast<int>(fParameters[kParamFilterCutoff]));
            fMaster->part[i]->SetController(C_filterq,             static_cast<int>(fParameters[kParamFilterQ]));
            fMaster->part[i]->SetController(C_bandwidth,           static_cast<int>(fParameters[kParamBandwidth]));
            fMaster->part[i]->SetController(C_fmamp,               static_cast<int>(fParameters[kParamModAmp]));
            fMaster->part[i]->SetController(C_resonance_center,    static_cast<int>(fParameters[kParamResCenter]));
            fMaster->part[i]->SetController(C_resonance_bandwidth, static_cast<int>(fParameters[kParamResBandwidth]));
        }
    }

    void _deleteMaster()
    {
        //ensure that everything has stopped
        fThread.stopThread(-1);

        fMaster = nullptr;
        delete fMiddleWare;
        fMiddleWare = nullptr;
    }

    // -------------------------------------------------------------------

public:
    static NativePluginHandle _instantiate(const NativeHostDescriptor* host)
    {
        static bool needsInit = true;

        if (needsInit)
        {
            needsInit = false;
            config.init();

            sprng(static_cast<prng_t>(std::time(nullptr)));

            // FIXME - kill this
            denormalkillbuf = new float[8192];
            for (int i=0; i < 8192; ++i)
                denormalkillbuf[i] = (RND - 0.5f) * 1e-16f;
        }

        return new ZynAddSubFxPlugin(host);
    }

    static void _cleanup(NativePluginHandle handle)
    {
        delete (ZynAddSubFxPlugin*)handle;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor zynaddsubfxDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_SYNTH,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
#ifdef HAVE_ZYN_UI_DEPS
                                                  |NATIVE_PLUGIN_HAS_UI
#endif
                                                  |NATIVE_PLUGIN_USES_MULTI_PROGS
                                                  |NATIVE_PLUGIN_USES_STATE),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_CONTROL_CHANGES
                                                     |NATIVE_PLUGIN_SUPPORTS_NOTE_AFTERTOUCH
                                                     |NATIVE_PLUGIN_SUPPORTS_PITCHBEND
                                                     |NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ ZynAddSubFxPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_zynaddsubfx_synth();

CARLA_EXPORT
void carla_register_native_plugin_zynaddsubfx_synth()
{
    carla_register_native_plugin(&zynaddsubfxDesc);
}

// -----------------------------------------------------------------------
