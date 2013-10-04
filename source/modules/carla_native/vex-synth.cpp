/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "juce_gui_basics.h"

using namespace juce;

#include "vex/VexArp.h"
#include "vex/VexChorus.h"
#include "vex/VexDelay.h"
#include "vex/VexReverb.h"
#include "vex/VexSyntModule.h"

#include "vex/PeggyViewComponent.h"
#include "vex/lookandfeel/MyLookAndFeel.h"
#include "vex/resources/Resources.h"

#include "vex/resources/Resources.cpp"

// -----------------------------------------------------------------------

class HelperWindow2 : public DocumentWindow
{
public:
    HelperWindow2()
        : DocumentWindow("PlugWindow", Colour(50, 50, 200), DocumentWindow::closeButton, false),
          fClosed(false)
    {
        setVisible(false);
        setAlwaysOnTop(true);
        setDropShadowEnabled(false);
        setOpaque(true);
        //setResizable(false, false);
        //setUsingNativeTitleBar(false);
    }

    void show(Component* const comp)
    {
        fClosed = false;

        const int width  = comp->getWidth();
        const int height = comp->getHeight()+getTitleBarHeight();

        centreWithSize(width, height);
        setContentNonOwned(comp, false);
        setSize(width, height);

        if (! isOnDesktop())
            addToDesktop();

        setVisible(true);
    }

    void hide()
    {
        setVisible(false);

        if (isOnDesktop())
            removeFromDesktop();

        clearContentComponent();
    }

    bool wasClosedByUser() const
    {
        return fClosed;
    }

protected:
    void closeButtonPressed() override
    {
        fClosed = true;
    }

private:
    bool fClosed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelperWindow2)
};

// -----------------------------------------------------------------------

