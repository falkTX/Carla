/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef DOXYGEN

#include "src/DistrhoDefines.h"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Intro */

/**
   @mainpage DISTRHO %Plugin Framework

   DISTRHO %Plugin Framework (or @b DPF for short)
   is a plugin framework designed to make development of new plugins an easy and enjoyable task.@n
   It allows developers to create plugins with custom UIs using a simple C++ API.@n
   The framework facilitates exporting various different plugin formats from the same code-base.

   DPF can build for LADSPA, DSSI, LV2 and VST2 formats.@n
   A JACK/Standalone mode is also available, allowing you to quickly test plugins.

   @section Macros
   You start by creating a "DistrhoPluginInfo.h" file describing the plugin via macros, see @ref PluginMacros.@n
   This file is included in the main DPF code to select which features to activate for each plugin format.

   For example, a plugin (with %UI) that use states will require LV2 hosts to support Atom and Worker extensions for
   message passing from the %UI to the plugin.@n
   If your plugin does not make use of states, the Worker extension is not set as a required feature.

   @section Plugin
   The next step is to create your plugin code by subclassing DPF's Plugin class.@n
   You need to pass the number of parameters in the constructor and also the number of programs and states, if any.

   Here's an example of an audio plugin that simply mutes the host output:
   @code
   class MutePlugin : public Plugin
   {
   public:
     /**
        Plugin class constructor.
      */
      MutePlugin()
          : Plugin(0, 0, 0) // 0 parameters, 0 programs and 0 states
      {
      }

   protected:
     /* ----------------------------------------------------------------------------------------
      * Information */

     /**
        Get the plugin label.
        This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
      */
      const char* getLabel() const override
      {
          return "Mute";
      }

     /**
        Get the plugin author/maker.
      */
      const char* getMaker() const override
      {
          return "DPF";
      }

     /**
        Get the plugin license name (a single line of text).
        For commercial plugins this should return some short copyright information.
      */
      const char* getLicense() const override
      {
          return "MIT";
      }

     /**
        Get the plugin version, in hexadecimal.
      */
      uint32_t getVersion() const override
      {
          return d_version(1, 0, 0);
      }

     /**
        Get the plugin unique Id.
        This value is used by LADSPA, DSSI and VST plugin formats.
      */
      int64_t getUniqueId() const override
      {
          return d_cconst('M', 'u', 't', 'e');
      }

     /* ----------------------------------------------------------------------------------------
      * This example has no parameters, so skip parameter stuff */

      void  initParameter(uint32_t, Parameter&) override {}
      float getParameterValue(uint32_t) const   override { return 0.0f; }
      void  setParameterValue(uint32_t, float)  override {}

     /* ----------------------------------------------------------------------------------------
      * Audio/MIDI Processing */

     /**
        Run/process function for plugins without MIDI input.
        NOTE: Some parameters might be null if there are no audio inputs or outputs.
      */
      void run(const float**, float** outputs, uint32_t frames) override
      {
          // get the left and right audio outputs
          float* const outL = outputs[0];
          float* const outR = outputs[1];

          // mute audio
          std::memset(outL, 0, sizeof(float)*frames);
          std::memset(outR, 0, sizeof(float)*frames);
      }

   };
   @endcode

   See the Plugin class for more information and to understand what each function does.

   @section Parameters
   A plugin is nothing without parameters.@n
   In DPF parameters can be inputs or outputs.@n
   They have hints to describe how they behave plus a name and a symbol identifying them.@n
   Parameters also have 'ranges' – a minimum, maximum and default value.

   Input parameters are "read-only": the plugin can read them but not change them.
   (the exception being when changing programs, more on that below)@n
   It's the host responsibility to save, restore and set input parameters.

   Output parameters can be changed at anytime by the plugin.@n
   The host will simply read their values and not change them.

   Here's an example of an audio plugin that has 1 input parameter:
   @code
   class GainPlugin : public Plugin
   {
   public:
     /**
        Plugin class constructor.
        You must set all parameter values to their defaults, matching ParameterRanges::def.
      */
      GainPlugin()
          : Plugin(1, 0, 0), // 1 parameter, 0 programs and 0 states
            fGain(1.0f)
      {
      }

   protected:
     /* ----------------------------------------------------------------------------------------
      * Information */

      const char* getLabel() const override
      {
          return "Gain";
      }

      const char* getMaker() const override
      {
          return "DPF";
      }

      const char* getLicense() const override
      {
          return "MIT";
      }

      uint32_t getVersion() const override
      {
          return d_version(1, 0, 0);
      }

      int64_t getUniqueId() const override
      {
          return d_cconst('G', 'a', 'i', 'n');
      }

     /* ----------------------------------------------------------------------------------------
      * Init */

     /**
        Initialize a parameter.
        This function will be called once, shortly after the plugin is created.
      */
      void initParameter(uint32_t index, Parameter& parameter) override
      {
          // we only have one parameter so we can skip checking the index

          parameter.hints      = kParameterIsAutomable;
          parameter.name       = "Gain";
          parameter.symbol     = "gain";
          parameter.ranges.min = 0.0f;
          parameter.ranges.max = 2.0f;
          parameter.ranges.def = 1.0f;
      }

     /* ----------------------------------------------------------------------------------------
      * Internal data */

     /**
        Get the current value of a parameter.
      */
      float getParameterValue(uint32_t index) const override
      {
          // same as before, ignore index check

          return fGain;
      }

     /**
        Change a parameter value.
      */
      void setParameterValue(uint32_t index, float value) override
      {
          // same as before, ignore index check

          fGain = value;
      }

     /* ----------------------------------------------------------------------------------------
      * Audio/MIDI Processing */

      void run(const float**, float** outputs, uint32_t frames) override
      {
          // get the mono input and output
          const float* const in  = inputs[0];
          /* */ float* const out = outputs[0];

          // apply gain against all samples
          for (uint32_t i=0; i < frames; ++i)
              out[i] = in[i] * fGain;
      }

   private:
      float fGain;
   };
   @endcode

   See the Parameter struct for more information about parameters.

   @section Programs
   Programs in DPF refer to plugin-side presets (usually called "factory presets"),
   an initial set of presets provided by plugin authors included in the actual plugin.

   To use programs you must first enable them by setting @ref DISTRHO_PLUGIN_WANT_PROGRAMS to 1 in your DistrhoPluginInfo.h file.@n
   When enabled you'll need to override 2 new function in your plugin code,
   Plugin::initProgramName(uint32_t, String&) and Plugin::loadProgram(uint32_t).

   Here's an example of a plugin with a "default" program:
   @code
   class PluginWithPresets : public Plugin
   {
   public:
      PluginWithPresets()
          : Plugin(2, 1, 0), // 2 parameters, 1 program and 0 states
            fGainL(1.0f),
            fGainR(1.0f),
      {
      }

   protected:
     /* ----------------------------------------------------------------------------------------
      * Information */

      const char* getLabel() const override
      {
          return "Prog";
      }

      const char* getMaker() const override
      {
          return "DPF";
      }

      const char* getLicense() const override
      {
          return "MIT";
      }

      uint32_t getVersion() const override
      {
          return d_version(1, 0, 0);
      }

      int64_t getUniqueId() const override
      {
          return d_cconst('P', 'r', 'o', 'g');
      }

     /* ----------------------------------------------------------------------------------------
      * Init */

     /**
        Initialize a parameter.
        This function will be called once, shortly after the plugin is created.
      */
      void initParameter(uint32_t index, Parameter& parameter) override
      {
          parameter.hints      = kParameterIsAutomable;
          parameter.ranges.min = 0.0f;
          parameter.ranges.max = 2.0f;
          parameter.ranges.def = 1.0f;

          switch (index)
          {
          case 0;
              parameter.name   = "Gain Right";
              parameter.symbol = "gainR";
              break;
          case 1;
              parameter.name   = "Gain Left";
              parameter.symbol = "gainL";
              break;
          }
      }

      /**
          Set the name of the program @a index.
          This function will be called once, shortly after the plugin is created.
        */
      void initProgramName(uint32_t index, String& programName)
      {
          switch(index)
          {
          case 0:
              programName = "Default";
              break;
          }
      }

     /* ----------------------------------------------------------------------------------------
      * Internal data */

     /**
        Get the current value of a parameter.
      */
      float getParameterValue(uint32_t index) const override
      {
          switch (index)
          {
          case 0;
              return fGainL;
          case 1;
              return fGainR;
          }
      }

     /**
        Change a parameter value.
      */
      void setParameterValue(uint32_t index, float value) override
      {
          switch (index)
          {
          case 0;
              fGainL = value;
              break;
          case 1;
              fGainR = value;
              break;
          }
      }

     /**
        Load a program.
      */
      void loadProgram(uint32_t index)
      {
          switch(index)
          {
          case 0:
              fGainL = 1.0f;
              fGainR = 1.0f;
              break;
          }
      }

     /* ----------------------------------------------------------------------------------------
      * Audio/MIDI Processing */

      void run(const float**, float** outputs, uint32_t frames) override
      {
          // get the left and right audio buffers
          const float* const inL  = inputs[0];
          const float* const inR  = inputs[0];
          /* */ float* const outL = outputs[0];
          /* */ float* const outR = outputs[0];

          // apply gain against all samples
          for (uint32_t i=0; i < frames; ++i)
          {
              outL[i] = inL[i] * fGainL;
              outR[i] = inR[i] * fGainR;
          }
      }

   private:
      float fGainL, fGainR;
   };
   @endcode

   @section States
   describe them

   @section MIDI
   describe them

   @section Latency
   describe it

   @section Time-Position
   describe it

   @section UI
   describe them
