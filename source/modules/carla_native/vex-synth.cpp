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

#include "JucePluginWindow.hpp"

using namespace juce;

#include "vex/VexArp.h"
#include "vex/VexChorus.h"
#include "vex/VexDelay.h"
#include "vex/VexReverb.h"
#include "vex/VexSyntModule.h"

#include "vex/PeggyViewComponent.h"
#include "vex/gui/SnappingSlider.h"
#include "vex/lookandfeel/MyLookAndFeel.h"
#include "vex/resources/Resources.h"

// -----------------------------------------------------------------------
// Copied from juce_AudioProcessor.cpp

// magic number to identify memory blocks that we've stored as XML
const uint32 magicXmlNumber = 0x21324356;

void copyXmlToBinary(const XmlElement& xml, juce::MemoryBlock& destData)
{
    const String xmlString(xml.createDocument(String::empty, true, false));
    const size_t stringLength = xmlString.getNumBytesAsUTF8();

    destData.setSize(stringLength + 9);

    uint32* const d = static_cast<uint32*>(destData.getData());
    d[0] = ByteOrder::swapIfBigEndian((const uint32) magicXmlNumber);
    d[1] = ByteOrder::swapIfBigEndian((const uint32) stringLength);

    xmlString.copyToUTF8((CharPointer_UTF8::CharType*) (d + 2), stringLength + 1);
}

XmlElement* getXmlFromBinary(const void* data, const int sizeInBytes)
{
    if (sizeInBytes > 8 && ByteOrder::littleEndianInt(data) == magicXmlNumber)
    {
        const int stringLength = (int) ByteOrder::littleEndianInt (addBytesToPointer (data, 4));

        if (stringLength > 0)
            return XmlDocument::parse (String::fromUTF8 (static_cast<const char*> (data) + 8, jmin ((sizeInBytes - 8), stringLength)));
    }

    return nullptr;
}

// -----------------------------------------------------------------------