class VexEditorComponent : public ComboBox::Listener,
                           public Slider::Listener,
                           public Button::Listener,
                           public ChangeListener,
                           public Component,
                           public PeggyViewComponent::Callback
{
public:
    VexEditorComponent()
    {
        internalCachedImage1 = ImageCache::getFromMemory(Resources::vex3_png, Resources::vex3_pngSize);

        // Comboboxes, wave selection
        addAndMakeVisible(comboBox1 = new ComboBox (String::empty));
        comboBox1->setEditableText(false);
        comboBox1->setJustificationType(Justification::centredLeft);
        comboBox1->setTextWhenNothingSelected(String("silent"));
        comboBox1->setTextWhenNoChoicesAvailable(String("silent"));
        comboBox1->addListener(this);
        comboBox1->setColour(ComboBox::backgroundColourId, Colours::black);
        comboBox1->setColour(ComboBox::textColourId, Colours::lightgrey);
        comboBox1->setColour(ComboBox::outlineColourId, Colours::grey);
        comboBox1->setColour(ComboBox::buttonColourId, Colours::grey);
        comboBox1->setWantsKeyboardFocus(false);
        comboBox1->setLookAndFeel(&mlaf);

        addAndMakeVisible (comboBox2 = new ComboBox (String::empty));
        comboBox2->setEditableText (false);
        comboBox2->setJustificationType (Justification::centredLeft);
        comboBox2->setTextWhenNothingSelected (String("silent"));
        comboBox2->setTextWhenNoChoicesAvailable (String("silent"));
        comboBox2->addListener (this);
        comboBox2->setColour(ComboBox::backgroundColourId, Colours::black);
        comboBox2->setColour(ComboBox::textColourId, Colours::lightgrey);
        comboBox2->setColour(ComboBox::outlineColourId, Colours::grey);
        comboBox2->setColour(ComboBox::buttonColourId, Colours::grey);
        comboBox2->setWantsKeyboardFocus(false);
        comboBox2->setLookAndFeel(&mlaf);

        addAndMakeVisible (comboBox3 = new ComboBox (String::empty));
        comboBox3->setEditableText (false);
        comboBox3->setJustificationType (Justification::centredLeft);
        comboBox3->setTextWhenNothingSelected (String("silent"));
        comboBox3->setTextWhenNoChoicesAvailable (String("silent"));
        comboBox3->addListener (this);
        comboBox3->setColour(ComboBox::backgroundColourId, Colours::black);
        comboBox3->setColour(ComboBox::textColourId, Colours::lightgrey);
        comboBox3->setColour(ComboBox::outlineColourId, Colours::grey);
        comboBox3->setColour(ComboBox::buttonColourId, Colours::grey);
        comboBox3->setWantsKeyboardFocus(false);
        comboBox3->setLookAndFeel(&mlaf);

        for (int i = 0, tableSize = WaveRenderer::getWaveTableSize(); i < tableSize; ++i)
        {
            String tableName(WaveRenderer::getWaveTableName(i));

            comboBox1->addItem(tableName, i + 1);
            comboBox2->addItem(tableName, i + 1);
            comboBox3->addItem(tableName, i + 1);
        }

        addChildComponent(p1 = new PeggyViewComponent(1, _d, this));
        p1->setLookAndFeel(&mlaf);
        addChildComponent(p2 = new PeggyViewComponent(2, _d, this));
        p2->setLookAndFeel(&mlaf);
        addChildComponent(p3 = new PeggyViewComponent(3, _d, this));
        p3->setLookAndFeel(&mlaf);

        //ownerFilter->addChangeListener (this);
        setSize(800,500);
    }

    ~VexEditorComponent()
    {
        removeAllChildren();
    }

protected:
    void paint(Graphics& g) override
    {
        g.drawImage(internalCachedImage1,
                    0, 0, 800, 500,
                    0, 0, internalCachedImage1.getWidth(), internalCachedImage1.getHeight());
    }

    void resized() override
    {
        comboBox1->setBounds(13, 38, 173, 23);
        comboBox2->setBounds(213, 38, 173, 23);
        comboBox3->setBounds(413, 38, 173, 23);

        p1->setBounds(10, 20, 207, 280);
        p2->setBounds(210, 20, 207, 280);
        p3->setBounds(410, 20, 207, 280);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
    }

    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
    {
    }

    void sliderValueChanged(Slider* sliderThatWasMoved) override
    {
    }

    void buttonClicked(Button* buttonThatWasClicked) override
    {
    }

    void somethingChanged(const uint32_t id) override
    {
    }

private:
    Image internalCachedImage1;
    MyLookAndFeel mlaf;

    ScopedPointer<ComboBox> comboBox1;
    ScopedPointer<ComboBox> comboBox2;
    ScopedPointer<ComboBox> comboBox3;

    ScopedPointer<PeggyViewComponent> p1;
    ScopedPointer<PeggyViewComponent> p2;
    ScopedPointer<PeggyViewComponent> p3;

    VexArpSettings _d;
};

// -----------------------------------------------------------------------

class VexSynthPlugin : public PluginClass
{
public:
    static const unsigned int kParamCount = 92;