*/

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Macros */

/**
   @defgroup PluginMacros Plugin Macros

   C Macros that describe your plugin. (defined in the "DistrhoPluginInfo.h" file)

   With these macros you can tell the host what features your plugin requires.@n
   Depending on which macros you enable, new functions will be available to call and/or override.

   All values are either integer or strings.@n
   For boolean-like values 1 means 'on' and 0 means 'off'.

   The values defined in this group are for documentation purposes only.@n
   All macros are disabled by default.

   Only 4 macros are required, they are:
    - @ref DISTRHO_PLUGIN_NAME
    - @ref DISTRHO_PLUGIN_NUM_INPUTS
    - @ref DISTRHO_PLUGIN_NUM_OUTPUTS
    - @ref DISTRHO_PLUGIN_URI
   @{
 */

/**
   The plugin name.@n
   This is used to identify your plugin before a Plugin instance can be created.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_NAME "Plugin Name"

/**
   Number of audio inputs the plugin has.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_NUM_INPUTS 2

/**
   Number of audio outputs the plugin has.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_NUM_OUTPUTS 2

/**
   The plugin URI when exporting in LV2 format.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_URI "urn:distrho:name"

/**
   Wherever the plugin has a custom %UI.
   @see DISTRHO_UI_USE_NANOVG
   @see UI
 */
