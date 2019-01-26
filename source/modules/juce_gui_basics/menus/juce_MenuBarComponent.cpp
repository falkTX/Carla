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

MenuBarComponent::MenuBarComponent (MenuBarModel* m)
    : model (nullptr),
      itemUnderMouse (-1),
      currentPopupIndex (-1),
      topLevelIndexClicked (0)
{
    setRepaintsOnMouseActivity (true);
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);

    setModel (m);
}

MenuBarComponent::~MenuBarComponent()
{
    setModel (nullptr);
    Desktop::getInstance().removeGlobalMouseListener (this);
}

MenuBarModel* MenuBarComponent::getModel() const noexcept
{
    return model;
}

void MenuBarComponent::setModel (MenuBarModel* const newModel)
{
    if (model != newModel)
    {
        if (model != nullptr)
            model->removeListener (this);

        model = newModel;

        if (model != nullptr)
            model->addListener (this);

        repaint();
        menuBarItemsChanged (nullptr);
    }
}

//==============================================================================
void MenuBarComponent::paint (Graphics& g)
{
    const bool isMouseOverBar = currentPopupIndex >= 0 || itemUnderMouse >= 0 || isMouseOver();

    getLookAndFeel().drawMenuBarBackground (g,
                                            getWidth(),
                                            getHeight(),
                                            isMouseOverBar,
                                            *this);

    if (model != nullptr)
    {
        for (int i = 0; i < menuNames.size(); ++i)
        {
            Graphics::ScopedSaveState ss (g);

            g.setOrigin (xPositions [i], 0);
            g.reduceClipRegion (0, 0, xPositions[i + 1] - xPositions[i], getHeight());

            getLookAndFeel().drawMenuBarItem (g,
                                              xPositions[i + 1] - xPositions[i],
                                              getHeight(),
                                              i,
                                              menuNames[i],
                                              i == itemUnderMouse,
                                              i == currentPopupIndex,
                                              isMouseOverBar,
                                              *this);
        }
    }
}

void MenuBarComponent::resized()
{
    xPositions.clear();
    int x = 0;
    xPositions.add (x);

    for (int i = 0; i < menuNames.size(); ++i)
    {
        x += getLookAndFeel().getMenuBarItemWidth (*this, i, menuNames[i]);
        xPositions.add (x);
    }
}

int MenuBarComponent::getItemAt (Point<int> p)
{
    for (int i = 0; i < xPositions.size(); ++i)
        if (p.x >= xPositions[i] && p.x < xPositions[i + 1])
            return reallyContains (p, true) ? i : -1;

    return -1;
}

void MenuBarComponent::repaintMenuItem (int index)
{
    if (isPositiveAndBelow (index, xPositions.size()))
    {
        const int x1 = xPositions [index];
        const int x2 = xPositions [index + 1];

        repaint (x1 - 2, 0, x2 - x1 + 4, getHeight());
    }
}

void MenuBarComponent::setItemUnderMouse (const int index)
{
    if (itemUnderMouse != index)
    {
        repaintMenuItem (itemUnderMouse);
        itemUnderMouse = index;
        repaintMenuItem (itemUnderMouse);
    }
}

void MenuBarComponent::setOpenItem (int index)
{
    if (currentPopupIndex != index)
    {
        if (currentPopupIndex < 0 && index >= 0)
            model->handleMenuBarActivate (true);
        else if (currentPopupIndex >= 0 && index < 0)
            model->handleMenuBarActivate (false);

        repaintMenuItem (currentPopupIndex);
        currentPopupIndex = index;
        repaintMenuItem (currentPopupIndex);

        Desktop& desktop = Desktop::getInstance();

        if (index >= 0)
            desktop.addGlobalMouseListener (this);
        else
            desktop.removeGlobalMouseListener (this);
    }
}

void MenuBarComponent::updateItemUnderMouse (Point<int> p)
{
    setItemUnderMouse (getItemAt (p));
}

void MenuBarComponent::showMenu (int index)
{
    if (index != currentPopupIndex)
    {
        PopupMenu::dismissAllActiveMenus();
        menuBarItemsChanged (nullptr);

        setOpenItem (index);
        setItemUnderMouse (index);

        if (index >= 0)
        {
            PopupMenu m (model->getMenuForIndex (itemUnderMouse,
                                                 menuNames [itemUnderMouse]));

            if (m.lookAndFeel == nullptr)
                m.setLookAndFeel (&getLookAndFeel());

            const Rectangle<int> itemPos (xPositions [index], 0, xPositions [index + 1] - xPositions [index], getHeight());

            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this)
                                                 .withTargetScreenArea (localAreaToGlobal (itemPos))
                                                 .withMinimumWidth (itemPos.getWidth()),
                             ModalCallbackFunction::forComponent (menuBarMenuDismissedCallback, this, index));
        }
    }
}