    VexSynthPlugin(const HostDescriptor* const host)
        : PluginClass(host),
          obf(nullptr),
          abf(nullptr),
          dbf1(nullptr),
          dbf2(nullptr),
          dbf3(nullptr),
          fChorus(fParameters),
          fDelay(fParameters),
          fReverb(fParameters),
          fSynth(fParameters)
    {
        std::memset(fParameters, 0, sizeof(float)*92);

        fParameters[0] = 1.0f; // main volume

        for (int i = 0; i < 3; ++i)
        {
            const int offset = i * 24;

            fParameters[offset +  1] = 0.5f;
            fParameters[offset +  2] = 0.5f;
            fParameters[offset +  3] = 0.5f;
            fParameters[offset +  4] = 0.5f;
            fParameters[offset +  5] = 0.9f;
            fParameters[offset +  6] = 0.0f;
            fParameters[offset +  7] = 1.0f;
            fParameters[offset +  8] = 0.5f;
            fParameters[offset +  9] = 0.0f;
            fParameters[offset + 10] = 0.2f;
            fParameters[offset + 11] = 0.0f;
            fParameters[offset + 12] = 0.5f;
            fParameters[offset + 13] = 0.5f;
            fParameters[offset + 14] = 0.0f;
            fParameters[offset + 15] = 0.3f;
            fParameters[offset + 16] = 0.7f;
            fParameters[offset + 17] = 0.1f;
            fParameters[offset + 18] = 0.5f;
            fParameters[offset + 19] = 0.5f;
            fParameters[offset + 20] = 0.0f;
            fParameters[offset + 21] = 0.0f;
            fParameters[offset + 22] = 0.5f;
            fParameters[offset + 23] = 0.5f;
            fParameters[offset + 24] = 0.5f;
        }

        // ^1 - 72

        fParameters[73] = 0.5f; // Delay Time
        fParameters[74] = 0.4f; // Delay Feedback
        fParameters[75] = 0.0f; // Delay Volume

        fParameters[76] = 0.3f; // Chorus Rate
        fParameters[77] = 0.6f; // Chorus Depth
        fParameters[78] = 0.5f; // Chorus Volume

        fParameters[79] = 0.6f; // Reverb Size
        fParameters[80] = 0.7f; // Reverb Width
        fParameters[81] = 0.6f; // Reverb Damp
        fParameters[82] = 0.0f; // Reverb Volume

        fParameters[83] = 0.5f; // wave1 panning
        fParameters[84] = 0.5f; // wave2 panning
        fParameters[85] = 0.5f; // wave3 panning

        fParameters[86] = 0.5f; // wave1 volume
        fParameters[87] = 0.5f; // wave2 volume
        fParameters[88] = 0.5f; // wave3 volume

        fParameters[89] = 1.0f; // wave1 on/off
        fParameters[90] = 1.0f; // wave2 on/off
        fParameters[91] = 1.0f; // wave3 on/off

        bufferSizeChanged(getBufferSize());
        sampleRateChanged(getSampleRate());
    }

    ~VexSynthPlugin()
    {
        delete obf;
        delete abf;
        delete dbf1;
        delete dbf2;
        delete dbf3;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParamCount;
    }

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        static Parameter paramInfo;

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;

        paramInfo.name = nullptr;
        paramInfo.unit = nullptr;
        paramInfo.ranges.def       = 0.0f;
        paramInfo.ranges.min       = 0.0f;
        paramInfo.ranges.max       = 1.0f;
        paramInfo.ranges.step      = PARAMETER_RANGES_DEFAULT_STEP;
        paramInfo.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        paramInfo.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        paramInfo.scalePointCount  = 0;
        paramInfo.scalePoints      = nullptr;

        if (index >= 1 && index <= 72)
        {
            uint32_t ri = index % 24;

            switch (ri)
            {
            case 1:
                paramInfo.name = "oct";
                break;
            case 2:
                paramInfo.name = "cent";
                break;
            case 3:
                paramInfo.name = "phaseOffset";
                break;
            case 4:
                paramInfo.name = "phaseIncOffset";
                break;
            case 5:
                paramInfo.name = "filter";
                break;
            case 6:
                paramInfo.name = "filter";
                break;
            case 7:
                paramInfo.name = "filter";
                break;
            case 8:
                paramInfo.name = "filter";
                break;
            case 9:
                paramInfo.name = "F ADSR";
                break;
            case 10:
                paramInfo.name = "F ADSR";
                break;
            case 11:
                paramInfo.name = "F ADSR";
                break;
            case 12:
                paramInfo.name = "F ADSR";
                break;
            case 13:
                paramInfo.name = "F velocity";
                break;
            case 14:
                paramInfo.name = "A ADSR";
                break;
            case 15:
                paramInfo.name = "A ADSR";
                break;
            case 16:
                paramInfo.name = "A ADSR";
                break;
            case 17:
                paramInfo.name = "A ADSR";
                break;
            case 18:
                paramInfo.name = "A velocity";
                break;
            case 19:
                paramInfo.name = "lfoC";
                break;
            case 20:
                paramInfo.name = "lfoA";
                break;
            case 21:
                paramInfo.name = "lfoF";
                break;
            case 22:
                paramInfo.name = "fx vol D";
                break;
            case 23:
                paramInfo.name = "fx vol C";
                break;
            case 24:
            case 0:
                paramInfo.name = "fx vol R";
                break;
            default:
                paramInfo.name = "unknown2";
                break;
            }

            paramInfo.hints = static_cast<ParameterHints>(hints);
            return &paramInfo;
        }

