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

PropertyComponent::PropertyComponent (const String& name, int height)
    : Component (name), preferredHeight (height)
{
    jassert (name.isNotEmpty());
}

PropertyComponent::~PropertyComponent() {}

void PropertyComponent::paint (Graphics& g)
{
    auto& lf = getLookAndFeel();

    lf.drawPropertyComponentBackground (g, getWidth(), getHeight(), *this);
    lf.drawPropertyComponentLabel      (g, getWidth(), getHeight(), *this);
}

void PropertyComponent::resized()
{
    if (auto c = getChildComponent(0))
        c->setBounds (getLookAndFeel().getPropertyComponentContentPosition (*this));
}

void PropertyComponent::enablementChanged()
{
    repaint();
}

} // namespace juce
