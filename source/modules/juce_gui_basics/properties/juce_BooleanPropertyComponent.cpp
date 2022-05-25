/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

BooleanPropertyComponent::BooleanPropertyComponent (const String& name,
                                                    const String& buttonTextWhenTrue,
                                                    const String& buttonTextWhenFalse)
    : PropertyComponent (name),
      onText (buttonTextWhenTrue),
      offText (buttonTextWhenFalse)
{
    addAndMakeVisible (button);
    button.setClickingTogglesState (false);
    button.onClick = [this] { setState (! getState()); };
}

BooleanPropertyComponent::BooleanPropertyComponent (const Value& valueToControl,
                                                    const String& name,
                                                    const String& buttonText)
    : PropertyComponent (name),
      onText (buttonText),
      offText (buttonText)
{
    addAndMakeVisible (button);
    button.setClickingTogglesState (false);
    button.setButtonText (buttonText);
    button.getToggleStateValue().referTo (valueToControl);
    button.setClickingTogglesState (true);
}

BooleanPropertyComponent::~BooleanPropertyComponent()
{
}

void BooleanPropertyComponent::setState (const bool newState)
{
    button.setToggleState (newState, sendNotification);
}

bool BooleanPropertyComponent::getState() const
{
    return button.getToggleState();
}

void BooleanPropertyComponent::paint (Graphics& g)
{
    PropertyComponent::paint (g);

    g.setColour (findColour (backgroundColourId));
    g.fillRect (button.getBounds());

    g.setColour (findColour (outlineColourId));
    g.drawRect (button.getBounds());
}

void BooleanPropertyComponent::refresh()
{
    button.setToggleState (getState(), dontSendNotification);
    button.setButtonText (button.getToggleState() ? onText : offText);
}

} // namespace juce
