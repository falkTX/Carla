/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifdef DISTRHO_UI_QT
# error We do not want Qt in the engine code!
#endif

#include "../CarlaNative.hpp"
#include "CarlaString.hpp"

#include "DistrhoPluginMain.cpp"

#define DISTRHO_PLUGIN_HAS_UI 1

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIMain.cpp"
# ifdef DISTRHO_UI_OPENGL
#  include "dgl/App.hpp"
#  include "dgl/Window.hpp"
# endif
#endif

#define WAIT_START_TIMEOUT  3000 /* ms */
#define WAIT_ZOMBIE_TIMEOUT 3000 /* ms */
#define WAIT_STEP 100            /* ms */

#include <fcntl.h>
//#include <sys/types.h>
#include <sys/wait.h>

using juce::ChildProcess;
using juce::NamedPipe;
using juce::Random;
using juce::ScopedPointer;
using juce::String;
using juce::StringArray;

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

#if DISTRHO_PLUGIN_HAS_UI
// -----------------------------------------------------------------------
// Carla UI

class UICarla
{
public:
    UICarla(const HostDescriptor* const host, PluginInternal* const plugin)
        : fHost(host),
          fPlugin(plugin),
          fUi(this, 0, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, uiResizeCallback),
#ifdef DISTRHO_UI_OPENGL
          glApp(fUi.getApp()),
          glWindow(fUi.getWindow())
#else
          pipeRecv(-1),
          pipeSend(-1),
          pid(-1)
#endif
    {
#ifdef DISTRHO_UI_OPENGL
        glWindow.setSize(fUi.getWidth(), fUi.getHeight());
        glWindow.setWindowTitle(host->uiName);
#else

        const char* argv[6];

        //------------------------------------------
        // argv[0] => filename

        CarlaString filename;
        filename += fHost->resourceDir;
#ifdef CARLA_OS_WIN
        filename += "\\resources\\";
#else
        filename += "/resources/";
#endif
        filename += fUi.getExternalFilename();

        argv[0] = (const char*)filename;

        //------------------------------------------
        // argv[1] => sample rate

        char sampleRateStr[12+1];
        std::snprintf(sampleRateStr, 12, "%g", host->get_sample_rate(host->handle));
        sampleRateStr[12] = '\0';

        argv[1] = sampleRateStr;

        //------------------------------------------
        // argv[2-3] => pipes

        int pipe1[2]; /* written by host process, read by plugin UI process */
        int pipe2[2]; /* written by plugin UI process, read by host process */

        if (pipe(pipe1) != 0)
        {
            fail("pipe1 creation failed");
            return;
        }

        if (pipe(pipe2) != 0)
        {
            fail("pipe2 creation failed");
            return;
        }

        char uiPipeRecv[100+1];
        char uiPipeSend[100+1];

        std::snprintf(uiPipeRecv, 100, "%d", pipe1[0]); /* [0] means reading end */
        std::snprintf(uiPipeSend, 100, "%d", pipe2[1]); /* [1] means writting end */

        uiPipeRecv[100] = '\0';
        uiPipeSend[100] = '\0';

        argv[2] = uiPipeRecv; /* reading end */
        argv[3] = uiPipeSend; /* writting end */

        //------------------------------------------
        // argv[4] => UI Name

        argv[4] = host->uiName;

        //------------------------------------------
        // argv[5] => NULL

        argv[5] = nullptr;

        //------------------------------------------
        // fork

        int ret = -1;

        if ((! fork_exec(argv, &ret)) || ret == -1)
        {
            close(pipe1[0]);
            close(pipe1[1]);
            close(pipe2[0]);
            close(pipe2[1]);
            fail("fork_exec() failed");
            return;
        }

        pid = ret;

        /* fork duplicated the handles, close pipe ends that are used by the child process */
        close(pipe1[0]);
        close(pipe2[1]);

        pipeSend = pipe1[1]; /* [1] means writting end */
        pipeRecv = pipe2[0]; /* [0] means reading end */

        fcntl(pipeRecv, F_SETFL, fcntl(pipeRecv, F_GETFL) | O_NONBLOCK);

        //------------------------------------------
        // wait a while for child process to confirm it is alive

        char ch;

        for (int i=0; ;)
        {
            ret = read(pipeRecv, &ch, 1);

            switch (ret)
            {
            case -1:
                if (errno == EAGAIN)
                {
                    if (i < WAIT_START_TIMEOUT / WAIT_STEP)
                    {
                        carla_msleep(WAIT_STEP);
                        i++;
                        continue;
                    }

                    carla_stderr("we have waited for child with pid %d to appear for %.1f seconds and we are giving up", (int)pid, (float)WAIT_START_TIMEOUT / 1000.0f);
                }
                else
                    carla_stderr("read() failed: %s", strerror(errno));
                break;

            case 1:
                if (ch == '\n')
                    // success
                    return;

                carla_stderr("read() wrong first char '%c'", ch);
                break;

            default:
                carla_stderr("read() returned %d", ret);
                break;
            }

            break;
        }

        carla_stderr("force killing misbehaved child %d (start)", (int)pid);

        if (kill(pid, SIGKILL) == -1)
        {
            carla_stderr("kill() failed: %s (start)\n", strerror(errno));
        }

        /* wait a while child to exit, we dont like zombie processes */
        wait_child(pid);
#endif
    }

