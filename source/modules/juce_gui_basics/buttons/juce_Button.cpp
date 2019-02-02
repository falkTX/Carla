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

class Button::CallbackHelper  : public Timer,
                                public ApplicationCommandManagerListener,
                                public Value::Listener,
                                public KeyListener
{
public:
    CallbackHelper (Button& b) : button (b)   {}

    void timerCallback() override
    {
        button.repeatTimerCallback();
    }

    bool keyStateChanged (bool, Component*) override
    {
        return button.keyStateChangedCallback();
    }

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (button.isOn))
            button.setToggleState (button.isOn.getValue(), sendNotification);
    }

    bool keyPressed (const KeyPress&, Component*) override
    {
        // returning true will avoid forwarding events for keys that we're using as shortcuts
        return button.isShortcutPressed();
    }

    void applicationCommandInvoked (const ApplicationCommandTarget::InvocationInfo& info) override
    {
        if (info.commandID == button.commandID
             && (info.commandFlags & ApplicationCommandInfo::dontTriggerVisualFeedback) == 0)
            button.flashButtonState();
    }

    void applicationCommandListChanged() override
    {
        button.applicationCommandListChangeCallback();
    }

private:
    Button& button;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CallbackHelper)
};

//==============================================================================
Button::Button (const String& name)
  : Component (name),
    text (name),
    buttonPressTime (0),
    lastRepeatTime (0),
    commandManagerToUse (nullptr),
    autoRepeatDelay (-1),
    autoRepeatSpeed (0),
    autoRepeatMinimumDelay (-1),
    radioGroupId (0),
    connectedEdgeFlags (0),
    commandID(),
    buttonState (buttonNormal),
    lastStatePainted (buttonNormal),
    lastToggleState (false),
    clickTogglesState (false),
    needsToRelease (false),
    needsRepainting (false),
    isKeyDown (false),
    triggerOnMouseDown (false),
    generateTooltip (false)
{
    callbackHelper = new CallbackHelper (*this);

    setWantsKeyboardFocus (true);
    isOn.addListener (callbackHelper);
}

Button::~Button()
{
    clearShortcuts();

    if (commandManagerToUse != nullptr)
        commandManagerToUse->removeListener (callbackHelper);

    isOn.removeListener (callbackHelper);
    callbackHelper = nullptr;
}

//==============================================================================
void Button::setButtonText (const String& newText)
{
    if (text != newText)
    {
        text = newText;
        repaint();
    }
}

void Button::setTooltip (const String& newTooltip)
{
    SettableTooltipClient::setTooltip (newTooltip);
    generateTooltip = false;
}

void Button::updateAutomaticTooltip (const ApplicationCommandInfo& info)
{
    if (generateTooltip && commandManagerToUse != nullptr)
    {
        String tt (info.description.isNotEmpty() ? info.description
                                                 : info.shortName);

        Array<KeyPress> keyPresses (commandManagerToUse->getKeyMappings()->getKeyPressesAssignedToCommand (commandID));

        for (int i = 0; i < keyPresses.size(); ++i)
        {
            const String key (keyPresses.getReference(i).getTextDescription());

            tt << " [";

            if (key.length() == 1)
                tt << TRANS("shortcut") << ": '" << key << "']";
            else
                tt << key << ']';
        }

        SettableTooltipClient::setTooltip (tt);
    }
}

void Button::setConnectedEdges (const int newFlags)
{
    if (connectedEdgeFlags != newFlags)
    {
        connectedEdgeFlags = newFlags;
        repaint();
    }
}

//==============================================================================
void Button::setToggleState (const bool shouldBeOn, const NotificationType notification)
{
    if (shouldBeOn != lastToggleState)
    {
        WeakReference<Component> deletionWatcher (this);

        if (shouldBeOn)
        {
            turnOffOtherButtonsInGroup (notification);

            if (deletionWatcher == nullptr)
                return;
        }

        // This test is done so that if the value is void rather than explicitly set to
        // false, the value won't be changed unless the required value is true.
        if (getToggleState() != shouldBeOn)
        {
            isOn = shouldBeOn;

            if (deletionWatcher == nullptr)
                return;
        }

        lastToggleState = shouldBeOn;
        repaint();

        if (notification != dontSendNotification)
        {
            // async callbacks aren't possible here
            jassert (notification != sendNotificationAsync);

            sendClickMessage (ModifierKeys::getCurrentModifiers());

            if (deletionWatcher == nullptr)
                return;
        }

        if (notification != dontSendNotification)
            sendStateMessage();
        else
            buttonStateChanged();
    }
}