#define DISTRHO_PLUGIN_HAS_UI 1

/**
   Wherever the plugin processing is realtime-safe.@n
   TODO - list rtsafe requirements
 */
#define DISTRHO_PLUGIN_IS_RT_SAFE 1

/**
   Wherever the plugin is a synth.@n
   @ref DISTRHO_PLUGIN_WANT_MIDI_INPUT is automatically enabled when this is too.
   @see DISTRHO_PLUGIN_WANT_MIDI_INPUT
 */
#define DISTRHO_PLUGIN_IS_SYNTH 1

/**
   Enable direct access between the %UI and plugin code.
   @see UI::getPluginInstancePointer()
   @note DO NOT USE THIS UNLESS STRICTLY NECESSARY!!
         Try to avoid it at all costs!
 */
#define DISTRHO_PLUGIN_WANT_DIRECT_ACCESS 0

/**
   Wherever the plugin introduces latency during audio or midi processing.
   @see Plugin::setLatency(uint32_t)
 */
#define DISTRHO_PLUGIN_WANT_LATENCY 1

/**
   Wherever the plugin wants MIDI input.@n
   This is automatically enabled if @ref DISTRHO_PLUGIN_IS_SYNTH is true.
 */
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 1

/**
   Wherever the plugin wants MIDI output.
   @see Plugin::writeMidiEvent(const MidiEvent&)
 */
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 1

/**
   Wherever the plugin provides its own internal programs.
   @see Plugin::initProgramName(uint32_t, String&)
   @see Plugin::loadProgram(uint32_t)
 */
#define DISTRHO_PLUGIN_WANT_PROGRAMS 1

/**
   Wherever the plugin uses internal non-parameter data.
   @see Plugin::initState(uint32_t, String&, String&)
   @see Plugin::setState(const char*, const char*)
 */
#define DISTRHO_PLUGIN_WANT_STATE 1

/**
   Wherever the plugin wants time position information from the host.
   @see Plugin::getTimePosition()
 */
#define DISTRHO_PLUGIN_WANT_TIMEPOS 1

/**
   Wherever the %UI uses NanoVG for drawing instead of the default raw OpenGL calls.@n
   When enabled your %UI instance will subclass @ref NanoWidget instead of @ref Widget.
 */
#define DISTRHO_UI_USE_NANOVG 1

/**
   The %UI URI when exporting in LV2 format.@n
   By default this is set to @ref DISTRHO_PLUGIN_URI with "#UI" as suffix.
 */
#define DISTRHO_UI_URI DISTRHO_PLUGIN_URI "#UI"

/** @} */

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DOXYGEN
