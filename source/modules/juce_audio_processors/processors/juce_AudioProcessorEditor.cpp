/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

AudioProcessorEditor::AudioProcessorEditor (AudioProcessor& p) noexcept  : processor (p)
{
    initialise();
}

AudioProcessorEditor::AudioProcessorEditor (AudioProcessor* p) noexcept  : processor (*p)
{
    // the filter must be valid..
    jassert (p != nullptr);
    initialise();
}

AudioProcessorEditor::~AudioProcessorEditor()
{
    // if this fails, then the wrapper hasn't called editorBeingDeleted() on the
    // filter for some reason..
    jassert (processor.getActiveEditor() != this);
    removeComponentListener (resizeListener);
}

void AudioProcessorEditor::setControlHighlight (ParameterControlHighlightInfo) {}
int AudioProcessorEditor::getControlParameterIndex (Component&)                { return -1; }

bool AudioProcessorEditor::supportsHostMIDIControllerPresence (bool)           { return true; }
void AudioProcessorEditor::hostMIDIControllerIsAvailable (bool)                {}

void AudioProcessorEditor::initialise()
{
    resizable = false;

    attachConstrainer (&defaultConstrainer);
    addComponentListener (resizeListener = new AudioProcessorEditorListener (*this));
}

//==============================================================================
void AudioProcessorEditor::setResizable (const bool shouldBeResizable, const bool useBottomRightCornerResizer)
{
    if (shouldBeResizable != resizable)
    {
        resizable = shouldBeResizable;

        if (! resizable)
        {
            setConstrainer (&defaultConstrainer);

            if (auto w = getWidth())
            {
                if (auto h = getHeight())
                {
                    defaultConstrainer.setSizeLimits (w, h, w, h);
                    resized();
                }
            }
        }
    }

    bool shouldHaveCornerResizer = (useBottomRightCornerResizer && shouldBeResizable);

    if (shouldHaveCornerResizer != (resizableCorner != nullptr))
    {
        if (shouldHaveCornerResizer)
        {
            Component::addChildComponent (resizableCorner = new ResizableCornerComponent (this, constrainer));
            resizableCorner->setAlwaysOnTop (true);
        }
        else
        {
            resizableCorner = nullptr;
        }
    }
}

void AudioProcessorEditor::setResizeLimits (int newMinimumWidth,
                                            int newMinimumHeight,
                                            int newMaximumWidth,
                                            int newMaximumHeight) noexcept
{
    // if you've set up a custom constrainer then these settings won't have any effect..
    jassert (constrainer == &defaultConstrainer || constrainer == nullptr);

    const bool shouldEnableResize      = (newMinimumWidth != newMaximumWidth || newMinimumHeight != newMaximumHeight);
    const bool shouldHaveCornerResizer = (shouldEnableResize != resizable    || resizableCorner != nullptr);

    setResizable (shouldEnableResize, shouldHaveCornerResizer);

    if (constrainer == nullptr)
        setConstrainer (&defaultConstrainer);

    defaultConstrainer.setSizeLimits (newMinimumWidth, newMinimumHeight,
                                      newMaximumWidth, newMaximumHeight);

    setBoundsConstrained (getBounds());
}

void AudioProcessorEditor::setConstrainer (ComponentBoundsConstrainer* newConstrainer)
{
    if (constrainer != newConstrainer)
    {
        resizable = true;
        attachConstrainer (newConstrainer);
    }
}

void AudioProcessorEditor::attachConstrainer (ComponentBoundsConstrainer* newConstrainer)
{
    if (constrainer != newConstrainer)
    {
        constrainer = newConstrainer;
        updatePeer();
    }
}

void AudioProcessorEditor::setBoundsConstrained (Rectangle<int> newBounds)
{
    if (constrainer != nullptr)
        constrainer->setBoundsForComponent (this, newBounds, false, false, false, false);
    else
        setBounds (newBounds);
}

void AudioProcessorEditor::editorResized (bool wasResized)
{
    if (wasResized)
    {
        bool resizerHidden = false;

        if (auto* peer = getPeer())
            resizerHidden = peer->isFullScreen() || peer->isKioskMode();

        if (resizableCorner != nullptr)
        {
            resizableCorner->setVisible (! resizerHidden);

            const int resizerSize = 18;
            resizableCorner->setBounds (getWidth() - resizerSize,
                                        getHeight() - resizerSize,
                                        resizerSize, resizerSize);
        }

        if (! resizable)
            if (auto w = getWidth())
                if (auto h = getHeight())
                    defaultConstrainer.setSizeLimits (w, h, w, h);
    }
}

void AudioProcessorEditor::updatePeer()
{
    if (isOnDesktop())
        if (auto* peer = getPeer())
            peer->setConstrainer (constrainer);
}

void AudioProcessorEditor::setScaleFactor (float newScale)
{
    setTransform (AffineTransform::scale (newScale));
    editorResized (true);
}

} // namespace juce