    void fail(const char* const error)
    {
        carla_stderr2(error);
        fHost->dispatcher(fHost->handle, HOST_OPCODE_UI_UNAVAILABLE, 0, 0, nullptr, 0.0f);
    }

    static bool fork_exec(const char* const argv[6], int* const retp)
    {
        int ret = *retp = vfork();

        switch (ret)
        {
        case 0: /* child process */
            execvp(argv[0], (char* const*)argv);
            carla_stderr2("exec of UI failed: %s", strerror(errno));
            return false;
        case -1:
            carla_stderr2("fork() failed to create new process for plugin UI");
            return false;
        }

        return true;
    }

    static bool wait_child(pid_t pid)
    {
        pid_t ret;
        int i;

        if (pid == -1)
        {
            carla_stderr2("Can't wait for pid -1");
            return false;
        }

        for (i = 0; i < WAIT_ZOMBIE_TIMEOUT / WAIT_STEP; ++i)
        {
            ret = waitpid(pid, NULL, WNOHANG);

            if (ret != 0)
            {
                if (ret == pid)
                {
                  //printf("child zombie with pid %d was consumed.\n", (int)pid);
                  return true;
                }

                if (ret == -1)
                {
                  carla_stderr2("waitpid(%d) failed: %s", (int)pid, strerror(errno));
                  return false;
                }

                carla_stderr2("we have waited for child pid %d to exit but we got pid %d instead", (int)pid, (int)ret);

                return false;
            }

            carla_msleep(WAIT_STEP); /* wait 100 ms */
        }

        carla_stderr2("we have waited for child with pid %d to exit for %.1f seconds and we are giving up", (int)pid, (float)WAIT_START_TIMEOUT / 1000.0f);
        return false;
    }

    char* read_line() const
    {
        char ch;
        ssize_t ret;

        char buf[0xff];
        char* ptr = buf;

        for (int i=0; i < 0xff; ++i)
        {
            ret = read(pipeRecv, &ch, 1);

            if (ret == 1 && ch != '\n')
            {
                if (ch == '\r')
                    ch = '\n';

                *ptr++ = ch;
                continue;
            }

            if (ptr != buf)
            {
                *ptr = '\0';
                return strdup(buf);
            }

            break;
        }

        return nullptr;
    }

    ~UICarla()
    {
        printf("UI CARLA HERE 00END\n");
#ifdef DISTRHO_UI_EXTERNAL
        write(pipeSend, "quit\n", 5);

        /* for a while wait child to exit, we dont like zombie processes */
        if (! wait_child(pid))
        {
            carla_stderr2("force killing misbehaved child %d (exit)", (int)pid);

            if (kill(pid, SIGKILL) == -1)
                carla_stderr2("kill() failed: %s (exit)", strerror(errno));
            else
                wait_child(pid);
        }
#endif
    }

    // ---------------------------------------------

    void carla_show(const bool yesNo)
    {
#ifdef DISTRHO_UI_OPENGL
        glWindow.setVisible(yesNo);
#else
        if (yesNo)
            write(pipeSend, "show\n", 5);
        else
            write(pipeSend, "hide\n", 5);
#endif
    }

