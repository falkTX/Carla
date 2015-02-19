/*
 * Carla Native Plugins
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"
#include "CarlaMIDI.h"
#include "CarlaThread.hpp"
#include "LinkedList.hpp"

#include "CarlaMathUtils.hpp"

#include "DSP/FFTwrapper.h"
#include "Misc/Master.h"
#include "Misc/Part.h"
#include "Misc/Util.h"

#ifdef HAVE_ZYN_UI_DEPS
# define WANT_ZYNADDSUBFX_UI
#endif

#ifdef WANT_ZYNADDSUBFX_UI
# ifdef override
#  define override_hack
#  undef override
# endif

# include "UI/MasterUI.h"
# include <FL/Fl_Shared_Image.H>
# include <FL/Fl_Tiled_Image.H>
# ifdef NTK_GUI
#  include <FL/Fl_Theme.H>
# endif

# ifdef override_hack
#  define override
#  undef override_hack
# endif
#endif

#include <ctime>
#include <set>
#include <string>

#include "juce_audio_basics.h"
using juce::FloatVectorOperations;

#ifdef WANT_ZYNADDSUBFX_UI
static CarlaString gPixmapPath;
extern CarlaString gUiPixmapPath;
# ifdef NTK_GUI
static Fl_Tiled_Image* gModuleBackdrop = nullptr;
# endif

void set_module_parameters(Fl_Widget* o)
{
    o->box(FL_DOWN_FRAME);
    o->align(o->align() | FL_ALIGN_IMAGE_BACKDROP);
    o->color(FL_BLACK);
    o->labeltype(FL_SHADOW_LABEL);

# ifdef NTK_GUI
    CARLA_SAFE_ASSERT_RETURN(gModuleBackdrop != nullptr,);
    o->image(gModuleBackdrop);
# endif
}
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

        Master& master(Master::getInstance());

        pthread_mutex_lock(&master.mutex);

        fPrograms.append(new ProgramInfo(0, 0, "default"));

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

        pthread_mutex_unlock(&master.mutex);
    }

    void load(Master* const master, const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        if (bank == 0)
        {
            pthread_mutex_lock(&master->mutex);

            master->partonoff(channel, 1);
            master->part[channel]->defaults();
            master->part[channel]->applyparameters(false);

            pthread_mutex_unlock(&master->mutex);

            return;
        }

        const std::string& bankdir(master->bank.banks[bank-1].dir);

        if (! bankdir.empty())
        {
            pthread_mutex_lock(&master->mutex);

            master->partonoff(channel, 1);

            master->bank.loadbank(bankdir);
            master->bank.loadfromslot(program, master->part[channel]);

            master->part[channel]->applyparameters(false);

            pthread_mutex_unlock(&master->mutex);
        }
    }

    uint32_t count() const noexcept
    {
        return static_cast<uint32_t>(fPrograms.count());
    }

    const NativeMidiProgram* getInfo(const uint32_t index) const noexcept
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

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ZynAddSubFxPrograms)
};

static ZynAddSubFxPrograms sPrograms;

// -----------------------------------------------------------------------

class ZynAddSubFxInstanceCount
{
public:
    ZynAddSubFxInstanceCount()
        : fCount(0),
          fMutex() {}

    ~ZynAddSubFxInstanceCount()
    {
        CARLA_SAFE_ASSERT(fCount == 0);
    }

    void addOne(const NativeHostDescriptor* const host)
    {
        if (fCount++ != 0)
            return;

        const CarlaMutexLocker cml(fMutex);

        CARLA_SAFE_ASSERT(synth == nullptr);
        CARLA_SAFE_ASSERT(denormalkillbuf == nullptr);

        reinit(host);

#ifdef WANT_ZYNADDSUBFX_UI
        if (gPixmapPath.isEmpty())
        {
            gPixmapPath   = host->resourceDir;
            gPixmapPath  += "/zynaddsubfx/";
            gUiPixmapPath = gPixmapPath;
        }
#endif
    }

    void removeOne()
    {
        if (--fCount != 0)
            return;

        const CarlaMutexLocker cml(fMutex);

        CARLA_SAFE_ASSERT(synth != nullptr);
        CARLA_SAFE_ASSERT(denormalkillbuf != nullptr);

        Master::deleteInstance();

        delete[] denormalkillbuf;
        denormalkillbuf = nullptr;

        delete synth;
        synth = nullptr;
    }

    void maybeReinit(const NativeHostDescriptor* const host)
    {
        if (static_cast< int>(host->get_buffer_size(host->handle)) == synth->buffersize &&
            static_cast<uint>(host->get_sample_rate(host->handle)) == synth->samplerate)
            return;

        const CarlaMutexLocker cml(fMutex);

        reinit(host);
    }

    CarlaMutex& getLock() noexcept
    {
        return fMutex;
    }

private:
    int fCount;
    CarlaMutex fMutex;

    void reinit(const NativeHostDescriptor* const host)
    {
        Master::deleteInstance();

        if (denormalkillbuf != nullptr)
        {
            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;
        }

        if (synth != nullptr)
        {
            delete synth;
            synth = nullptr;
        }

        synth = new SYNTH_T();
        synth->buffersize = static_cast<int>(host->get_buffer_size(host->handle));
        synth->samplerate = static_cast<uint>(host->get_sample_rate(host->handle));

        if (synth->buffersize > 32)
            synth->buffersize = 32;

        synth->alias();

        config.init();
        config.cfg.SoundBufferSize = synth->buffersize;
        config.cfg.SampleRate      = static_cast<int>(synth->samplerate);
        config.cfg.GzipCompression = 0;

        sprng(static_cast<prng_t>(std::time(nullptr)));

        denormalkillbuf = new float[synth->buffersize];
        for (int i=0; i < synth->buffersize; ++i)
            denormalkillbuf[i] = (RND - 0.5f) * 1e-16f;

        Master::getInstance();
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ZynAddSubFxInstanceCount)
};

static ZynAddSubFxInstanceCount sInstanceCount;

// -----------------------------------------------------------------------

class ZynAddSubFxThread : public CarlaThread
{
public:
    ZynAddSubFxThread(Master* const master, const NativeHostDescriptor* const host)
        : CarlaThread("ZynAddSubFxThread"),
          fMaster(master),
          kHost(host),
#ifdef WANT_ZYNADDSUBFX_UI
          fUi(nullptr),
          fUiClosed(0),
          fNextUiAction(-1),
#endif
          fChangeProgram(false),
          fNextChannel(0),
          fNextBank(0),
          fNextProgram(0) {}

    ~ZynAddSubFxThread()
    {
#ifdef WANT_ZYNADDSUBFX_UI
        // must be closed by now
        CARLA_SAFE_ASSERT(fUi == nullptr);
#endif
    }

    void loadProgramLater(const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        fNextChannel   = channel;
        fNextBank      = bank;
        fNextProgram   = program;
        fChangeProgram = true;
    }

    void stopLoadProgramLater()
    {
        fChangeProgram = false;
        fNextChannel   = 0;
        fNextBank      = 0;
        fNextProgram   = 0;
    }

    void setMaster(Master* const master)
    {
        fMaster = master;
    }

#ifdef WANT_ZYNADDSUBFX_UI
    void uiHide()
    {
        fNextUiAction = 0;
    }

    void uiShow()
    {
        fNextUiAction = 1;
    }

    void uiRepaint()
    {
        if (fUi != nullptr)
            fNextUiAction = 2;
    }

    void uiChangeName(const char* const name)
    {
        if (fUi != nullptr)
        {
            Fl::lock();
            fUi->masterwindow->label(name);
            Fl::unlock();
        }
    }
#endif

protected:
    void run() override
    {
        while (! shouldThreadExit())
        {
#ifdef WANT_ZYNADDSUBFX_UI
            Fl::lock();

            if (fNextUiAction == 2) // repaint
            {
                CARLA_ASSERT(fUi != nullptr);

                if (fUi != nullptr)
                    fUi->refresh_master_ui();
            }
            else if (fNextUiAction == 1) // init/show
            {
                static bool initialized = false;

                if (! initialized)
                {
                    initialized = true;
                    fl_register_images();

# ifdef NTK_GUI
                    Fl_Dial::default_style(Fl_Dial::PIXMAP_DIAL);

                    if (Fl_Shared_Image* const img = Fl_Shared_Image::get(gPixmapPath + "knob.png"))
                        Fl_Dial::default_image(img);

                    if (Fl_Shared_Image* const img = Fl_Shared_Image::get(gPixmapPath + "window_backdrop.png"))
                        Fl::scheme_bg(new Fl_Tiled_Image(img));

                    if (Fl_Shared_Image* const img = Fl_Shared_Image::get(gPixmapPath + "module_backdrop.png"))
                        gModuleBackdrop = new Fl_Tiled_Image(img);

                    Fl::background(50, 50, 50);
                    Fl::background2(70, 70, 70);
                    Fl::foreground(255, 255, 255);

                    Fl_Theme::set("Cairo");
# endif
                }

                if (fUi == nullptr)
                {
                    fUiClosed = 0;
                    fUi = new MasterUI(fMaster, &fUiClosed);
                    fUi->masterwindow->label(kHost->uiName);
                    fUi->showUI();
                }
                else
                    fUi->showUI();
            }
            else if (fNextUiAction == 0) // close
            {
                CARLA_ASSERT(fUi != nullptr);

                if (fUi != nullptr)
                {
                    delete fUi;
                    fUi = nullptr;
                }
            }

            fNextUiAction = -1;

            if (fUiClosed != 0)
            {
                fUiClosed     = 0;
                fNextUiAction = 0;
                kHost->ui_closed(kHost->handle);
            }

            Fl::check();
            Fl::unlock();
#endif

            if (fChangeProgram)
            {
                fChangeProgram = false;
                sPrograms.load(fMaster, fNextChannel, fNextBank, fNextProgram);
                fNextChannel = 0;
                fNextBank    = 0;
                fNextProgram = 0;

#ifdef WANT_ZYNADDSUBFX_UI
                if (fUi != nullptr)
                {
                    Fl::lock();
                    fUi->refresh_master_ui();
                    Fl::unlock();
                }
#endif

                carla_msleep(15);
            }
            else
            {
                carla_msleep(30);
            }
        }

#ifdef WANT_ZYNADDSUBFX_UI
        if (fUi != nullptr)
        {
            Fl::lock();
            delete fUi;
            fUi = nullptr;
            Fl::check();
            Fl::unlock();
        }
#endif
    }

private:
    Master* fMaster;
    const NativeHostDescriptor* const kHost;

#ifdef WANT_ZYNADDSUBFX_UI
    MasterUI* fUi;
    int       fUiClosed;
    volatile int fNextUiAction;
#endif

    volatile bool     fChangeProgram;
    volatile uint8_t  fNextChannel;
    volatile uint32_t fNextBank;
    volatile uint32_t fNextProgram;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ZynAddSubFxThread)
};

// -----------------------------------------------------------------------

class ZynAddSubFxPlugin : public NativePluginClass
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
        : NativePluginClass(host),
          fMaster(nullptr),
          fSampleRate(static_cast<uint>(getSampleRate())),
          fIsActive(false),
          fThread(nullptr, host),
          leakDetector_ZynAddSubFxPlugin()
    {
        // init parameters to default
        fParameters[kParamFilterCutoff] = 64.0f;
        fParameters[kParamFilterQ]      = 64.0f;
        fParameters[kParamBandwidth]    = 64.0f;
        fParameters[kParamModAmp]       = 127.0f;
        fParameters[kParamResCenter]    = 64.0f;
        fParameters[kParamResBandwidth] = 64.0f;

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

    uint32_t getMidiProgramCount() const override
    {
        return sPrograms.count();
    }

    const NativeMidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        return sPrograms.getInfo(index);
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) final
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount,);

        const uint zynIndex(getZynParameterFromIndex(index));
        CARLA_SAFE_ASSERT_RETURN(zynIndex != C_NULL,);

        fParameters[index] = value;

        for (int npart=0; npart<NUM_MIDI_PARTS; ++npart)
        {
            if (fMaster->part[npart] != nullptr && fMaster->part[npart]->Penabled != 0)
                fMaster->part[npart]->SetController(zynIndex, static_cast<int>(value));
        }
    }

    void setMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        if (bank >= fMaster->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        if (isOffline() || ! fIsActive)
        {
            sPrograms.load(fMaster, channel, bank, program);
#ifdef WANT_ZYNADDSUBFX_UI
            fThread.uiRepaint();
#endif
            return;
        }

        fThread.loadProgramLater(channel, bank, program);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        pthread_mutex_lock(&fMaster->mutex);

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

        fMaster->applyparameters(false);

        pthread_mutex_unlock(&fMaster->mutex);
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
        const CarlaMutexTryLocker cmtl(sInstanceCount.getLock());

        if (cmtl.wasNotLocked() || pthread_mutex_trylock(&fMaster->mutex) != 0)
        {
            FloatVectorOperations::clear(outBuffer[0], static_cast<int>(frames));
            FloatVectorOperations::clear(outBuffer[1], static_cast<int>(frames));
            return;
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
                if (getZynParameterFromIndex(midiEvent->data[1]) == C_NULL)
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

        fMaster->GetAudioOutSamples(frames, fSampleRate, outBuffer[0], outBuffer[1]);

        pthread_mutex_unlock(&fMaster->mutex);
    }

#ifdef WANT_ZYNADDSUBFX_UI
    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
            fThread.uiShow();
        else
            fThread.uiHide();
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        config.save();

        char* data = nullptr;
        fMaster->getalldata(&data);
        return data;
    }

    void setState(const char* const data) override
    {
        fThread.stopLoadProgramLater();
        fMaster->putalldata(const_cast<char*>(data), 0);
        fMaster->applyparameters(true);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    void bufferSizeChanged(const uint32_t) final
    {
        char* const state(getState());

        _deleteMaster();
        sInstanceCount.maybeReinit(getHostHandle());
        _initMaster();

        if (state != nullptr)
        {
            fMaster->putalldata(state, 0);
            fMaster->applyparameters(true);
            std::free(state);
        }
    }

    void sampleRateChanged(const double sampleRate) final
    {
        fSampleRate = static_cast<uint>(sampleRate);

        char* const state(getState());

        _deleteMaster();
        sInstanceCount.maybeReinit(getHostHandle());
        _initMaster();

        if (state != nullptr)
        {
            fMaster->putalldata(state, 0);
            fMaster->applyparameters(true);
            std::free(state);
        }
    }

#ifdef WANT_ZYNADDSUBFX_UI
    void uiNameChanged(const char* const uiName) override
    {
        fThread.uiChangeName(uiName);
    }
#endif

    // -------------------------------------------------------------------

private:
    Master* fMaster;
    uint    fSampleRate;
    bool    fIsActive;
    float   fParameters[kParamCount];

    ZynAddSubFxThread fThread;

    /*
    static Parameters getParameterFromZynIndex(const MidiControllers index)
    {
        switch (index)
        {
        case C_filtercutoff:
            return kParamFilterCutoff;
        case C_filterq:
            return kParamFilterQ;
        case C_bandwidth:
            return kParamBandwidth;
        case C_fmamp:
            return kParamModAmp;
        case C_resonance_center:
            return kParamResCenter;
        case C_resonance_bandwidth:
            return kParamResBandwidth;
        default:
            return kParamCount;
        }
    }
    */

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
        fMaster = new Master();
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
        pthread_mutex_lock(&fMaster->mutex);
        pthread_mutex_unlock(&fMaster->mutex);
        fThread.stopThread(-1);

        delete fMaster;
        fMaster = nullptr;
    }

    // -------------------------------------------------------------------

public:
    static NativePluginHandle _instantiate(const NativeHostDescriptor* host)
    {
        sInstanceCount.addOne(host);
        return new ZynAddSubFxPlugin(host);
    }

    static void _cleanup(NativePluginHandle handle)
    {
        delete (ZynAddSubFxPlugin*)handle;
        sInstanceCount.removeOne();
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor zynaddsubfxDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_SYNTH,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
#ifdef WANT_ZYNADDSUBFX_UI
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