void MenuBarComponent::menuBarMenuDismissedCallback (int result, MenuBarComponent* bar, int topLevelIndex)
{
    if (bar != nullptr)
        bar->menuDismissed (topLevelIndex, result);
}

void MenuBarComponent::menuDismissed (int topLevelIndex, int itemId)
{
    topLevelIndexClicked = topLevelIndex;
    postCommandMessage (itemId);
}

void MenuBarComponent::handleCommandMessage (int commandId)
{
    const Point<int> mousePos (getMouseXYRelative());
    updateItemUnderMouse (mousePos);

    if (currentPopupIndex == topLevelIndexClicked)
        setOpenItem (-1);

    if (commandId != 0 && model != nullptr)
        model->menuItemSelected (commandId, topLevelIndexClicked);
}

//==============================================================================
void MenuBarComponent::mouseEnter (const MouseEvent& e)
{
    if (e.eventComponent == this)
        updateItemUnderMouse (e.getPosition());
}

void MenuBarComponent::mouseExit (const MouseEvent& e)
{
    if (e.eventComponent == this)
        updateItemUnderMouse (e.getPosition());
}

void MenuBarComponent::mouseDown (const MouseEvent& e)
{
    if (currentPopupIndex < 0)
    {
        const MouseEvent e2 (e.getEventRelativeTo (this));
        updateItemUnderMouse (e2.getPosition());

        currentPopupIndex = -2;
        showMenu (itemUnderMouse);
    }
}

void MenuBarComponent::mouseDrag (const MouseEvent& e)
{
    const MouseEvent e2 (e.getEventRelativeTo (this));
    const int item = getItemAt (e2.getPosition());

    if (item >= 0)
        showMenu (item);
}

void MenuBarComponent::mouseUp (const MouseEvent& e)
{
    const MouseEvent e2 (e.getEventRelativeTo (this));

    updateItemUnderMouse (e2.getPosition());

    if (itemUnderMouse < 0 && getLocalBounds().contains (e2.x, e2.y))
    {
        setOpenItem (-1);
        PopupMenu::dismissAllActiveMenus();
    }
}

void MenuBarComponent::mouseMove (const MouseEvent& e)
{
    const MouseEvent e2 (e.getEventRelativeTo (this));

    if (lastMousePos != e2.getPosition())
    {
        if (currentPopupIndex >= 0)
        {
            const int item = getItemAt (e2.getPosition());

            if (item >= 0)
                showMenu (item);
        }
        else
        {
            updateItemUnderMouse (e2.getPosition());
        }

        lastMousePos = e2.getPosition();
    }
}

bool MenuBarComponent::keyPressed (const KeyPress& key)
{
    const int numMenus = menuNames.size();

    if (numMenus > 0)
    {
        const int currentIndex = jlimit (0, numMenus - 1, currentPopupIndex);

        if (key.isKeyCode (KeyPress::leftKey))
        {
            showMenu ((currentIndex + numMenus - 1) % numMenus);
            return true;
        }

        if (key.isKeyCode (KeyPress::rightKey))
        {
            showMenu ((currentIndex + 1) % numMenus);
            return true;
        }
    }

    return false;
}

void MenuBarComponent::menuBarItemsChanged (MenuBarModel* /*menuBarModel*/)
{
    StringArray newNames;

    if (model != nullptr)
        newNames = model->getMenuBarNames();

    if (newNames != menuNames)
    {
        menuNames = newNames;
        repaint();
        resized();
    }
}

void MenuBarComponent::menuCommandInvoked (MenuBarModel* /*menuBarModel*/,
                                           const ApplicationCommandTarget::InvocationInfo& info)
{
    if (model == nullptr || (info.commandFlags & ApplicationCommandInfo::dontTriggerVisualFeedback) != 0)
        return;

    for (int i = 0; i < menuNames.size(); ++i)
    {
        const PopupMenu menu (model->getMenuForIndex (i, menuNames [i]));

        if (menu.containsCommandItem (info.commandID))
        {
            setItemUnderMouse (i);
            startTimer (200);
            break;
        }
    }
}

void MenuBarComponent::timerCallback()
{
    stopTimer();
    updateItemUnderMouse (getMouseXYRelative());
}

} // namespace juce