    void carla_idle()
    {
        fUi.idle();

#if 1//def DISTRHO_UI_EXTERNAL
      char* locale = strdup(setlocale(LC_NUMERIC, nullptr));
      setlocale(LC_NUMERIC, "POSIX");

      for (;;)
      {
          char* const msg = read_line();

          if (msg == nullptr)
              break;

          if (std::strcmp(msg, "control") == 0)
          {
              int index;
              float value;
              char* indexStr = read_line();
              char* valueStr = read_line();

              index = atoi(indexStr);

              if (sscanf(valueStr, "%f", &value) == 1)
                  fHost->ui_parameter_changed(fHost->handle, index, value);
              else
                  fprintf(stderr, "failed to convert \"%s\" to float\n", valueStr);

              carla_stdout("PARAM CHANGE, %i %f", index, value);

              std::free(indexStr);
              std::free(valueStr);
          }
          else if (std::strcmp(msg, "configure") == 0)
          {
              char* const key   = read_line();
              char* const value = read_line();

              fHost->ui_custom_data_changed(fHost->handle, key, value);

              carla_stdout("STATE CHANGE, \"%s\" \"%s\"", key, value);

              std::free(key);
              std::free(value);
          }
          else if (std::strcmp(msg, "exiting") == 0)
          {
              /* for a while wait child to exit, we dont like zombie processes */
              if (! wait_child(pid))
              {
                  fprintf(stderr, "force killing misbehaved child %d (exit)\n", (int)pid);

                  if (kill(pid, SIGKILL) == -1)
                      fprintf(stderr, "kill() failed: %s (exit)\n", strerror(errno));
                  else
                      wait_child(pid);
              }

              fHost->ui_closed(fHost->handle);
          }
          else
          {
              carla_stderr("unknown message HOST: \"%s\"", msg);
          }

          std::free(msg);
      }

      setlocale(LC_NUMERIC, locale);
      std::free(locale);
#endif
    }

    void carla_setParameterValue(const uint32_t index, const float value)
    {
#ifdef DISTRHO_UI_OPENGL
        fUi.parameterChanged(index, value);
#else
        char msgParamIndex[0xff+1];
        char msgParamValue[0xff+1];

        std::snprintf(msgParamIndex, 0xff, "%d", index);
        std::snprintf(msgParamValue, 0xff, "%f", value);

        msgParamIndex[0xff] = '\0';
        msgParamValue[0xff] = '\0';

        write(pipeSend, "control\n", 8);
        write(pipeSend, msgParamIndex, std::strlen(msgParamIndex));
        write(pipeSend, msgParamValue, std::strlen(msgParamValue));
#endif
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void carla_setMidiProgram(const uint32_t realProgram)
    {
 #ifdef DISTRHO_UI_OPENGL
        fUi.programChanged(realProgram);
 #else
        char msgProgram[0xff+1];

        std::snprintf(msgProgram, 0xff, "%d", realProgram);

        msgProgram[0xff] = '\0';

        write(pipeSend, "program\n", 8);
        write(pipeSend, msgProgram, std::strlen(msgProgram));
 #endif
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void carla_setCustomData(const char* const key, const char* const value)
    {
 #ifdef DISTRHO_UI_OPENGL
        fUi.stateChanged(key, value);
 #else
        CarlaString skey(key), svalue(value);
        skey.replace('\n', '\r');
        svalue.replace('\n', '\r');

        write(pipeSend, "configure\n", 10);
        write(pipeSend, (const char*)skey,   skey.length());
        write(pipeSend, (const char*)svalue, svalue.length());
 #endif
    }
#endif

    void carla_setUiTitle(const char* const uiTitle)
    {
#ifdef DISTRHO_UI_OPENGL
        glWindow.setWindowTitle(uiName);
#else
        CarlaString stitle(uiTitle);
        stitle.replace('\n', '\r');

        write(pipeSend, "uiTitle\n", 8);
        write(pipeSend, (const char*)stitle, stitle.length());
#endif
    }

    // ---------------------------------------------

protected:
    void handleEditParameter(uint32_t, bool)
    {
        // TODO
    }

    void handleSetParameterValue(uint32_t rindex, float value)
    {
        fHost->ui_parameter_changed(fHost->handle, rindex, value);
    }

    void handleSetState(const char* key, const char* value)
    {
        fHost->ui_custom_data_changed(fHost->handle, key, value);
    }

    void handleSendNote(bool, uint8_t, uint8_t, uint8_t)
    {
        // TODO
    }

    void handleUiResize(unsigned int /*width*/, unsigned int /*height*/)
    {
        // TODO
    }

    // ---------------------------------------------

private:
    // Plugin stuff
    const HostDescriptor* const fHost;
    PluginInternal* const fPlugin;

    // UI
    UIInternal fUi;

#ifdef DISTRHO_UI_OPENGL
    // OpenGL stuff
    App& glApp;
    Window& glWindow;
#else
    int pipeRecv; /* the pipe end that is used for receiving messages from UI */
    int pipeSend; /* the pipe end that is used for sending messages to UI */
    pid_t pid;
#endif

    // ---------------------------------------------
    // Callbacks

#ifdef DISTRHO_UI_OPENGL
    #define handlePtr ((UICarla*)ptr)

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        handlePtr->handleEditParameter(index, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        handlePtr->handleSetParameterValue(rindex, value);
    }

 #if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        handlePtr->handleSetState(key, value);
    }
 #else
    static constexpr setStateFunc setStateCallback = nullptr;
 #endif

 #if DISTRHO_PLUGIN_IS_SYNTH
    static void sendNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->handleSendNote(onOff, channel, note, velocity);
    }
 #else
    static constexpr sendNoteFunc sendNoteCallback = nullptr;
 #endif

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        handlePtr->handleUiResize(width, height);
    }

    #undef handlePtr