void Button::setToggleState (const bool shouldBeOn, bool sendChange)
{
    setToggleState (shouldBeOn, sendChange ? sendNotification : dontSendNotification);
}

void Button::setClickingTogglesState (const bool shouldToggle) noexcept
{
    clickTogglesState = shouldToggle;

    // if you've got clickTogglesState turned on, you shouldn't also connect the button
    // up to be a command invoker. Instead, your command handler must flip the state of whatever
    // it is that this button represents, and the button will update its state to reflect this
    // in the applicationCommandListChanged() method.
    jassert (commandManagerToUse == nullptr || ! clickTogglesState);
}

bool Button::getClickingTogglesState() const noexcept
{
    return clickTogglesState;
}

void Button::setRadioGroupId (const int newGroupId, NotificationType notification)
{
    if (radioGroupId != newGroupId)
    {
        radioGroupId = newGroupId;

        if (lastToggleState)
            turnOffOtherButtonsInGroup (notification);
    }
}

void Button::turnOffOtherButtonsInGroup (const NotificationType notification)
{
    if (auto* p = getParentComponent())
    {
        if (radioGroupId != 0)
        {
            WeakReference<Component> deletionWatcher (this);

            for (auto* c : p->getChildren())
            {
                if (c != this)
                {
                    if (auto b = dynamic_cast<Button*> (c))
                    {
                        if (b->getRadioGroupId() == radioGroupId)
                        {
                            b->setToggleState (false, notification);

                            if (deletionWatcher == nullptr)
                                return;
                        }
                    }
                }
            }
        }
    }
}

//==============================================================================
void Button::enablementChanged()
{
    updateState();
    repaint();
}

Button::ButtonState Button::updateState()
{
    return updateState (isMouseOver (true), isMouseButtonDown());
}

Button::ButtonState Button::updateState (const bool over, const bool down)
{
    ButtonState newState = buttonNormal;

    if (isEnabled() && isVisible() && ! isCurrentlyBlockedByAnotherModalComponent())
    {
        if ((down && (over || (triggerOnMouseDown && buttonState == buttonDown))) || isKeyDown)
            newState = buttonDown;
        else if (over)
            newState = buttonOver;
    }

    setState (newState);
    return newState;
}

void Button::setState (const ButtonState newState)
{
    if (buttonState != newState)
    {
        buttonState = newState;
        repaint();

        if (buttonState == buttonDown)
        {
            buttonPressTime = Time::getApproximateMillisecondCounter();
            lastRepeatTime = 0;
        }

        sendStateMessage();
    }
}

bool Button::isDown() const noexcept    { return buttonState == buttonDown; }
bool Button::isOver() const noexcept    { return buttonState != buttonNormal; }

void Button::buttonStateChanged() {}

uint32 Button::getMillisecondsSinceButtonDown() const noexcept
{
    const uint32 now = Time::getApproximateMillisecondCounter();
    return now > buttonPressTime ? now - buttonPressTime : 0;
}

void Button::setTriggeredOnMouseDown (const bool isTriggeredOnMouseDown) noexcept
{
    triggerOnMouseDown = isTriggeredOnMouseDown;
}

bool Button::getTriggeredOnMouseDown() const noexcept
{
    return triggerOnMouseDown;
}

//==============================================================================
void Button::clicked()
{
}

void Button::clicked (const ModifierKeys&)
{
    clicked();
}

enum { clickMessageId = 0x2f3f4f99 };

void Button::triggerClick()
{
    postCommandMessage (clickMessageId);
}

void Button::internalClickCallback (const ModifierKeys& modifiers)
{
    if (clickTogglesState)
    {
        const bool shouldBeOn = (radioGroupId != 0 || ! lastToggleState);

        if (shouldBeOn != getToggleState())
        {
            setToggleState (shouldBeOn, sendNotification);
            return;
        }
    }

    sendClickMessage (modifiers);
}

void Button::flashButtonState()
{
    if (isEnabled())
    {
        needsToRelease = true;
        setState (buttonDown);
        callbackHelper->startTimer (100);
    }
}