class VexEditorComponent : public Component,
                           public Timer,
                           public ComboBox::Listener,
                           public Slider::Listener,
                           public Button::Listener,
                           public PeggyViewComponent::Callback // ignored
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void getChangedParameters(bool params[92]) = 0;
        virtual float getFilterParameterValue(const uint32_t index) const = 0;
        virtual String getFilterWaveName(const int part) const = 0;
        virtual void editorParameterChanged(const uint32_t index, const float value) = 0;
        virtual void editorWaveChanged(const int part, const String& wave) = 0;
    };

    VexEditorComponent(Callback* const callback, VexArpSettings& arpSet1, VexArpSettings& arpSet2, VexArpSettings& arpSet3)
        : fCallback(callback),
          fNeedsUpdate(false)
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

        addAndMakeVisible(comboBox2 = new ComboBox (String::empty));
        comboBox2->setEditableText(false);
        comboBox2->setJustificationType(Justification::centredLeft);
        comboBox2->setTextWhenNothingSelected(String("silent"));
        comboBox2->setTextWhenNoChoicesAvailable(String("silent"));
        comboBox2->addListener(this);
        comboBox2->setColour(ComboBox::backgroundColourId, Colours::black);
        comboBox2->setColour(ComboBox::textColourId, Colours::lightgrey);
        comboBox2->setColour(ComboBox::outlineColourId, Colours::grey);
        comboBox2->setColour(ComboBox::buttonColourId, Colours::grey);
        comboBox2->setWantsKeyboardFocus(false);
        comboBox2->setLookAndFeel(&mlaf);

        addAndMakeVisible(comboBox3 = new ComboBox(String::empty));
        comboBox3->setEditableText(false);
        comboBox3->setJustificationType(Justification::centredLeft);
        comboBox3->setTextWhenNothingSelected(String("silent"));
        comboBox3->setTextWhenNoChoicesAvailable(String("silent"));
        comboBox3->addListener(this);
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

        //Make sliders
        for (int i = 0; i < kSliderCount; ++i)
        {
            addAndMakeVisible(sliders[i] = new SnappingSlider("s"));
            sliders[i]->setSliderStyle(Slider::RotaryVerticalDrag);
            sliders[i]->setRange(0, 1, 0);
            sliders[i]->setSnap(0.0f, 0.0f);
            sliders[i]->setLookAndFeel(&mlaf);
            sliders[i]->addListener(this);
        }

        //Adjust the center of some
        sliders[1]->setRange(0, 1, 0.25f);
        sliders[1]->setSnap(0.5f, 0.05f);
        sliders[2]->setSnap(0.5f, 0.05f);
        sliders[3]->setSnap(0.5f, 0.05f);
        sliders[4]->setSnap(0.5f, 0.05f);
        sliders[8]->setSnap(0.5f, 0.05f);
        sliders[13]->setSnap(0.5f, 0.05f);
        sliders[18]->setSnap(0.5f, 0.05f);

        int tmp = 24;
        sliders[1 + tmp]->setRange(0, 1, 0.25f);
        sliders[1 + tmp]->setSnap(0.5f, 0.05f);
        sliders[2 + tmp]->setSnap(0.5f, 0.05f);
        sliders[3 + tmp]->setSnap(0.5f, 0.05f);
        sliders[4 + tmp]->setSnap(0.5f, 0.05f);
        sliders[8 + tmp]->setSnap(0.5f, 0.05f);
        sliders[13 + tmp]->setSnap(0.5f, 0.05f);
        sliders[18 + tmp]->setSnap(0.5f, 0.05f);

        tmp = 48;
        sliders[1 + tmp]->setRange(0, 1, 0.25f);
        sliders[1 + tmp]->setSnap(0.5f, 0.05f);
        sliders[2 + tmp]->setSnap(0.5f, 0.05f);
        sliders[3 + tmp]->setSnap(0.5f, 0.05f);
        sliders[4 + tmp]->setSnap(0.5f, 0.05f);
        sliders[8 + tmp]->setSnap(0.5f, 0.05f);
        sliders[13 + tmp]->setSnap(0.5f, 0.05f);
        sliders[18 + tmp]->setSnap(0.5f, 0.05f);

        sliders[83]->setSnap(0.5f, 0.05f);
        sliders[84]->setSnap(0.5f, 0.05f);
        sliders[85]->setSnap(0.5f, 0.05f);

        sliders[75]->setSnap(0.0f, 0.05f);
        sliders[78]->setSnap(0.0f, 0.05f);
        sliders[82]->setSnap(0.0f, 0.05f);

        //PART ON/OFF
        addAndMakeVisible(TB1 = new TextButton ("new button"));
        TB1->setButtonText(String::empty);
        TB1->addListener(this);
        TB1->setColour(TextButton::buttonColourId, Colours::darkred.withAlpha(0.5f));
        TB1->setColour(TextButton::buttonOnColourId, Colours::red);
        TB1->setClickingTogglesState(true);
        TB1->setToggleState(false, dontSendNotification);

        addAndMakeVisible(TB2 = new TextButton ("new button"));
        TB2->setButtonText(String::empty);
        TB2->addListener(this);
        TB2->setColour(TextButton::buttonColourId, Colours::darkred.withAlpha(0.5f));
        TB2->setColour(TextButton::buttonOnColourId, Colours::red);
        TB2->setClickingTogglesState(true);
        TB2->setToggleState(false, dontSendNotification);

        addAndMakeVisible(TB3 = new TextButton ("new button"));
        TB3->setButtonText(String::empty);
        TB3->addListener(this);
        TB3->setColour(TextButton::buttonColourId, Colours::darkred.withAlpha(0.5f));
        TB3->setColour(TextButton::buttonOnColourId, Colours::red);
        TB3->setClickingTogglesState(true);
        TB3->setToggleState(false, dontSendNotification);

        //Peggy ON/OFF
        addAndMakeVisible(TB4 = new TextButton ("new button"));
        TB4->setButtonText(String::empty);
        TB4->addListener(this);
        TB4->setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
        TB4->setColour(TextButton::buttonOnColourId, Colours::blue);
        TB4->setClickingTogglesState(true);
        TB4->setToggleState(false, dontSendNotification);

        addAndMakeVisible(TB5 = new TextButton ("new button"));
        TB5->setButtonText(String::empty);
        TB5->addListener(this);
        TB5->setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
        TB5->setColour(TextButton::buttonOnColourId, Colours::blue);
        TB5->setClickingTogglesState(true);
        TB5->setToggleState(false, dontSendNotification);

        addAndMakeVisible(TB6 = new TextButton ("new button"));
        TB6->setButtonText(String::empty);
        TB6->addListener(this);
        TB6->setColour(TextButton::buttonColourId, Colours::darkblue.withAlpha(0.5f));
        TB6->setColour(TextButton::buttonOnColourId, Colours::blue);
        TB6->setClickingTogglesState(true);
        TB6->setToggleState(false, dontSendNotification);

        addChildComponent(p1 = new PeggyViewComponent(arpSet1, this));
        p1->setLookAndFeel(&mlaf);
        addChildComponent(p2 = new PeggyViewComponent(arpSet2, this));
        p2->setLookAndFeel(&mlaf);
        addChildComponent(p3 = new PeggyViewComponent(arpSet3, this));
        p3->setLookAndFeel(&mlaf);

        setSize(800,500);
        updateParametersFromFilter(true);

        startTimer(50);
    }

    ~VexEditorComponent()
    {
        stopTimer();
        removeAllChildren();
    }

    void setNeedsUpdate()
    {
        fNeedsUpdate = true;
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

        // ...
        sliders[73]->setBounds(623, 23, 28, 28);
        sliders[74]->setBounds(686, 23, 28, 28);
        sliders[75]->setBounds(748, 23, 28, 28);
        sliders[76]->setBounds(623, 98, 28, 28);
        sliders[77]->setBounds(686, 98, 28, 28);
        sliders[78]->setBounds(748, 98, 28, 28);
        sliders[79]->setBounds(611, 173, 28, 28);
        sliders[80]->setBounds(661, 173, 28, 28);
        sliders[81]->setBounds(711, 173, 28, 28);
        sliders[82]->setBounds(761, 173, 28, 28);
        sliders[85]->setBounds(711, 373, 28, 28);
        sliders[84]->setBounds(661, 373, 28, 28);
        sliders[83]->setBounds(611, 373, 28, 28);
        sliders[86]->setBounds(611, 449, 28, 28);
        sliders[87]->setBounds(661, 449, 28, 28);
        sliders[88]->setBounds(711, 449, 28, 28);
        sliders[0]->setBounds(761, 449, 28, 28);

        // ...
        sliders[1]->setBounds(11, 73, 28, 28);
        sliders[2]->setBounds(61, 73, 28, 28);
        sliders[3]->setBounds(111, 73, 28, 28);
        sliders[4]->setBounds(161, 73, 28, 28);
        sliders[24 + 1]->setBounds(211, 73, 28, 28);
        sliders[24 + 2]->setBounds(261, 73, 28, 28);
        sliders[24 + 3]->setBounds(311, 73, 28, 28);
        sliders[24 + 4]->setBounds(361, 73, 28, 28);
        sliders[48 + 1]->setBounds(411, 73, 28, 28);
        sliders[48 + 2]->setBounds(461, 73, 28, 28);
        sliders[48 + 3]->setBounds(511, 73, 28, 28);
        sliders[48 + 4]->setBounds(561, 73, 28, 28);

        // ...
        sliders[5]->setBounds(11, 149, 28, 28);
        sliders[6]->setBounds(61, 149, 28, 28);
        sliders[7]->setBounds(111, 149, 28, 28);
        sliders[8]->setBounds(161, 149, 28, 28);
        sliders[24 + 5]->setBounds(211, 149, 28, 28);
        sliders[24 + 6]->setBounds(261, 149, 28, 28);
        sliders[24 + 7]->setBounds(311, 149, 28, 28);
        sliders[24 + 8]->setBounds(361, 149, 28, 28);
        sliders[48 + 5]->setBounds(411, 149, 28, 28);
        sliders[48 + 6]->setBounds(461, 149, 28, 28);
        sliders[48 + 7]->setBounds(511, 149, 28, 28);
        sliders[48 + 8]->setBounds(561, 149, 28, 28);

        // ...
        sliders[9]->setBounds(11, 223, 28, 28);
        sliders[10]->setBounds(48, 223, 28, 28);
        sliders[11]->setBounds(86, 223, 28, 28);
        sliders[12]->setBounds(123, 223, 28, 28);
        sliders[13]->setBounds(161, 223, 28, 28);
        sliders[24 + 9]->setBounds(211, 223, 28, 28);
        sliders[24 + 10]->setBounds(248, 223, 28, 28);
        sliders[24 + 11]->setBounds(286, 223, 28, 28);
        sliders[24 + 12]->setBounds(323, 223, 28, 28);
        sliders[24 + 13]->setBounds(361, 223, 28, 28);
        sliders[48 + 9]->setBounds(411, 223, 28, 28);
        sliders[48 + 10]->setBounds(448, 223, 28, 28);
        sliders[48 + 11]->setBounds(486, 223, 28, 28);
        sliders[48 + 12]->setBounds(523, 223, 28, 28);
        sliders[48 + 13]->setBounds(561, 223, 28, 28);

        // ...
        sliders[14]->setBounds(11, 298, 28, 28);
        sliders[15]->setBounds(48, 298, 28, 28);
        sliders[16]->setBounds(86, 298, 28, 28);
        sliders[17]->setBounds(123, 298, 28, 28);
        sliders[18]->setBounds(161, 298, 28, 28);
        sliders[24 + 14]->setBounds(211, 298, 28, 28);
        sliders[24 + 15]->setBounds(248, 298, 28, 28);
        sliders[24 + 16]->setBounds(286, 298, 28, 28);
        sliders[24 + 17]->setBounds(323, 298, 28, 28);
        sliders[24 + 18]->setBounds(361, 298, 28, 28);
        sliders[48 + 14]->setBounds(411, 298, 28, 28);
        sliders[48 + 15]->setBounds(448, 298, 28, 28);
        sliders[48 + 16]->setBounds(486, 298, 28, 28);
        sliders[48 + 17]->setBounds(523, 298, 28, 28);
        sliders[48 + 18]->setBounds(561, 298, 28, 28);

        // ...
        sliders[19]->setBounds(11, 374, 28, 28);
        sliders[20]->setBounds(86, 374, 28, 28);
        sliders[21]->setBounds(161, 374, 28, 28);
        sliders[24 + 19]->setBounds(211, 374, 28, 28);
        sliders[24 + 20]->setBounds(286, 374, 28, 28);
        sliders[24 + 21]->setBounds(361, 374, 28, 28);
        sliders[48 + 19]->setBounds(411, 374, 28, 28);
        sliders[48 + 20]->setBounds(486, 374, 28, 28);
        sliders[48 + 21]->setBounds(561, 374, 28, 28);

        // ...
        sliders[22]->setBounds(11, 448, 28, 28);
        sliders[23]->setBounds(86, 448, 28, 28);
        sliders[24]->setBounds(161, 448, 28, 28);
        sliders[24 + 22]->setBounds(211, 448, 28, 28);
        sliders[24 + 23]->setBounds(286, 448, 28, 28);
        sliders[48]->setBounds(361, 448, 28, 28);
        sliders[48 + 22]->setBounds(411, 448, 28, 28);
        sliders[48 + 23]->setBounds(486, 448, 28, 28);
        sliders[48 + 24]->setBounds(561, 448, 28, 28);

        TB1->setBounds(174, 4, 13, 13);
        TB2->setBounds(374, 4, 13, 13);
        TB3->setBounds(574, 4, 13, 13);
        TB4->setBounds(154, 4, 13, 13);
        TB5->setBounds(354, 4, 13, 13);
        TB6->setBounds(554, 4, 13, 13);

        p1->setBounds(10, 20, 207, 280);
        p2->setBounds(210, 20, 207, 280);
        p3->setBounds(410, 20, 207, 280);
    }

    void timerCallback() override
    {
        if (fNeedsUpdate)
        {
            updateParametersFromFilter(true);
            fNeedsUpdate = false;
        }
        else
        {
            updateParametersFromFilter(false);
        }
    }

    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
    {
        if (comboBoxThatHasChanged == comboBox1)
            fCallback->editorWaveChanged(1, comboBox1->getText());
        else if (comboBoxThatHasChanged == comboBox2)
            fCallback->editorWaveChanged(2, comboBox2->getText());
        else if (comboBoxThatHasChanged == comboBox3)
            fCallback->editorWaveChanged(3, comboBox3->getText());
    }

    void sliderValueChanged(Slider* sliderThatWasMoved) override
    {
        for (int i = 0; i < kSliderCount; ++i)
        {
            if (sliders[i] == sliderThatWasMoved)
            {
                fCallback->editorParameterChanged(i, (float)sliderThatWasMoved->getValue());
                return;
            }
        }
    }

    void buttonClicked(Button* buttonThatWasClicked) override
    {
        if (buttonThatWasClicked == TB1)
            fCallback->editorParameterChanged(89, TB1->getToggleState() ? 1.0f : 0.0f);
        else if (buttonThatWasClicked == TB2)
            fCallback->editorParameterChanged(90, TB2->getToggleState() ? 1.0f : 0.0f);
        else if (buttonThatWasClicked == TB3)
            fCallback->editorParameterChanged(91, TB3->getToggleState() ? 1.0f : 0.0f);
        else if (buttonThatWasClicked == TB4)
            p1->setVisible(!p1->isVisible());
        else if (buttonThatWasClicked == TB5)
            p2->setVisible(!p2->isVisible());
        else if (buttonThatWasClicked == TB6)
            p3->setVisible(!p3->isVisible());
    }

    void arpParameterChanged(const uint32_t) override
    {
        // unused, we don't need to register arp settings changes in synth
    }

    void updateParametersFromFilter(const bool all)
    {
        if (all)
        {
            for (int i = 0; i < kSliderCount; ++i)
                sliders[i]->setValue(fCallback->getFilterParameterValue(i), dontSendNotification);

            TB1->setToggleState(fCallback->getFilterParameterValue(89) > 0.5f ? true : false, dontSendNotification);
            TB2->setToggleState(fCallback->getFilterParameterValue(90) > 0.5f ? true : false, dontSendNotification);
            TB3->setToggleState(fCallback->getFilterParameterValue(91) > 0.5f ? true : false, dontSendNotification);

            comboBox1->setText(fCallback->getFilterWaveName(1), dontSendNotification);
            comboBox2->setText(fCallback->getFilterWaveName(2), dontSendNotification);
            comboBox3->setText(fCallback->getFilterWaveName(3), dontSendNotification);

            p1->update();
            p2->update();
            p3->update();
        }
        else
        {
            bool params[92];
            fCallback->getChangedParameters(params);

            for (int i=0; i < 89; ++i)
            {
                if (params[i])
                    sliders[i]->setValue(fCallback->getFilterParameterValue(i), dontSendNotification);
            }

            if (params[89]) TB1->setToggleState(fCallback->getFilterParameterValue(89) > 0.5f ? true : false, dontSendNotification);
            if (params[90]) TB2->setToggleState(fCallback->getFilterParameterValue(90) > 0.5f ? true : false, dontSendNotification);
            if (params[91]) TB3->setToggleState(fCallback->getFilterParameterValue(91) > 0.5f ? true : false, dontSendNotification);
        }
    }

