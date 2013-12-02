/*
 * Carla Native Plugins
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

// for UINT32_MAX
#define __STDC_LIMIT_MACROS
#include <cstdint>

#include "CarlaNative.hpp"
#include "CarlaMIDI.h"
#include "CarlaThread.hpp"
#include "List.hpp"

#include "zynaddsubfx/DSP/FFTwrapper.h"
#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Part.h"
#include "zynaddsubfx/Misc/Util.h"

#ifdef WANT_ZYNADDSUBFX_UI
# ifdef override
#  define override_hack
#  undef override
# endif

# include "zynaddsubfx/UI/common.H"
# include "zynaddsubfx/UI/MasterUI.h"
# include <FL/Fl_Shared_Image.H>
# include <FL/Fl_Tiled_Image.H>
# include <FL/Fl_Theme.H>

# ifdef override_hack
#  define override
#  undef override_hack
# endif
#endif

#include <ctime>
#include <set>
#include <string>

#ifdef USE_JUCE
#include "juce_audio_basics.h"
using juce::FloatVectorOperations;
#endif

#ifdef WANT_ZYNADDSUBFX_UI
static Fl_Tiled_Image* gModuleBackdrop = nullptr;
static CarlaString gPixmapPath;
extern CarlaString gUiPixmapPath;

void set_module_parameters(Fl_Widget* o)
{
    CARLA_ASSERT(gModuleBackdrop != nullptr);

    o->box(FL_DOWN_FRAME);
    o->align(o->align() | FL_ALIGN_IMAGE_BACKDROP);
    o->color(FL_BLACK);
    o->labeltype(FL_SHADOW_LABEL);

    if (gModuleBackdrop != nullptr)
        o->image(gModuleBackdrop);
}
#endif

// -----------------------------------------------------------------------

class ZynAddSubFxPrograms
{
public:
    ZynAddSubFxPrograms()
        : fInitiated(false)
    {
    }

    ~ZynAddSubFxPrograms()
    {
        if (! fInitiated)
            return;

        for (auto it = fPrograms.begin(); it.valid(); it.next())
        {
            const ProgramInfo*& pInfo(*it);
            delete pInfo;
        }

        fPrograms.clear();

        FFT_cleanup();
    }

    void init()
    {
        if (fInitiated)
            return;
        fInitiated = true;

        fPrograms.append(new ProgramInfo(0, 0, "default"));

        Master& master(Master::getInstance());

        pthread_mutex_lock(&master.mutex);

        // refresh banks
        master.bank.rescanforbanks();

        for (uint32_t i=0, size = master.bank.banks.size(); i < size; ++i)
        {
            if (master.bank.banks[i].dir.empty())
                continue;

            master.bank.loadbank(master.bank.banks[i].dir);

            for (unsigned int instrument = 0; instrument < BANK_SIZE; ++instrument)
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

    uint32_t count()
    {
        return fPrograms.count();
    }

    const NativeMidiProgram* getInfo(const uint32_t index)
    {
        if (index >= fPrograms.count())
            return nullptr;

        const ProgramInfo*& pInfo(fPrograms.getAt(index));

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

      ProgramInfo(uint32_t bank_, uint32_t prog_, const char* name_)
        : bank(bank_),
          prog(prog_),
          name(carla_strdup(name_)) {}

      ~ProgramInfo()
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
    NativeMidiProgram fRetProgram;
    List<const ProgramInfo*> fPrograms;

    CARLA_DECLARE_NON_COPY_CLASS(ZynAddSubFxPrograms)
};

static ZynAddSubFxPrograms sPrograms;

// -----------------------------------------------------------------------

class ZynAddSubFxInstanceCount
{
public:
    ZynAddSubFxInstanceCount()
        : fCount(0)
    {
    }

    ~ZynAddSubFxInstanceCount()
    {
        CARLA_ASSERT(fCount == 0);
    }

    void addOne(const NativeHostDescriptor* const host)
    {
        if (fCount++ == 0)
        {
            CARLA_ASSERT(synth == nullptr);
            CARLA_ASSERT(denormalkillbuf == nullptr);

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
    }

    void removeOne()
    {
        if (--fCount == 0)
        {
            CARLA_ASSERT(synth != nullptr);
            CARLA_ASSERT(denormalkillbuf != nullptr);

            Master::deleteInstance();

            delete[] denormalkillbuf;
            denormalkillbuf = nullptr;

            delete synth;
            synth = nullptr;
        }
    }

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
        synth->buffersize = host->get_buffer_size(host->handle);
        synth->samplerate = host->get_sample_rate(host->handle);
        synth->alias();

        config.init();
        config.cfg.SoundBufferSize = synth->buffersize;
        config.cfg.SampleRate      = synth->samplerate;
        config.cfg.GzipCompression = 0;

        sprng(std::time(nullptr));

        denormalkillbuf = new float[synth->buffersize];
        for (int i=0; i < synth->buffersize; ++i)
            denormalkillbuf[i] = (RND - 0.5f) * 1e-16;

        Master::getInstance();
    }

    void maybeReinit(const NativeHostDescriptor* const host)
    {
        if (host->get_buffer_size(host->handle) == static_cast<uint32_t>(synth->buffersize) &&
            host->get_sample_rate(host->handle) == static_cast<double>(synth->samplerate))
            return;

        reinit(host);
    }

private:
    int fCount;

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
          fNextProgram(0)
    {
    }

    ~ZynAddSubFxThread()
    {
#ifdef WANT_ZYNADDSUBFX_UI
        // must be closed by now
        CARLA_ASSERT(fUi == nullptr);
#endif
    }

    void loadProgramLater(const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        fNextChannel = channel;
        fNextBank    = bank;
        fNextProgram = program;
        fChangeProgram = true;
    }

    void stopLoadProgramLater()
    {
        fChangeProgram = false;
        fNextChannel = 0;
        fNextBank    = 0;
        fNextProgram = 0;
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
        while (! shouldExit())
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

                    Fl_Dial::default_style(Fl_Dial::PIXMAP_DIAL);

                    if (Fl_Shared_Image* const img = Fl_Shared_Image::get(gPixmapPath + "knob.png"))
                        Fl_Dial::default_image(img);

                    if (Fl_Shared_Image* const img = Fl_Shared_Image::get(gPixmapPath + "window_backdrop.png"))
                        Fl::scheme_bg(new Fl_Tiled_Image(img));

                    if(Fl_Shared_Image* const img = Fl_Shared_Image::get(gPixmapPath + "module_backdrop.png"))
                        gModuleBackdrop = new Fl_Tiled_Image(img);

                    Fl::background(50, 50, 50);
                    Fl::background2(70, 70, 70);
                    Fl::foreground(255, 255, 255);

                    Fl_Theme::set("Cairo");
                }

                CARLA_ASSERT(fUi == nullptr);

                if (fUi == nullptr)
                {
                    fUiClosed = 0;
                    fUi = new MasterUI(fMaster, &fUiClosed);
                    fUi->masterwindow->label(kHost->uiName);
                    fUi->showUI();
                }
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
        if (shouldExit() || fUi != nullptr)
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
};

// -----------------------------------------------------------------------

class ZynAddSubFxPlugin : public NativePluginClass
{
public:
    ZynAddSubFxPlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fMaster(new Master()),
          fSampleRate(getSampleRate()),
          fIsActive(false),
          fThread(fMaster, host)
    {
        fThread.start();

        for (int i = 0; i < NUM_MIDI_PARTS; ++i)
            fMaster->partonoff(i, 1);

        sPrograms.init();
    }

    ~ZynAddSubFxPlugin() override
    {
        deleteMaster();
    }

protected:
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
        }
        else
            fThread.loadProgramLater(channel, bank, program);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        if (std::strcmp(key, "CarlaAlternateFile1") == 0) // xmz
            fMaster->loadXML(value);
        else if (std::strcmp(key, "CarlaAlternateFile2") == 0) // xiz
            fMaster->part[0]->loadXMLinstrument(value);
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
        if (pthread_mutex_trylock(&fMaster->mutex) != 0)
        {
#ifdef USE_JUCE
            FloatVectorOperations::clear(outBuffer[0], frames);
            FloatVectorOperations::clear(outBuffer[1], frames);
#else
            carla_zeroFloat(outBuffer[0], frames);
            carla_zeroFloat(outBuffer[1], frames);
#endif
            return;
        }

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const NativeMidiEvent* const midiEvent(&midiEvents[i]);

            const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);
            const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent->data);

            if (MIDI_IS_STATUS_NOTE_OFF(status))
            {
                const uint8_t note = midiEvent->data[1];

                fMaster->noteOff(channel, note);
            }
            else if (MIDI_IS_STATUS_NOTE_ON(status))
            {
                const uint8_t note = midiEvent->data[1];
                const uint8_t velo = midiEvent->data[2];

                fMaster->noteOn(channel, note, velo);
            }
            else if (MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status))
            {
                const uint8_t note     = midiEvent->data[1];
                const uint8_t pressure = midiEvent->data[2];

                fMaster->polyphonicAftertouch(channel, note, pressure);
            }
            else if (MIDI_IS_STATUS_CONTROL_CHANGE(status))
            {
                const uint8_t control = midiEvent->data[1];
                const uint8_t value   = midiEvent->data[2];

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
        fMaster->putalldata((char*)data, 0);
        fMaster->applyparameters(true);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    // TODO - save&load current state

    void bufferSizeChanged(const uint32_t) final
    {
        deleteMaster();
        sInstanceCount.maybeReinit(getHostHandle());
        initMaster();
    }

    void sampleRateChanged(const double sampleRate) final
    {
        fSampleRate = sampleRate;
        deleteMaster();
        sInstanceCount.maybeReinit(getHostHandle());
        initMaster();
    }

    void initMaster()
    {
        fMaster = new Master();
        fThread.setMaster(fMaster);
        fThread.start();

        for (int i = 0; i < NUM_MIDI_PARTS; ++i)
            fMaster->partonoff(i, 1);
    }

    void deleteMaster()
    {
        //ensure that everything has stopped
        pthread_mutex_lock(&fMaster->mutex);
        pthread_mutex_unlock(&fMaster->mutex);
        fThread.stop(-1);

        delete fMaster;
        fMaster = nullptr;
    }

#ifdef WANT_ZYNADDSUBFX_UI
    void uiNameChanged(const char* const uiName) override
    {
        fThread.uiChangeName(uiName);
    }
#endif

    // -------------------------------------------------------------------

private:
    Master*  fMaster;
    unsigned fSampleRate;
    bool     fIsActive;

    ZynAddSubFxThread fThread;

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
    /* category  */ PLUGIN_CATEGORY_SYNTH,
#ifdef WANT_ZYNADDSUBFX_UI
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_HAS_UI|PLUGIN_USES_STATE),
#else
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_USES_STATE),
#endif
    /* supports  */ static_cast<NativePluginSupports>(PLUGIN_SUPPORTS_CONTROL_CHANGES|PLUGIN_SUPPORTS_NOTE_AFTERTOUCH|PLUGIN_SUPPORTS_PITCHBEND|PLUGIN_SUPPORTS_ALL_SOUND_OFF),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_zynaddsubfx_synth()
{
    carla_register_native_plugin(&zynaddsubfxDesc);
}

// -----------------------------------------------------------------------