void Button::handleCommandMessage (int commandId)
{
    if (commandId == clickMessageId)
    {
        if (isEnabled())
        {
            flashButtonState();
            internalClickCallback (ModifierKeys::getCurrentModifiers());
        }
    }
    else
    {
        Component::handleCommandMessage (commandId);
    }
}

//==============================================================================
void Button::addListener (Listener* l)      { buttonListeners.add (l); }
void Button::removeListener (Listener* l)   { buttonListeners.remove (l); }

void Button::sendClickMessage (const ModifierKeys& modifiers)
{
    Component::BailOutChecker checker (this);

    if (commandManagerToUse != nullptr && commandID != 0)
    {
        ApplicationCommandTarget::InvocationInfo info (commandID);
        info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromButton;
        info.originatingComponent = this;

        commandManagerToUse->invoke (info, true);
    }

    clicked (modifiers);

    if (! checker.shouldBailOut())
        buttonListeners.callChecked (checker, &Button::Listener::buttonClicked, this);
}

void Button::sendStateMessage()
{
    Component::BailOutChecker checker (this);

    buttonStateChanged();

    if (! checker.shouldBailOut())
        buttonListeners.callChecked (checker, &Button::Listener::buttonStateChanged, this);
}

//==============================================================================
void Button::paint (Graphics& g)
{
    if (needsToRelease && isEnabled())
    {
        needsToRelease = false;
        needsRepainting = true;
    }

    paintButton (g, isOver(), isDown());
    lastStatePainted = buttonState;
}

//==============================================================================
void Button::mouseEnter (const MouseEvent&)     { updateState (true,  false); }
void Button::mouseExit (const MouseEvent&)      { updateState (false, false); }

void Button::mouseDown (const MouseEvent& e)
{
    updateState (true, true);

    if (isDown())
    {
        if (autoRepeatDelay >= 0)
            callbackHelper->startTimer (autoRepeatDelay);

        if (triggerOnMouseDown)
            internalClickCallback (e.mods);
    }
}

void Button::mouseUp (const MouseEvent& e)
{
    const bool wasDown = isDown();
    const bool wasOver = isOver();
    updateState (isMouseOrTouchOver (e), false);

    if (wasDown && wasOver && ! triggerOnMouseDown)
    {
        if (lastStatePainted != buttonDown)
            flashButtonState();

        internalClickCallback (e.mods);
    }
}

void Button::mouseDrag (const MouseEvent& e)
{
    const ButtonState oldState = buttonState;
    updateState (isMouseOrTouchOver (e), true);

    if (autoRepeatDelay >= 0 && buttonState != oldState && isDown())
        callbackHelper->startTimer (autoRepeatSpeed);
}

bool Button::isMouseOrTouchOver (const MouseEvent& e)
{
    if (e.source.isTouch())
        return getLocalBounds().toFloat().contains (e.position);

    return isMouseOver();
}

void Button::focusGained (FocusChangeType)
{
    updateState();
    repaint();
}

void Button::focusLost (FocusChangeType)
{
    updateState();
    repaint();
}

void Button::visibilityChanged()
{
    needsToRelease = false;
    updateState();
}

void Button::parentHierarchyChanged()
{
    Component* const newKeySource = (shortcuts.size() == 0) ? nullptr : getTopLevelComponent();

    if (newKeySource != keySource.get())
    {
        if (keySource != nullptr)
            keySource->removeKeyListener (callbackHelper);

        keySource = newKeySource;

        if (keySource != nullptr)
            keySource->addKeyListener (callbackHelper);
    }
}

//==============================================================================
void Button::setCommandToTrigger (ApplicationCommandManager* const newCommandManager,
                                  const CommandID newCommandID, const bool generateTip)
{
    commandID = newCommandID;
    generateTooltip = generateTip;

    if (commandManagerToUse != newCommandManager)
    {
        if (commandManagerToUse != nullptr)
            commandManagerToUse->removeListener (callbackHelper);

        commandManagerToUse = newCommandManager;

        if (commandManagerToUse != nullptr)
            commandManagerToUse->addListener (callbackHelper);

        // if you've got clickTogglesState turned on, you shouldn't also connect the button
        // up to be a command invoker. Instead, your command handler must flip the state of whatever
        // it is that this button represents, and the button will update its state to reflect this
        // in the applicationCommandListChanged() method.
        jassert (commandManagerToUse == nullptr || ! clickTogglesState);
    }

    if (commandManagerToUse != nullptr)
        applicationCommandListChangeCallback();
    else
        setEnabled (true);
}