#else
    static constexpr editParamFunc editParameterCallback = nullptr;
    static constexpr setParamFunc  setParameterCallback  = nullptr;
    static constexpr setStateFunc  setStateCallback      = nullptr;
    static constexpr sendNoteFunc  sendNoteCallback      = nullptr;
    static constexpr uiResizeFunc  uiResizeCallback      = nullptr;
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UICarla)
};
#endif // DISTRHO_PLUGIN_HAS_UI

// -----------------------------------------------------------------------
// Carla Plugin

class PluginCarla : public PluginClass
{
public:
    PluginCarla(const HostDescriptor* const host)
        : PluginClass(host)
    {
#if DISTRHO_PLUGIN_HAS_UI
        fUiPtr = nullptr;
#endif
    }

    ~PluginCarla() override
    {
#if DISTRHO_PLUGIN_HAS_UI
        fUiPtr = nullptr;
#endif
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return fPlugin.getParameterCount();
    }

    const ::Parameter* getParameterInfo(const uint32_t index) const override
    {
        CARLA_ASSERT(index < getParameterCount());

        static ::Parameter param;

        // reset
        param.hints = ::PARAMETER_IS_ENABLED;
        param.scalePointCount = 0;
        param.scalePoints = nullptr;

        {
            int      nativeParamHints = ::PARAMETER_IS_ENABLED;
            const uint32_t paramHints = fPlugin.getParameterHints(index);

            if (paramHints & PARAMETER_IS_AUTOMABLE)
                nativeParamHints |= ::PARAMETER_IS_AUTOMABLE;
            if (paramHints & PARAMETER_IS_BOOLEAN)
                nativeParamHints |= ::PARAMETER_IS_BOOLEAN;
            if (paramHints & PARAMETER_IS_INTEGER)
                nativeParamHints |= ::PARAMETER_IS_INTEGER;
            if (paramHints & PARAMETER_IS_LOGARITHMIC)
                nativeParamHints |= ::PARAMETER_IS_LOGARITHMIC;
            if (paramHints & PARAMETER_IS_OUTPUT)
                nativeParamHints |= ::PARAMETER_IS_OUTPUT;

            param.hints = static_cast<ParameterHints>(nativeParamHints);
        }

        param.name = fPlugin.getParameterName(index);
        param.unit = fPlugin.getParameterUnit(index);

        {
            const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

            param.ranges.def = ranges.def;
            param.ranges.min = ranges.min;
            param.ranges.max = ranges.max;
            param.ranges.step = ranges.step;
            param.ranges.stepSmall = ranges.stepSmall;
            param.ranges.stepLarge = ranges.stepLarge;
        }

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        CARLA_ASSERT(index < getParameterCount());

        return fPlugin.getParameterValue(index);
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t getMidiProgramCount() const override
    {
        return fPlugin.getProgramCount();
    }

    const ::MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        CARLA_ASSERT(index < getMidiProgramCount());

        if (index >= fPlugin.getProgramCount())
            return nullptr;

        static ::MidiProgram midiProgram;

        midiProgram.bank    = index / 128;
        midiProgram.program = index % 128;
        midiProgram.name    = fPlugin.getProgramName(index);

        return &midiProgram;
    }
#endif

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(index < getParameterCount());