private:
    static const int kSliderCount = 89;

    Callback* const fCallback;
    volatile bool fNeedsUpdate;

    Image internalCachedImage1;
    MyLookAndFeel mlaf;

    ScopedPointer<ComboBox> comboBox1;
    ScopedPointer<ComboBox> comboBox2;
    ScopedPointer<ComboBox> comboBox3;

    ScopedPointer<SnappingSlider> sliders[kSliderCount];

    ScopedPointer<TextButton> TB1;
    ScopedPointer<TextButton> TB2;
    ScopedPointer<TextButton> TB3;
    ScopedPointer<TextButton> TB4;
    ScopedPointer<TextButton> TB5;
    ScopedPointer<TextButton> TB6;

    ScopedPointer<PeggyViewComponent> p1;
    ScopedPointer<PeggyViewComponent> p2;
    ScopedPointer<PeggyViewComponent> p3;

    VexArpSettings _d;
};

// -----------------------------------------------------------------------

class VexSynthPlugin : public PluginClass,
                       public VexEditorComponent::Callback
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
          fArp1(&fArpSet1),
          fArp2(&fArpSet2),
          fArp3(&fArpSet3),
          fChorus(fParameters),
          fDelay(fParameters),
          fReverb(fParameters),
          fSynth(fParameters)
    {
        std::memset(fParameters, 0, sizeof(float)*kParamCount);
        std::memset(fParamsChanged, 0, sizeof(bool)*92);

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
        fParameters[90] = 0.0f; // wave2 on/off
        fParameters[91] = 0.0f; // wave3 on/off

        bufferSizeChanged(getBufferSize());
        sampleRateChanged(getSampleRate());

        for (unsigned int i = 0; i < kParamCount; ++i)
            fSynth.update(i);

        fMidiInBuffer.ensureSize(512*4);
    }

    ~VexSynthPlugin() override
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
                paramInfo.ranges.def = 0.5f;
                break;
            case 2:
                paramInfo.name = "cent";
                paramInfo.ranges.def = 0.5f;
                break;
            case 3:
                paramInfo.name = "phaseOffset";
                paramInfo.ranges.def = 0.5f;
                break;
            case 4:
                paramInfo.name = "phaseIncOffset";
                paramInfo.ranges.def = 0.5f;
                break;
            case 5:
                paramInfo.name = "filter";
                paramInfo.ranges.def = 0.9f;
                break;
            case 6:
                paramInfo.name = "filter";
                paramInfo.ranges.def = 0.0f;
                break;
            case 7:
                paramInfo.name = "filter";
                paramInfo.ranges.def = 1.0f;
                break;
            case 8:
                paramInfo.name = "filter";
                paramInfo.ranges.def = 0.5f;
                break;
            case 9:
                paramInfo.name = "F ADSR";
                paramInfo.ranges.def = 0.0f;
                break;
            case 10:
                paramInfo.name = "F ADSR";
                paramInfo.ranges.def = 0.2f;
                break;
            case 11:
                paramInfo.name = "F ADSR";
                paramInfo.ranges.def = 0.0f;
                break;
            case 12:
                paramInfo.name = "F ADSR";
                paramInfo.ranges.def = 0.5f;
                break;
            case 13:
                paramInfo.name = "F velocity";
                paramInfo.ranges.def = 0.5f;
                break;
            case 14:
                paramInfo.name = "A ADSR";
                paramInfo.ranges.def = 0.0f;
                break;
            case 15:
                paramInfo.name = "A ADSR";
                paramInfo.ranges.def = 0.3f;
                break;
            case 16:
                paramInfo.name = "A ADSR";
                paramInfo.ranges.def = 0.7f;
                break;
            case 17:
                paramInfo.name = "A ADSR";
                paramInfo.ranges.def = 0.1f;
                break;
            case 18:
                paramInfo.name = "A velocity";
                paramInfo.ranges.def = 0.5f;
                break;
            case 19:
                paramInfo.name = "lfoC";
                paramInfo.ranges.def = 0.5f;
                break;
            case 20:
                paramInfo.name = "lfoA";
                paramInfo.ranges.def = 0.0f;
                break;
            case 21:
                paramInfo.name = "lfoF";
                paramInfo.ranges.def = 0.0f;
                break;
            case 22:
                paramInfo.name = "fx vol D";
                paramInfo.ranges.def = 0.5f;
                break;
            case 23:
                paramInfo.name = "fx vol C";
                paramInfo.ranges.def = 0.5f;
                break;
            case 24:
            case 0:
                paramInfo.name = "fx vol R";
                paramInfo.ranges.def = 0.5f;
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
            paramInfo.ranges.def = 1.0f;
            break;
        case 73:
            hints |= PARAMETER_IS_INTEGER;
            paramInfo.name = "Delay Time";
            paramInfo.unit = "steps";
            paramInfo.ranges.def = 4.0f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 8.0f;
            break;
        case 74:
            paramInfo.name = "Delay Feedback";
            paramInfo.unit = "%";
            paramInfo.ranges.def = 40.0f;
            paramInfo.ranges.min = 0.0f;
            paramInfo.ranges.max = 100.0f;
            break;
        case 75:
            paramInfo.name = "Delay Volume";
            paramInfo.ranges.def = 0.0f;
            break;
        case 76:
            paramInfo.name = "Chorus Rate";
            paramInfo.ranges.def = 0.3f;
            break;
        case 77:
            paramInfo.name = "Chorus Depth";
            paramInfo.ranges.def = 0.6f;
            break;
        case 78:
            paramInfo.name = "Chorus Volume";
            paramInfo.ranges.def = 0.5f;
            break;
        case 79:
            paramInfo.name = "Reverb Size";
            paramInfo.ranges.def = 0.6f;
            break;
        case 80:
            paramInfo.name = "Reverb Width";
            paramInfo.ranges.def = 0.7f;
            break;
        case 81:
            paramInfo.name = "Reverb Damp";
            paramInfo.ranges.def = 0.6f;
            break;
        case 82:
            paramInfo.name = "Reverb Volume";
            paramInfo.ranges.def = 0.0f;
            break;
        case 83:
            paramInfo.name = "Wave1 Panning";
            paramInfo.ranges.def = 0.5f;
            break;
        case 84:
            paramInfo.name = "Wave2 Panning";
            paramInfo.ranges.def = 0.5f;
            break;
        case 85:
            paramInfo.name = "Wave3 Panning";
            paramInfo.ranges.def = 0.5f;
            break;
        case 86:
            paramInfo.name = "Wave1 Volume";
            paramInfo.ranges.def = 0.5f;
            break;
        case 87:
            paramInfo.name = "Wave2 Volume";
            paramInfo.ranges.def = 0.5f;
            break;
        case 88:
            paramInfo.name = "Wave3 Volume";
            paramInfo.ranges.def = 0.5f;
            break;
        case 89:
            hints |= PARAMETER_IS_BOOLEAN;
            paramInfo.name = "Wave1 on/off";
            paramInfo.ranges.def = 1.0f;
            break;
        case 90:
            hints |= PARAMETER_IS_BOOLEAN;
            paramInfo.name = "Wave2 on/off";
            paramInfo.ranges.def = 0.0f;
            break;
        case 91:
            hints |= PARAMETER_IS_BOOLEAN;
            paramInfo.name = "Wave3 on/off";
            paramInfo.ranges.def = 0.0f;
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
        if (index >= kParamCount)
            return 0.0f;

        switch (index)
        {
        case 73:
            return fParameters[index] * 8.0f;
        case 74:
            return fParameters[index] * 100.0f;
        default:
            return fParameters[index];
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index >= kParamCount)
            return;

        float realValue;

        switch (index)
        {
        case 73:
            realValue = value/8.0f;
            break;
        case 74:
            realValue = value/100.0f;
            break;
        default:
            realValue = value;
            break;
        }

        fParameters[index] = realValue;
        fParamsChanged[index] = true;
        fSynth.update(index);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** outBuffer, const uint32_t frames, const MidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        // Get time info
        const TimeInfo* const timeInfo(getTimeInfo());

        bool   timePlaying = false;
        double ppqPos      = 0.0;
        double barStartPos = 0.0;
        double bpm         = 120.0;

        if (timeInfo != nullptr)
        {
            timePlaying = timeInfo->playing;

            if (timeInfo->bbt.valid)
            {
                double ppqBar  = double(timeInfo->bbt.bar - 1) * timeInfo->bbt.beatsPerBar;
                double ppqBeat = double(timeInfo->bbt.beat - 1);
                double ppqTick = double(timeInfo->bbt.tick) / timeInfo->bbt.ticksPerBeat;

                ppqPos      = ppqBar + ppqBeat + ppqTick;
                barStartPos = ppqBar;
                bpm         = timeInfo->bbt.beatsPerMinute;
            }
        }

        // put carla events in juce midi buffer
        fMidiInBuffer.clear();

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const MidiEvent* const midiEvent(&midiEvents[i]);
            fMidiInBuffer.addEvent(MidiMessage(midiEvent->data, midiEvent->size), midiEvent->time);
        }

        // process MIDI (arppeggiator)
        const MidiBuffer& part1Midi(fArpSet1.on ? fArp1.processMidi(fMidiInBuffer, timePlaying, ppqPos, barStartPos, bpm, frames) : fMidiInBuffer);
        const MidiBuffer& part2Midi(fArpSet2.on ? fArp2.processMidi(fMidiInBuffer, timePlaying, ppqPos, barStartPos, bpm, frames) : fMidiInBuffer);
        const MidiBuffer& part3Midi(fArpSet3.on ? fArp3.processMidi(fMidiInBuffer, timePlaying, ppqPos, barStartPos, bpm, frames) : fMidiInBuffer);

        int snum;
        MidiMessage midiMessage(0xf4);
        MidiBuffer::Iterator Iterator1(part1Midi);

        while (Iterator1.getNextEvent(midiMessage, snum))
        {
            if (midiMessage.isNoteOn())
                fSynth.playNote(midiMessage.getNoteNumber(), midiMessage.getVelocity(), snum, 1);
            else if (midiMessage.isNoteOff())
                fSynth.releaseNote(midiMessage.getNoteNumber(), snum, 1 );
            else if (midiMessage.isAllSoundOff())
                fSynth.kill();
            else if (midiMessage.isAllNotesOff())
                fSynth.releaseAll(snum);
        }

        MidiBuffer::Iterator Iterator2(part2Midi);

        while (Iterator2.getNextEvent(midiMessage, snum))
        {
            if (midiMessage.isNoteOn())
                fSynth.playNote(midiMessage.getNoteNumber(), midiMessage.getVelocity(), snum, 2);
            else if (midiMessage.isNoteOff())
                fSynth.releaseNote(midiMessage.getNoteNumber(), snum, 2 );
        }

        MidiBuffer::Iterator Iterator3(part3Midi);

        while (Iterator3.getNextEvent(midiMessage, snum))
        {
            if (midiMessage.isNoteOn())
                fSynth.playNote(midiMessage.getNoteNumber(), midiMessage.getVelocity(), snum, 3);
            else if (midiMessage.isNoteOff())
                fSynth.releaseNote(midiMessage.getNoteNumber(), snum, 3 );
        }

        fMidiInBuffer.clear();

        if (obf->getNumSamples() < (int)frames)
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
        MessageManagerLock mmLock;

        if (show)
        {
            if (fWindow == nullptr)
            {
                fWindow = new JucePluginWindow();
                fWindow->setName(getUiName());
            }

            if (fView == nullptr)
                fView = new VexEditorComponent(this, fArpSet1, fArpSet2, fArpSet3);

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

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        MemoryBlock destData;
        //destData.append(fParameters, sizeof(float) * kParamCount);
        destData.append(&fArpSet1, sizeof(VexArpSettings));
        destData.append(&fArpSet2, sizeof(VexArpSettings));
        destData.append(&fArpSet3, sizeof(VexArpSettings));

        XmlElement xmlState("VEX");

        xmlState.setAttribute("Wave1", fSynth.getWaveName(1));
        xmlState.setAttribute("Wave2", fSynth.getWaveName(2));
        xmlState.setAttribute("Wave3", fSynth.getWaveName(3));

        MemoryBlock tmp;
        copyXmlToBinary(xmlState, tmp);

        destData.append(tmp.getData(), tmp.getSize());

        return strdup(destData.toBase64Encoding().toRawUTF8());
    }

    void setState(const char* const data64) override
    {
        static const int kParamDataSize = 0; //sizeof(float)*kParamCount
        static const int xmlOffset      = kParamDataSize + sizeof(VexArpSettings) * 3;

        MemoryBlock destData;
        destData.fromBase64Encoding(data64);

        const char* const data(&destData[0]);
        const size_t  dataSize(destData.getSize());

        if (XmlElement* const xmlState = getXmlFromBinary(data + xmlOffset, dataSize - xmlOffset))
        {
            if (xmlState->hasTagName("VEX"))
            {
                fSynth.setWaveLater(1, xmlState->getStringAttribute("Wave1"));
                fSynth.setWaveLater(2, xmlState->getStringAttribute("Wave2"));
                fSynth.setWaveLater(3, xmlState->getStringAttribute("Wave3"));

                //std::memcpy(fParameters, data,                                               sizeof(float) * kParamCount);
                std::memcpy(&fArpSet1,   data + (kParamDataSize + sizeof(VexArpSettings)*0), sizeof(VexArpSettings));
                std::memcpy(&fArpSet2,   data + (kParamDataSize + sizeof(VexArpSettings)*1), sizeof(VexArpSettings));
                std::memcpy(&fArpSet3,   data + (kParamDataSize + sizeof(VexArpSettings)*2), sizeof(VexArpSettings));

                //for (unsigned int i = 0; i < kParamCount; ++i)
                //    fSynth.update(i);

                if (fView != nullptr)
                    fView->setNeedsUpdate();
            }

            delete xmlState;
        }
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
        fArp1.setSampleRate(sampleRate);
        fArp2.setSampleRate(sampleRate);
        fArp3.setSampleRate(sampleRate);
        fChorus.setSampleRate(sampleRate);
        fDelay.setSampleRate(sampleRate);
        fSynth.setSampleRate(sampleRate);
    }

    void uiNameChanged(const char* const uiName) override
    {
        if (fWindow == nullptr)
            return;

        MessageManagerLock mmLock;
        fWindow->setName(uiName);
    }

    // -------------------------------------------------------------------
    // Vex editor calls

    void getChangedParameters(bool params[92]) override
    {
        std::memcpy(params, fParamsChanged, sizeof(bool)*92);
        std::memset(fParamsChanged, 0, sizeof(bool)*92);
    }

    float getFilterParameterValue(const uint32_t index) const override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount, 0.0f)

        return fParameters[index];
    }

    String getFilterWaveName(const int part) const override
    {
        CARLA_SAFE_ASSERT_RETURN(part >= 1 && part <= 3, "")

        return fSynth.getWaveName(part);
    }

    void editorParameterChanged(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < kParamCount,)

        if (fParameters[index] == value)
            return;

        fParameters[index] = value;
        fSynth.update(index);

        uiParameterChanged(index, getParameterValue(index)); // some params have specific bounds
    }

    void editorWaveChanged(const int part, const String& wave) override
    {
        CARLA_SAFE_ASSERT_RETURN(part >= 1 && part <= 3,)

        fSynth.setWaveLater(part, wave);
    }

private:
    float fParameters[kParamCount];
    bool  fParamsChanged[92];

    AudioSampleBuffer* obf;
    AudioSampleBuffer* abf;
    AudioSampleBuffer* dbf1; // delay
    AudioSampleBuffer* dbf2; // chorus
    AudioSampleBuffer* dbf3; // reverb

    VexArpSettings fArpSet1, fArpSet2, fArpSet3;
    VexArp fArp1, fArp2, fArp3;

    VexChorus fChorus;
    VexDelay fDelay;
    VexReverb fReverb;
    VexSyntModule fSynth;

    ScopedPointer<VexEditorComponent> fView;
    ScopedPointer<JucePluginWindow> fWindow;

    MidiBuffer fMidiInBuffer;

    PluginClassEND(VexSynthPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VexSynthPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor vexSynthDesc = {
    /* category  */ PLUGIN_CATEGORY_SYNTH,
    /* hints     */ static_cast<PluginHints>(PLUGIN_HAS_GUI|PLUGIN_NEEDS_UI_JUCE|PLUGIN_USES_STATE|PLUGIN_USES_TIME),
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
