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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

// for UINT32_MAX
#define __STDC_LIMIT_MACROS
#include <cstdint>

#include "CarlaNative.hpp"
#include "CarlaMIDI.h"
#include "CarlaString.hpp"
#include "RtList.hpp"

#include <QtCore/QThread>

#include "zynaddsubfx/Misc/Master.h"
#include "zynaddsubfx/Misc/Part.h"
#include "zynaddsubfx/Misc/Util.h"

#ifdef WANT_ZYNADDSUBFX_UI
// FIXME
# ifdef override
#  define override_hack
#  undef override
# endif

# include "zynaddsubfx/UI/common.H"
# include "zynaddsubfx/UI/MasterUI.h"
# include <FL/Fl_Shared_Image.H>
# include <FL/Fl_Tiled_Image.H>
# include <FL/Fl_Dial.H>
# include <FL/Fl_Theme.H>

# ifdef override_hack
#  define override
#  undef override_hack
# endif
#endif

#include <ctime>

#include <set>
#include <string>

// Dummy variables and functions for linking purposes
const char* instance_name = nullptr;
class WavFile;
namespace Nio {
   bool start(void){return 1;}
   void stop(void){}
   bool setSource(std::string){return true;}
   bool setSink(std::string){return true;}
   std::set<std::string> getSources(void){return std::set<std::string>();}
   std::set<std::string> getSinks(void){return std::set<std::string>();}
   std::string getSource(void){return "";}
   std::string getSink(void){return "";}
   void waveNew(WavFile*){}
   void waveStart(void){}
   void waveStop(void){}
   void waveEnd(void){}
}

SYNTH_T* synth = nullptr;

#ifdef WANT_ZYNADDSUBFX_UI
#define PIXMAP_PATH "/resources/zynaddsubfx/"

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

class ZynAddSubFxPlugin : public PluginDescriptorClass
{
public:
    enum Parameters {
        PARAMETER_COUNT = 0
    };

    ZynAddSubFxPlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host),
          fMaster(new Master()),
          fSampleRate(getSampleRate()),
          fIsActive(false),
          fIsOffline(false),
          fThread(fMaster, host)
    {
        fThread.start();
        maybeInitPrograms(fMaster);

        for (int i = 0; i < NUM_MIDI_PARTS; ++i)
            fMaster->partonoff(i, 1);
    }

    ~ZynAddSubFxPlugin() override
    {
        //ensure that everything has stopped
        pthread_mutex_lock(&fMaster->mutex);
        pthread_mutex_unlock(&fMaster->mutex);
        fThread.stop();
        fThread.wait();

        delete fMaster;
        fMaster = nullptr;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() override
    {
        return PARAMETER_COUNT;
    }

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        CARLA_ASSERT(index < getParameterCount());

        //if (index >= PARAMETER_COUNT)
        return nullptr;

        static Parameter param;

        param.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        param.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        param.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
#if 0
        case PARAMETER_MASTER:
            param.hints = PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE;
            param.name  = "Master Volume";
            param.unit  = nullptr;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.ranges.def = 100.0f;
            break;
#endif
        }

        return &param;
    }

    float getParameterValue(const uint32_t index) override
    {
        CARLA_ASSERT(index < getParameterCount());

        switch (index)
        {
#if 0
        case PARAMETER_MASTER:
            return fMaster->Pvolume;
#endif
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() override
    {
        return sPrograms.count();
    }

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= sPrograms.count())
            return nullptr;

        const ProgramInfo* const pInfo(sPrograms.getAt(index));

        static MidiProgram midiProgram;
        midiProgram.bank    = pInfo->bank;
        midiProgram.program = pInfo->prog;
        midiProgram.name    = pInfo->name;

        return &midiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(index < getParameterCount());

        switch (index)
        {
        }

        return;

        // unused, TODO
        (void)value;
    }

    void setMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        if (bank >= fMaster->bank.banks.size())
            return;
        if (program >= BANK_SIZE)
            return;

        if (fIsOffline || ! fIsActive)
        {
            loadProgram(fMaster, channel, bank, program);
#ifdef WANT_ZYNADDSUBFX_UI
            fThread.uiRepaint();
#endif
        }
        else
            fThread.loadLater(channel, bank, program);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

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

    void process(float**, float** const outBuffer, const uint32_t frames, const uint32_t midiEventCount, const MidiEvent* const midiEvents) override
    {
        if (pthread_mutex_trylock(&fMaster->mutex) != 0)
        {
            carla_zeroFloat(outBuffer[0], frames);
            carla_zeroFloat(outBuffer[1], frames);
            return;
        }

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const MidiEvent* const midiEvent(&midiEvents[i]);

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

    char* getState() override
    {
        config.save();

        char* data = nullptr;
        fMaster->getalldata(&data);
        return data;
    }

    void setState(const char* const data) override
    {
        fThread.stopLoadLater();
        fMaster->putalldata((char*)data, 0);
        fMaster->applyparameters(true);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    intptr_t pluginDispatcher(const PluginDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr) override
    {
        switch (opcode)
        {
        case PLUGIN_OPCODE_NULL:
            break;
        case PLUGIN_OPCODE_BUFFER_SIZE_CHANGED:
            // TODO
            break;
        case PLUGIN_OPCODE_SAMPLE_RATE_CHANGED:
            // TODO
            break;
        case PLUGIN_OPCODE_UI_NAME_CHANGED:
#ifdef WANT_ZYNADDSUBFX_UI
            // TODO
#endif
            break;
        }

        return 0;

        // unused
        (void)index;
        (void)value;
        (void)ptr;
    }

    // -------------------------------------------------------------------

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
#endif
    };

    class ZynThread : public QThread
    {
    public:
        ZynThread(Master* const master, const HostDescriptor* const host)
            : fMaster(master),
              kHost(host),
#ifdef WANT_ZYNADDSUBFX_UI
              fUi(nullptr),
              fUiClosed(0),
              fNextUiAction(-1),
#endif
              fQuit(false),
              fChangeProgram(false),
              fNextChannel(0),
              fNextBank(0),
              fNextProgram(0)
        {
        }

        ~ZynThread()
        {
            // must be closed by now
#ifdef WANT_ZYNADDSUBFX_UI
            CARLA_ASSERT(fUi == nullptr);
#endif
            CARLA_ASSERT(fQuit);
        }

        void loadLater(const uint8_t channel, const uint32_t bank, const uint32_t program)
        {
            fNextChannel = channel;
            fNextBank    = bank;
            fNextProgram = program;
            fChangeProgram = true;
        }

        void stopLoadLater()
        {
            fChangeProgram = false;
            fNextChannel = 0;
            fNextBank    = 0;
            fNextProgram = 0;
        }

        void stop()
        {
            fQuit = true;
            quit();
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
#endif

    protected:
        void run() override
        {
            while (! fQuit)
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
                    loadProgram(fMaster, fNextChannel, fNextBank, fNextProgram);
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
            if (fQuit && fUi != nullptr)
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
        Master* const fMaster;
        const HostDescriptor* const kHost;

#ifdef WANT_ZYNADDSUBFX_UI
        MasterUI* fUi;
        int       fUiClosed;
        int       fNextUiAction;
#endif

        bool     fQuit;
        bool     fChangeProgram;
        uint8_t  fNextChannel;
        uint32_t fNextBank;
        uint32_t fNextProgram;
    };

    Master* const  fMaster;
    unsigned fSampleRate;
    bool fIsActive;
    bool fIsOffline;

    ZynThread fThread;

    static int sInstanceCount;
    static NonRtList<ProgramInfo*> sPrograms;

    static void maybeInitPrograms(Master* const master)
    {
        static bool doSearch = true;

        if (! doSearch)
            return;
        doSearch = false;

        sPrograms.append(new ProgramInfo(0, 0, "default"));

        pthread_mutex_lock(&master->mutex);

        // refresh banks
        master->bank.rescanforbanks();

        for (uint32_t i=0, size = master->bank.banks.size(); i < size; ++i)
        {
            if (master->bank.banks[i].dir.empty())
                continue;

            master->bank.loadbank(master->bank.banks[i].dir);

            for (unsigned int instrument = 0; instrument < BANK_SIZE; ++instrument)
            {
                const std::string insName(master->bank.getname(instrument));

                if (insName.empty() || insName[0] == '\0' || insName[0] == ' ')
                    continue;

                sPrograms.append(new ProgramInfo(i+1, instrument, insName.c_str()));
            }
        }

        pthread_mutex_unlock(&master->mutex);
    }

    static void loadProgram(Master* const master, const uint8_t channel, const uint32_t bank, const uint32_t program)
    {
        if (bank == 0)
        {
            pthread_mutex_lock(&master->mutex);

            master->part[channel]->defaults();
            master->part[channel]->applyparameters(false);
            master->partonoff(channel, 1);

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

public:
    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host)
    {
        if (sInstanceCount++ == 0)
        {
            CARLA_ASSERT(synth == nullptr);
            CARLA_ASSERT(denormalkillbuf == nullptr);

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

#ifdef WANT_ZYNADDSUBFX_UI
            if (gPixmapPath.isEmpty())
            {
                gPixmapPath   = host->resource_dir;
                gPixmapPath  += PIXMAP_PATH;
                gUiPixmapPath = gPixmapPath;
            }
#endif
        }

        return new ZynAddSubFxPlugin(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (ZynAddSubFxPlugin*)handle;

        if (--sInstanceCount == 0)
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

    static void _clearPrograms()
    {
        for (auto it = sPrograms.begin(); it.valid(); it.next())
        {
            ProgramInfo* const programInfo(*it);
            delete programInfo;
        }

        sPrograms.clear();
    }

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZynAddSubFxPlugin)
};

int ZynAddSubFxPlugin::sInstanceCount = 0;
NonRtList<ZynAddSubFxPlugin::ProgramInfo*> ZynAddSubFxPlugin::sPrograms;

static const struct ProgramsDestructor {
    ProgramsDestructor() {}
    ~ProgramsDestructor() {
        ZynAddSubFxPlugin::_clearPrograms();
    }
} _programsDestructor;

// -----------------------------------------------------------------------

static const PluginDescriptor zynaddsubfxDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
#ifdef WANT_ZYNADDSUBFX_UI
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_SYNTH|PLUGIN_HAS_GUI|PLUGIN_USES_STATE),
#else
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_SYNTH|PLUGIN_USES_STATE),
#endif
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ ZynAddSubFxPlugin::PARAMETER_COUNT,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_zynaddsubfx()
{
    carla_register_native_plugin(&zynaddsubfxDesc);
}

// -----------------------------------------------------------------------