        fPlugin.setParameterValue(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void setMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        const uint32_t realProgram(bank * 128 + program);

        if (realProgram >= fPlugin.getProgramCount())
            return;

        fPlugin.setProgram(realProgram);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        fPlugin.setState(key, value);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
        fPlugin.activate();
    }

    void deactivate() override
    {
        fPlugin.deactivate();
    }

#if DISTRHO_PLUGIN_IS_SYNTH
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const ::MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        uint32_t i;

        for (i=0; i < midiEventCount && i < MAX_MIDI_EVENTS; ++i)
        {
            const ::MidiEvent* const midiEvent = &midiEvents[i];
            MidiEvent* const realMidiEvent = &fRealMidiEvents[i];

            realMidiEvent->frame = midiEvent->time;
            realMidiEvent->size  = midiEvent->size;

            for (uint8_t j=0; j < midiEvent->size; ++j)
                realMidiEvent->buf[j] = midiEvent->data[j];
        }

        fPlugin.run(inBuffer, outBuffer, frames, fRealMidiEvents, i);
    }
#else
    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const ::MidiEvent* const, const uint32_t) override
    {
        fPlugin.run(inBuffer, outBuffer, frames, nullptr, 0);
    }
#endif

    // -------------------------------------------------------------------
    // Plugin UI calls

#if DISTRHO_PLUGIN_HAS_UI
    void uiShow(const bool show) override
    {
        if (show)
            createUiIfNeeded();

        if (fUiPtr != nullptr)
            fUiPtr->carla_show(show);
    }

    void uiIdle() override
    {
        CARLA_ASSERT(fUiPtr != nullptr);

        if (fUiPtr != nullptr)
            fUiPtr->carla_idle();
    }

    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        CARLA_ASSERT(fUiPtr != nullptr);
        CARLA_ASSERT(index < getParameterCount());

        if (fUiPtr != nullptr)
            fUiPtr->carla_setParameterValue(index, value);
    }

# if DISTRHO_PLUGIN_WANT_PROGRAMS
    void uiSetMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        CARLA_ASSERT(fUiPtr != nullptr);

        const uint32_t realProgram(bank * 128 + program);

        if (realProgram >= fPlugin.getProgramCount())
            return;

        if (fUiPtr != nullptr)
            fUiPtr->carla_setMidiProgram(realProgram);
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    void uiSetCustomData(const char* const key, const char* const value) override
    {
        CARLA_ASSERT(fUiPtr != nullptr);
        CARLA_ASSERT(key != nullptr);
        CARLA_ASSERT(value != nullptr);

        if (fUiPtr != nullptr)
            fUiPtr->carla_setCustomData(key, value);
    }
# endif
#endif

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        fPlugin.setBufferSize(bufferSize, true);
    }

    void sampleRateChanged(const double sampleRate) override
    {
        fPlugin.setSampleRate(sampleRate, true);
    }

#if DISTRHO_PLUGIN_HAS_UI
    void uiNameChanged(const char* const uiName) override
    {
        if (fUiPtr != nullptr)
            fUiPtr->carla_setUiTitle(uiName);
    }
#endif

    // -------------------------------------------------------------------

private:
    PluginInternal fPlugin;

#if DISTRHO_PLUGIN_IS_SYNTH
    MidiEvent fRealMidiEvents[MAX_MIDI_EVENTS];
#endif

#if DISTRHO_PLUGIN_HAS_UI
    // UI
    ScopedPointer<UICarla> fUiPtr;

    void createUiIfNeeded()
    {
        if (fUiPtr == nullptr)
        {
            d_lastUiSampleRate = getSampleRate();
            fUiPtr = new UICarla(getHostHandle(), &fPlugin);
        }
    }
#endif

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCarla)

    // -------------------------------------------------------------------

public:
    static PluginHandle _instantiate(const HostDescriptor* host)
    {
        d_lastBufferSize = host->get_buffer_size(host->handle);
        d_lastSampleRate = host->get_sample_rate(host->handle);
        return new PluginCarla(host);
    }

    static void _cleanup(PluginHandle handle)
    {
        delete (PluginCarla*)handle;
    }
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