        switch (index)
        {
        case 0:
            paramInfo.name = "Main volume";
            break;
        case 73:
            paramInfo.name = "Delay Time";
            break;
        case 74:
            paramInfo.name = "Delay Feedback";
            break;
        case 75:
            paramInfo.name = "Delay Volume";
            break;
        case 76:
            paramInfo.name = "Chorus Rate";
            break;
        case 77:
            paramInfo.name = "Chorus Depth";
            break;
        case 78:
            paramInfo.name = "Chorus Volume";
            break;
        case 79:
            paramInfo.name = "Reverb Size";
            break;
        case 80:
            paramInfo.name = "Reverb Width";
            break;
        case 81:
            paramInfo.name = "Reverb Damp";
            break;
        case 82:
            paramInfo.name = "Reverb Volume";
            break;
        case 83:
            paramInfo.name = "Wave1 Panning";
            break;
        case 84:
            paramInfo.name = "Wave2 Panning";
            break;
        case 85:
            paramInfo.name = "Wave3 Panning";
            break;
        case 86:
            paramInfo.name = "Wave1 Volume";
            break;
        case 87:
            paramInfo.name = "Wave2 Volume";
            break;
        case 88:
            paramInfo.name = "Wave3 Volume";
            break;
        case 89:
            paramInfo.name = "Wave1 on/off";
            break;
        case 90:
            paramInfo.name = "Wave2 on/off";
            break;
        case 91:
            paramInfo.name = "Wave3 on/off";
            break;
        default:
            paramInfo.name = "unknown";
            break;
        }

