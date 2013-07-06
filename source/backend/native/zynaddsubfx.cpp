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

#include "zynaddsubfx/Effects/Alienwah.h"
#include "zynaddsubfx/Effects/Chorus.h"
#include "zynaddsubfx/Effects/Distorsion.h"
#include "zynaddsubfx/Effects/DynamicFilter.h"
#include "zynaddsubfx/Effects/Echo.h"
#include "zynaddsubfx/Effects/Phaser.h"
#include "zynaddsubfx/Effects/Reverb.h"

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

    const MidiProgram* getInfo(const uint32_t index)
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
#endif
    };

    bool fInitiated;
    MidiProgram fRetProgram;
    NonRtList<const ProgramInfo*> fPrograms;

    CARLA_DECLARE_NON_COPYABLE(ZynAddSubFxPrograms)
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

    void addOne(const HostDescriptor* const host)
    {
        if (fCount++ == 0)
        {
            CARLA_ASSERT(synth == nullptr);
            CARLA_ASSERT(denormalkillbuf == nullptr);

            reinit(host);

#ifdef WANT_ZYNADDSUBFX_UI
            if (gPixmapPath.isEmpty())
            {
                gPixmapPath   = host->resource_dir;
                gPixmapPath  += PIXMAP_PATH;
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

    void reinit(const HostDescriptor* const host)
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

    void maybeReinit(const HostDescriptor* const host)
    {
        if (host->get_buffer_size(host->handle) == static_cast<uint32_t>(synth->buffersize) &&
            host->get_sample_rate(host->handle) == static_cast<double>(synth->samplerate))
            return;

        reinit(host);
    }

private:
    int fCount;

    CARLA_DECLARE_NON_COPYABLE(ZynAddSubFxInstanceCount)
};

static ZynAddSubFxInstanceCount sInstanceCount;

// -----------------------------------------------------------------------

class ZynAddSubFxThread : public QThread
{
public:
    ZynAddSubFxThread(Master* const master, const HostDescriptor* const host)
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

    ~ZynAddSubFxThread()
    {
        // must be closed by now
#ifdef WANT_ZYNADDSUBFX_UI
        CARLA_ASSERT(fUi == nullptr);
#endif
        CARLA_ASSERT(fQuit);
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
    Master* fMaster;
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

// -----------------------------------------------------------------------

#define ZynPluginDescriptorClassEND(ClassName)                                      \
public:                                                                             \
    static PluginHandle _instantiate(const PluginDescriptor*, HostDescriptor* host) \
    {                                                                               \
        sInstanceCount.addOne(host);                                                \
        return new ClassName(host);                                                 \
    }                                                                               \
    static void _cleanup(PluginHandle handle)                                       \
    {                                                                               \
        delete (ClassName*)handle;                                                  \
        sInstanceCount.removeOne();                                                 \
    }

// -----------------------------------------------------------------------

class FxAbstractPlugin : public PluginDescriptorClass
{
protected:
    FxAbstractPlugin(const HostDescriptor* const host, const uint32_t paramCount, const uint32_t programCount)
        : PluginDescriptorClass(host),
          fEffect(nullptr),
          efxoutl(new float[synth->buffersize]),
          efxoutr(new float[synth->buffersize]),
          fFirstInit(true),
          kParamCount(paramCount-2), // volume and pan handled by host
          kProgramCount(programCount)
    {
        carla_zeroFloat(efxoutl, synth->buffersize);
        carla_zeroFloat(efxoutr, synth->buffersize);
    }

    ~FxAbstractPlugin() override
    {
        CARLA_ASSERT(fEffect != nullptr);

        if (efxoutl != nullptr)
        {
            delete[] efxoutl;
            efxoutl = nullptr;
        }

        if (efxoutr != nullptr)
        {
            delete[] efxoutr;
            efxoutr = nullptr;
        }

        if (fEffect != nullptr)
        {
            delete fEffect;
            fEffect = nullptr;
        }
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() final
    {
        return kParamCount;
    }

    float getParameterValue(const uint32_t index) final
    {
        return fEffect->getpar(index+2);
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() final
    {
        return kProgramCount;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) final
    {
        fEffect->changepar(index+2, value);
        fFirstInit = false;
    }

    void setMidiProgram(const uint8_t, const uint32_t, const uint32_t program) final
    {
        fEffect->setpreset(program);

        const float volume(float(fEffect->getpar(0))/127.0f);
        hostDispatcher(HOST_OPCODE_SET_VOLUME, 0, 0, nullptr, volume);

        const unsigned char pan(fEffect->getpar(1));
        const float panning(float(pan)/63.5f-1.0f);
        hostDispatcher(HOST_OPCODE_SET_PANNING, 0, 0, nullptr, (pan == 64) ? 0.0f : panning);

        fEffect->changepar(0, 127);
        fEffect->changepar(1, 64);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() final
    {
        fEffect->cleanup();

        if (fFirstInit)
        {
            fFirstInit = false;
            hostDispatcher(HOST_OPCODE_SET_DRYWET, 0, 0, nullptr, 0.5f);
        }
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const uint32_t, const MidiEvent* const) final
    {
        CARLA_ASSERT(synth->buffersize == static_cast<int>(frames));

        fEffect->out(Stereo<float*>(inBuffer[0], inBuffer[1]));

        carla_copyFloat(outBuffer[0], efxoutl, frames);
        carla_copyFloat(outBuffer[1], efxoutr, frames);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    intptr_t pluginDispatcher(const PluginDispatcherOpcode opcode, const int32_t, const intptr_t, void* const, const float) final
    {
        switch (opcode)
        {
        case PLUGIN_OPCODE_BUFFER_SIZE_CHANGED:
        {
            const uint32_t bufferSize(getBufferSize());
            delete[] efxoutl;
            delete[] efxoutr;
            efxoutl = new float[bufferSize];
            efxoutr = new float[bufferSize];
            carla_zeroFloat(efxoutl, synth->buffersize);
            carla_zeroFloat(efxoutr, synth->buffersize);
            *((float**)&fEffect->efxoutl) = efxoutl;
            *((float**)&fEffect->efxoutr) = efxoutr;
            // no break
        }
        case PLUGIN_OPCODE_SAMPLE_RATE_CHANGED:
            sInstanceCount.maybeReinit(hostHandle());
            break;
        default:
            break;
        }

        return 0;
    }

    Effect* fEffect;
    float*  efxoutl;
    float*  efxoutr;
    bool    fFirstInit;
    const uint32_t kParamCount;
    const uint32_t kProgramCount;
};

// -----------------------------------------------------------------------

class FxAlienWahPlugin : public FxAbstractPlugin
{
public:
    FxAlienWahPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 11, 4)
    {
        fEffect = new Alienwah(false, efxoutl, efxoutr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= kParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 70.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 62.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Depth";
            param.ranges.def = 60.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 105.0f;
            break;
        case 6:
            param.name = "Delay";
            param.ranges.def = 25.0f;
            param.ranges.max = 100.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 0.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Phase";
            param.ranges.def = 64.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "AlienWah1";
            break;
        case 1:
            midiProg.name = "AlienWah2";
            break;
        case 2:
            midiProg.name = "AlienWah3";
            break;
        case 3:
            midiProg.name = "AlienWah4";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    ZynPluginDescriptorClassEND(FxAlienWahPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxAlienWahPlugin)
};

// -----------------------------------------------------------------------

class FxChorusPlugin : public FxAbstractPlugin
{
public:
    FxChorusPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 12, 10)
    {
        fEffect = new Chorus(false, efxoutl, efxoutr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= kParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 50.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 90.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Depth";
            param.ranges.def = 40.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Delay";
            param.ranges.def = 85.0f;
            break;
        case 6:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 64.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 119.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Flange Mode";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 9:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Subtract Output";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Chorus1";
            break;
        case 1:
            midiProg.name = "Chorus2";
            break;
        case 2:
            midiProg.name = "Chorus3";
            break;
        case 3:
            midiProg.name = "Celeste1";
            break;
        case 4:
            midiProg.name = "Celeste2";
            break;
        case 5:
            midiProg.name = "Flange1";
            break;
        case 6:
            midiProg.name = "Flange2";
            break;
        case 7:
            midiProg.name = "Flange3";
            break;
        case 8:
            midiProg.name = "Flange4";
            break;
        case 9:
            midiProg.name = "Flange5";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    ZynPluginDescriptorClassEND(FxChorusPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxChorusPlugin)
};

// -----------------------------------------------------------------------

class FxDistortionPlugin : public FxAbstractPlugin
{
public:
    FxDistortionPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 11, 6)
    {
        fEffect = new Distorsion(false, efxoutl, efxoutr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= kParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[14];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 35.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Drive";
            param.ranges.def = 56.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Level";
            param.ranges.def = 70.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_USES_SCALEPOINTS;
            param.name = "Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 13.0f;
            param.scalePointCount = 14;
            param.scalePoints     = scalePoints;
            scalePoints[ 0].label = "Arctangent";
            scalePoints[ 1].label = "Asymmetric";
            scalePoints[ 2].label = "Pow";
            scalePoints[ 3].label = "Sine";
            scalePoints[ 4].label = "Quantisize";
            scalePoints[ 5].label = "Zigzag";
            scalePoints[ 6].label = "Limiter";
            scalePoints[ 7].label = "Upper Limiter";
            scalePoints[ 8].label = "Lower Limiter";
            scalePoints[ 9].label = "Inverse Limiter";
            scalePoints[10].label = "Clip";
            scalePoints[11].label = "Asym2";
            scalePoints[12].label = "Pow2";
            scalePoints[13].label = "sigmoid";
            scalePoints[ 0].value = 0.0f;
            scalePoints[ 1].value = 1.0f;
            scalePoints[ 2].value = 2.0f;
            scalePoints[ 3].value = 3.0f;
            scalePoints[ 4].value = 4.0f;
            scalePoints[ 5].value = 5.0f;
            scalePoints[ 6].value = 6.0f;
            scalePoints[ 7].value = 7.0f;
            scalePoints[ 8].value = 8.0f;
            scalePoints[ 9].value = 9.0f;
            scalePoints[10].value = 10.0f;
            scalePoints[11].value = 11.0f;
            scalePoints[12].value = 12.0f;
            scalePoints[13].value = 13.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Negate";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Low-Pass Filter";
            param.ranges.def = 96.0f;
            break;
        case 6:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "High-Pass Filter";
            param.ranges.def = 0.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Stereo";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Pre-Filtering";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Overdrive 1";
            break;
        case 1:
            midiProg.name = "Overdrive 2";
            break;
        case 2:
            midiProg.name = "A. Exciter 1";
            break;
        case 3:
            midiProg.name = "A. Exciter 2";
            break;
        case 4:
            midiProg.name = "Guitar Amp";
            break;
        case 5:
            midiProg.name = "Quantisize";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    ZynPluginDescriptorClassEND(FxDistortionPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxDistortionPlugin)
};

// -----------------------------------------------------------------------

class FxDynamicFilterPlugin : public FxAbstractPlugin
{
public:
    FxDynamicFilterPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 10, 5)
    {
        fEffect = new DynamicFilter(false, efxoutl, efxoutr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= kParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 80.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 64.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Depth";
            param.ranges.def = 0.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Amp sns";
            param.ranges.def = 90.0f;
            break;
        case 6:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Amp sns inv";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Amp Smooth";
            param.ranges.def = 60.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "WahWah";
            break;
        case 1:
            midiProg.name = "AutoWah";
            break;
        case 2:
            midiProg.name = "Sweep";
            break;
        case 3:
            midiProg.name = "VocalMorph1";
            break;
        case 4:
            midiProg.name = "VocalMorph2";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    ZynPluginDescriptorClassEND(FxDynamicFilterPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxDynamicFilterPlugin)
};

// -----------------------------------------------------------------------

class FxEchoPlugin : public FxAbstractPlugin
{
public:
    FxEchoPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 7, 9)
    {
        fEffect = new Echo(false, efxoutl, efxoutr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= kParamCount)
            return nullptr;

        static Parameter param;

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Delay";
            param.ranges.def = 35.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Delay";
            param.ranges.def = 64.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 30.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 59.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "High Damp";
            param.ranges.def = 0.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Echo 1";
            break;
        case 1:
            midiProg.name = "Echo 2";
            break;
        case 2:
            midiProg.name = "Echo 3";
            break;
        case 3:
            midiProg.name = "Simple Echo";
            break;
        case 4:
            midiProg.name = "Canyon";
            break;
        case 5:
            midiProg.name = "Panning Echo 1";
            break;
        case 6:
            midiProg.name = "Panning Echo 2";
            break;
        case 7:
            midiProg.name = "Panning Echo 3";
            break;
        case 8:
            midiProg.name = "Feedback Echo";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    ZynPluginDescriptorClassEND(FxEchoPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEchoPlugin)
};

// -----------------------------------------------------------------------

class FxPhaserPlugin : public FxAbstractPlugin
{
public:
    FxPhaserPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 15, 12)
    {
        fEffect = new Phaser(false, efxoutl, efxoutr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= kParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 36.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 64.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Depth";
            param.ranges.def = 110.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 64.0f;
            break;
        case 6:
            param.name = "Stages";
            param.ranges.def = 1.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 12.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross|Offset";
            param.ranges.def = 0.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Subtract Output";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 9:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Phase|Width";
            param.ranges.def = 20.0f;
            break;
        case 10:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Hyper";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 11:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Distortion";
            param.ranges.def = 0.0f;
            break;
        case 12:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Analog";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Phaser 1";
            break;
        case 1:
            midiProg.name = "Phaser 2";
            break;
        case 2:
            midiProg.name = "Phaser 3";
            break;
        case 3:
            midiProg.name = "Phaser 4";
            break;
        case 4:
            midiProg.name = "Phaser 5";
            break;
        case 5:
            midiProg.name = "Phaser 6";
            break;
        case 6:
            midiProg.name = "APhaser 1";
            break;
        case 7:
            midiProg.name = "APhaser 2";
            break;
        case 8:
            midiProg.name = "APhaser 3";
            break;
        case 9:
            midiProg.name = "APhaser 4";
            break;
        case 10:
            midiProg.name = "APhaser 5";
            break;
        case 11:
            midiProg.name = "APhaser 6";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    ZynPluginDescriptorClassEND(FxPhaserPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPhaserPlugin)
};

// -----------------------------------------------------------------------

class FxReverbPlugin : public FxAbstractPlugin
{
public:
    FxReverbPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 13, 13)
    {
        fEffect = new Reverb(false, efxoutl, efxoutr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index >= kParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[3];

        int hints = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Time";
            param.ranges.def = 63.0f;
            break;
        case 1:
            param.name = "Delay";
            param.ranges.def = 24.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 0.0f;
            break;
        case 3:
            hints = 0x0;
            param.name = "bw";
            break;
        case 4:
            hints = 0x0;
            param.name = "E/R";
            break;
        case 5:
            param.name = "Low-Pass Filter";
            param.ranges.def = 85.0f;
            break;
        case 6:
            param.name = "High-Pass Filter";
            param.ranges.def = 5.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Damp";
            param.ranges.def = 83.0f;
            break;
        case 8:
            param.name = "Type";
            param.ranges.def = 1.0f;
            param.ranges.max = 2.0f;
            param.scalePointCount = 3;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Random";
            scalePoints[1].label  = "Freeverb";
            scalePoints[2].label  = "Bandwidth";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            scalePoints[2].value  = 2.0f;
            break;
        case 9:
            param.name = "Room size";
            param.ranges.def = 64.0f;
            break;
        case 10:
            param.name = "Bandwidth";
            param.ranges.def = 20.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Cathedral1";
            break;
        case 1:
            midiProg.name = "Cathedral2";
            break;
        case 2:
            midiProg.name = "Cathedral3";
            break;
        case 3:
            midiProg.name = "Hall1";
            break;
        case 4:
            midiProg.name = "Hall2";
            break;
        case 5:
            midiProg.name = "Room1";
            break;
        case 6:
            midiProg.name = "Room2";
            break;
        case 7:
            midiProg.name = "Basement";
            break;
        case 8:
            midiProg.name = "Tunnel";
            break;
        case 9:
            midiProg.name = "Echoed1";
            break;
        case 10:
            midiProg.name = "Echoed2";
            break;
        case 11:
            midiProg.name = "VeryLong1";
            break;
        case 12:
            midiProg.name = "VeryLong2";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    ZynPluginDescriptorClassEND(FxReverbPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxReverbPlugin)
};

// -----------------------------------------------------------------------

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
          fThread(fMaster, host)
    {
        fThread.start();

        for (int i = 0; i < NUM_MIDI_PARTS; ++i)
            fMaster->partonoff(i, 1);

        sPrograms.init();
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
        return sPrograms.getInfo(index);
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
        fThread.stopLoadProgramLater();
        fMaster->putalldata((char*)data, 0);
        fMaster->applyparameters(true);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    intptr_t pluginDispatcher(const PluginDispatcherOpcode opcode, const int32_t index, const intptr_t value, void* const ptr, const float) override
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
        case PLUGIN_OPCODE_OFFLINE_CHANGED:
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
    Master*  fMaster;
    unsigned fSampleRate;
    bool     fIsActive;

    ZynAddSubFxThread fThread;

    ZynPluginDescriptorClassEND(ZynAddSubFxPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZynAddSubFxPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor fxAlienWahDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_USES_PANNING|PLUGIN_USES_STATIC_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 11-2,
    /* paramOuts */ 0,
    /* name      */ "ZynAlienWah",
    /* label     */ "zynAlienWah",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxAlienWahPlugin)
};

static const PluginDescriptor fxChorusDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_USES_PANNING|PLUGIN_USES_STATIC_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 12-2,
    /* paramOuts */ 0,
    /* name      */ "ZynChorus",
    /* label     */ "zynChorus",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxChorusPlugin)
};

static const PluginDescriptor fxDistortionDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_USES_STATIC_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 11-2,
    /* paramOuts */ 0,
    /* name      */ "ZynDistortion",
    /* label     */ "zynDistortion",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxDistortionPlugin)
};

static const PluginDescriptor fxDynamicFilterDesc = {
    /* category  */ PLUGIN_CATEGORY_FILTER,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_USES_STATIC_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 10-2,
    /* paramOuts */ 0,
    /* name      */ "ZynDynamicFilter",
    /* label     */ "zynDynamicFilter",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxDynamicFilterPlugin)
};

static const PluginDescriptor fxEchoDesc = {
    /* category  */ PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_USES_PANNING|PLUGIN_USES_STATIC_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 7-2,
    /* paramOuts */ 0,
    /* name      */ "ZynEcho",
    /* label     */ "zynEcho",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxEchoPlugin)
};

static const PluginDescriptor fxPhaserDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_USES_STATIC_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 15-2,
    /* paramOuts */ 0,
    /* name      */ "ZynPhaser",
    /* label     */ "zynPhaser",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxPhaserPlugin)
};

static const PluginDescriptor fxReverbDesc = {
    /* category  */ PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_USES_STATIC_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 13-2,
    /* paramOuts */ 0,
    /* name      */ "ZynReverb",
    /* label     */ "zynReverb",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxReverbPlugin)
};

static const PluginDescriptor zynaddsubfxDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
#ifdef WANT_ZYNADDSUBFX_UI
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_SYNTH|PLUGIN_HAS_GUI|PLUGIN_USES_STATE),
#else
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_SYNTH|PLUGIN_USES_STATE),
#endif
    /* supports  */ static_cast<PluginSupports>(PLUGIN_SUPPORTS_CONTROL_CHANGES|PLUGIN_SUPPORTS_NOTE_AFTERTOUCH|PLUGIN_SUPPORTS_PITCHBEND|PLUGIN_SUPPORTS_ALL_SOUND_OFF),
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
    carla_register_native_plugin(&fxAlienWahDesc);
    carla_register_native_plugin(&fxChorusDesc);
    carla_register_native_plugin(&fxDistortionDesc);
    carla_register_native_plugin(&fxDynamicFilterDesc);
    carla_register_native_plugin(&fxEchoDesc);
    carla_register_native_plugin(&fxPhaserDesc);
    carla_register_native_plugin(&fxReverbDesc);
    carla_register_native_plugin(&zynaddsubfxDesc);
}

// -----------------------------------------------------------------------