void Button::applicationCommandListChangeCallback()
{
    if (commandManagerToUse != nullptr)
    {
        ApplicationCommandInfo info (0);

        if (commandManagerToUse->getTargetForCommand (commandID, info) != nullptr)
        {
            updateAutomaticTooltip (info);
            setEnabled ((info.flags & ApplicationCommandInfo::isDisabled) == 0);
            setToggleState ((info.flags & ApplicationCommandInfo::isTicked) != 0, dontSendNotification);
        }
        else
        {
            setEnabled (false);
        }
    }
}

//==============================================================================
void Button::addShortcut (const KeyPress& key)
{
    if (key.isValid())
    {
        jassert (! isRegisteredForShortcut (key));  // already registered!

        shortcuts.add (key);
        parentHierarchyChanged();
    }
}

void Button::clearShortcuts()
{
    shortcuts.clear();
    parentHierarchyChanged();
}

bool Button::isShortcutPressed() const
{
    if (isShowing() && ! isCurrentlyBlockedByAnotherModalComponent())
        for (int i = shortcuts.size(); --i >= 0;)
            if (shortcuts.getReference(i).isCurrentlyDown())
                return true;

    return false;
}

bool Button::isRegisteredForShortcut (const KeyPress& key) const
{
    for (int i = shortcuts.size(); --i >= 0;)
        if (key == shortcuts.getReference(i))
            return true;

    return false;
}

bool Button::keyStateChangedCallback()
{
    if (! isEnabled())
        return false;

    const bool wasDown = isKeyDown;
    isKeyDown = isShortcutPressed();

    if (autoRepeatDelay >= 0 && (isKeyDown && ! wasDown))
        callbackHelper->startTimer (autoRepeatDelay);

    updateState();

    if (isEnabled() && wasDown && ! isKeyDown)
    {
        internalClickCallback (ModifierKeys::getCurrentModifiers());

        // (return immediately - this button may now have been deleted)
        return true;
    }

    return wasDown || isKeyDown;
}

bool Button::keyPressed (const KeyPress& key)
{
    if (isEnabled() && key.isKeyCode (KeyPress::returnKey))
    {
        triggerClick();
        return true;
    }

    return false;
}

//==============================================================================
void Button::setRepeatSpeed (const int initialDelayMillisecs,
                             const int repeatMillisecs,
                             const int minimumDelayInMillisecs) noexcept
{
    autoRepeatDelay = initialDelayMillisecs;
    autoRepeatSpeed = repeatMillisecs;
    autoRepeatMinimumDelay = jmin (autoRepeatSpeed, minimumDelayInMillisecs);
}

void Button::repeatTimerCallback()
{
    if (needsRepainting)
    {
        callbackHelper->stopTimer();
        updateState();
        needsRepainting = false;
    }
    else if (autoRepeatSpeed > 0 && (isKeyDown || (updateState() == buttonDown)))
    {
        int repeatSpeed = autoRepeatSpeed;

        if (autoRepeatMinimumDelay >= 0)
        {
            double timeHeldDown = jmin (1.0, getMillisecondsSinceButtonDown() / 4000.0);
            timeHeldDown *= timeHeldDown;

            repeatSpeed = repeatSpeed + (int) (timeHeldDown * (autoRepeatMinimumDelay - repeatSpeed));
        }

        repeatSpeed = jmax (1, repeatSpeed);

        const uint32 now = Time::getMillisecondCounter();

        // if we've been blocked from repeating often enough, speed up the repeat timer to compensate..
        if (lastRepeatTime != 0 && (int) (now - lastRepeatTime) > repeatSpeed * 2)
            repeatSpeed = jmax (1, repeatSpeed / 2);

        lastRepeatTime = now;
        callbackHelper->startTimer (repeatSpeed);

        internalClickCallback (ModifierKeys::getCurrentModifiers());
    }
    else if (! needsToRelease)
    {
        callbackHelper->stopTimer();
    }
}

} // namespace juce