        paramInfo.hints = static_cast<ParameterHints>(hints);
        return &paramInfo;
    }

    float getParameterValue(const uint32_t index) const override
    {
        return (index < kParamCount) ? fParameters[index] : 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
        {
            fParameters[index] = value;
            fSynth.update(index);
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** outBuffer, const uint32_t frames, const MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        const TimeInfo* const timeInfo(getTimeInfo());
        const double bpm((timeInfo != nullptr && timeInfo->bbt.valid) ? timeInfo->bbt.beatsPerMinute : 120.0);

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const MidiEvent* const midiEvent(&midiEvents[i]);

            const uint8_t status(MIDI_GET_STATUS_FROM_DATA(midiEvent->data));

            if (status == MIDI_STATUS_NOTE_ON)
            {
                fSynth.playNote(midiEvent->data[1], midiEvent->data[2], 0, 1);
            }
            else if (status == MIDI_STATUS_NOTE_OFF)
            {
                fSynth.releaseNote(midiEvent->data[1], 0, 1);
            }
            else if (status == MIDI_STATUS_CONTROL_CHANGE)
            {
                const uint8_t control(midiEvent->data[1]);

                if (control == MIDI_CONTROL_ALL_SOUND_OFF)
                    fSynth.kill();
                else if (control == MIDI_CONTROL_ALL_NOTES_OFF)
                    fSynth.releaseAll(1);
            }
        }

        if (obf->getNumSamples() != (int)frames)
        {
                obf->setSize(2,  frames, 0, 0, 1);
                abf->setSize(2,  frames, 0, 0, 1);
                dbf1->setSize(2, frames, 0, 0, 1);
                dbf2->setSize(2, frames, 0, 0, 1);
                dbf3->setSize(2, frames, 0, 0, 1);
        }

        obf ->clear();
        dbf1->clear();
        dbf2->clear();
        dbf3->clear();

        fSynth.doProcess(*obf, *abf, *dbf1, *dbf2, *dbf3);

        if (fParameters[75] > 0.001f) fDelay.processBlock(dbf1, bpm);
        if (fParameters[78] > 0.001f) fChorus.processBlock(dbf2);
        if (fParameters[82] > 0.001f) fReverb.processBlock(dbf3);

        AudioSampleBuffer output(outBuffer, 2, 0, frames);
        output.clear();

        obf->addFrom(0, 0, *dbf1, 0, 0, frames, fParameters[75]);
        obf->addFrom(1, 0, *dbf1, 1, 0, frames, fParameters[75]);
        obf->addFrom(0, 0, *dbf2, 0, 0, frames, fParameters[78]);
        obf->addFrom(1, 0, *dbf2, 1, 0, frames, fParameters[78]);
        obf->addFrom(0, 0, *dbf3, 0, 0, frames, fParameters[82]);
        obf->addFrom(1, 0, *dbf3, 1, 0, frames, fParameters[82]);

        output.addFrom(0, 0, *obf, 0, 0, frames, fParameters[0]);
        output.addFrom(1, 0, *obf, 1, 0, frames, fParameters[0]);
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (fWindow == nullptr)
            {
                fWindow = new HelperWindow2();
                fWindow->setName(getUiName());
            }

            if (fView == nullptr)
                fView = new VexEditorComponent();

            fWindow->show(fView);
        }
        else if (fWindow != nullptr)
        {
            fWindow->hide();

            fView = nullptr;
            fWindow = nullptr;
        }
    }

    void uiIdle() override
    {
        if (fWindow == nullptr)
            return;

        if (fWindow->wasClosedByUser())
        {
            uiShow(false);
            uiClosed();
        }
    }

    void uiSetParameterValue(const uint32_t, const float) override
    {
        if (fView == nullptr)
            return;

        //fView->update();
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        delete obf;
        delete abf;
        delete dbf1;
        delete dbf2;
        delete dbf3;

        obf  = new AudioSampleBuffer(2, bufferSize);
        abf  = new AudioSampleBuffer(2, bufferSize);
        dbf1 = new AudioSampleBuffer(2, bufferSize);
        dbf2 = new AudioSampleBuffer(2, bufferSize);
        dbf3 = new AudioSampleBuffer(2, bufferSize);
    }

    void sampleRateChanged(const double sampleRate) override
    {
        fChorus.setSampleRate(sampleRate);
        fDelay.setSampleRate(sampleRate);
        fSynth.setSampleRate(sampleRate);
    }

    void uiNameChanged(const char* const uiName) override
    {
        if (fWindow == nullptr)
            return;

        fWindow->setName(uiName);
    }

private:
    float fParameters[92];

    AudioSampleBuffer* obf;
    AudioSampleBuffer* abf;
    AudioSampleBuffer* dbf1; // delay
    AudioSampleBuffer* dbf2; // chorus
    AudioSampleBuffer* dbf3; // reverb

    VexChorus fChorus;
    VexDelay fDelay;
    VexReverb fReverb;
    VexSyntModule fSynth;

    ScopedPointer<VexEditorComponent> fView;
    ScopedPointer<HelperWindow2> fWindow;

    PluginClassEND(VexSynthPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexSynthPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor vexSynthDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
    /* hints     */ static_cast<PluginHints>(PLUGIN_HAS_GUI|PLUGIN_NEEDS_SINGLE_THREAD|PLUGIN_USES_TIME),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ VexSynthPlugin::kParamCount,
    /* paramOuts */ 0,
    /* name      */ "VexSynth",
    /* label     */ "vexSynth",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(VexSynthPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_vex_synth()
{
    carla_register_native_plugin(&vexSynthDesc);
}

// -----------------------------------------------------------------------
