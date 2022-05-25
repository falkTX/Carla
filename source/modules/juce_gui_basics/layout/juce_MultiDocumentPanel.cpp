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

MultiDocumentPanelWindow::MultiDocumentPanelWindow (Colour backgroundColour)
    : DocumentWindow (String(), backgroundColour,
                      DocumentWindow::maximiseButton | DocumentWindow::closeButton, false)
{
}

MultiDocumentPanelWindow::~MultiDocumentPanelWindow()
{
}

//==============================================================================
void MultiDocumentPanelWindow::maximiseButtonPressed()
{
    if (auto* owner = getOwner())
        owner->setLayoutMode (MultiDocumentPanel::MaximisedWindowsWithTabs);
    else
        jassertfalse; // these windows are only designed to be used inside a MultiDocumentPanel!
}

void MultiDocumentPanelWindow::closeButtonPressed()
{
    if (auto* owner = getOwner())
        owner->closeDocumentAsync (getContentComponent(), true, nullptr);
    else
        jassertfalse; // these windows are only designed to be used inside a MultiDocumentPanel!
}

void MultiDocumentPanelWindow::activeWindowStatusChanged()
{
    DocumentWindow::activeWindowStatusChanged();
    updateOrder();
}

void MultiDocumentPanelWindow::broughtToFront()
{
    DocumentWindow::broughtToFront();
    updateOrder();
}

void MultiDocumentPanelWindow::updateOrder()
{
    if (auto* owner = getOwner())
        owner->updateOrder();
}

MultiDocumentPanel* MultiDocumentPanelWindow::getOwner() const noexcept
{
    return findParentComponentOfClass<MultiDocumentPanel>();
}

//==============================================================================
struct MultiDocumentPanel::TabbedComponentInternal   : public TabbedComponent
{
    TabbedComponentInternal() : TabbedComponent (TabbedButtonBar::TabsAtTop) {}

    void currentTabChanged (int, const String&) override
    {
        if (auto* owner = findParentComponentOfClass<MultiDocumentPanel>())
            owner->updateOrder();
    }
};


//==============================================================================
MultiDocumentPanel::MultiDocumentPanel()
{
    setOpaque (true);
}

MultiDocumentPanel::~MultiDocumentPanel()
{
    for (int i = components.size(); --i >= 0;)
        if (auto* component = components[i])
            closeDocumentInternal (component);
}

//==============================================================================
namespace MultiDocHelpers
{
    static bool shouldDeleteComp (Component* const c)
    {
        return c->getProperties() ["mdiDocumentDelete_"];
    }
}

#if JUCE_MODAL_LOOPS_PERMITTED
bool MultiDocumentPanel::closeAllDocuments (const bool checkItsOkToCloseFirst)
{
    while (! components.isEmpty())
        if (! closeDocument (components.getLast(), checkItsOkToCloseFirst))
            return false;

    return true;
}
#endif

void MultiDocumentPanel::closeLastDocumentRecursive (SafePointer<MultiDocumentPanel> parent,
                                                     bool checkItsOkToCloseFirst,
                                                     std::function<void (bool)> callback)
{
    if (parent->components.isEmpty())
    {
        if (callback != nullptr)
            callback (true);

        return;
    }

    parent->closeDocumentAsync (parent->components.getLast(),
                                checkItsOkToCloseFirst,
                                [parent, checkItsOkToCloseFirst, callback] (bool closeResult)
    {
        if (parent == nullptr)
            return;

        if (! closeResult)
        {
            if (callback != nullptr)
                callback (false);

            return;
        }

        parent->closeLastDocumentRecursive (parent, checkItsOkToCloseFirst, std::move (callback));
    });
}

void MultiDocumentPanel::closeAllDocumentsAsync (bool checkItsOkToCloseFirst, std::function<void (bool)> callback)
{
    closeLastDocumentRecursive (this, checkItsOkToCloseFirst, std::move (callback));
}

#if JUCE_MODAL_LOOPS_PERMITTED
bool MultiDocumentPanel::tryToCloseDocument (Component*)
{
    // If you hit this assertion then you need to implement this method in a subclass.
    jassertfalse;
    return false;
}
#endif

MultiDocumentPanelWindow* MultiDocumentPanel::createNewDocumentWindow()
{
    return new MultiDocumentPanelWindow (backgroundColour);
}

void MultiDocumentPanel::addWindow (Component* component)
{
    auto* dw = createNewDocumentWindow();

    dw->setResizable (true, false);
    dw->setContentNonOwned (component, true);
    dw->setName (component->getName());

    auto bkg = component->getProperties() ["mdiDocumentBkg_"];
    dw->setBackgroundColour (bkg.isVoid() ? backgroundColour : Colour ((uint32) static_cast<int> (bkg)));

    int x = 4;

    if (auto* topComp = getChildren().getLast())
        if (topComp->getX() == x && topComp->getY() == x)
            x += 16;

    dw->setTopLeftPosition (x, x);

    auto pos = component->getProperties() ["mdiDocumentPos_"];
    if (pos.toString().isNotEmpty())
        dw->restoreWindowStateFromString (pos.toString());

    addAndMakeVisible (dw);
    dw->toFront (true);
}

bool MultiDocumentPanel::addDocument (Component* const component,
                                      Colour docColour,
                                      const bool deleteWhenRemoved)
{
    // If you try passing a full DocumentWindow or ResizableWindow in here, you'll end up
    // with a frame-within-a-frame! Just pass in the bare content component.
    jassert (dynamic_cast<ResizableWindow*> (component) == nullptr);

    if (component == nullptr || (maximumNumDocuments > 0 && components.size() >= maximumNumDocuments))
        return false;

    components.add (component);
    component->getProperties().set ("mdiDocumentDelete_", deleteWhenRemoved);
    component->getProperties().set ("mdiDocumentBkg_", (int) docColour.getARGB());
    component->addComponentListener (this);

    if (mode == FloatingWindows)
    {
        if (isFullscreenWhenOneDocument())
        {
            if (components.size() == 1)
            {
                addAndMakeVisible (component);
            }
            else
            {
                if (components.size() == 2)
                    addWindow (components.getFirst());

                addWindow (component);
            }
        }
        else
        {
           addWindow (component);
        }
    }
    else
    {
        if (tabComponent == nullptr && components.size() > numDocsBeforeTabsUsed)
        {
            tabComponent.reset (new TabbedComponentInternal());
            addAndMakeVisible (tabComponent.get());

            auto temp = components;

            for (auto& c : temp)
                tabComponent->addTab (c->getName(), docColour, c, false);

            resized();
        }
        else
        {
            if (tabComponent != nullptr)
                tabComponent->addTab (component->getName(), docColour, component, false);
            else
                addAndMakeVisible (component);
        }

        setActiveDocument (component);
    }

    resized();
    activeDocumentChanged();
    return true;
}

void MultiDocumentPanel::closeDocumentInternal (Component* component)
{
    // Intellisense warns about component being uninitialised.
    // I'm not sure how a function argument could be uninitialised.
    JUCE_BEGIN_IGNORE_WARNINGS_MSVC (6001)

    component->removeComponentListener (this);

    const bool shouldDelete = MultiDocHelpers::shouldDeleteComp (component);
    component->getProperties().remove ("mdiDocumentDelete_");
    component->getProperties().remove ("mdiDocumentBkg_");

    if (mode == FloatingWindows)
    {
        for (auto* child : getChildren())
        {
            if (auto* dw = dynamic_cast<MultiDocumentPanelWindow*> (child))
            {
                if (dw->getContentComponent() == component)
                {
                    std::unique_ptr<MultiDocumentPanelWindow> (dw)->clearContentComponent();
                    break;
                }
            }
        }

        if (shouldDelete)
            delete component;

        components.removeFirstMatchingValue (component);

        if (isFullscreenWhenOneDocument() && components.size() == 1)
        {
            for (int i = getNumChildComponents(); --i >= 0;)
            {
                std::unique_ptr<MultiDocumentPanelWindow> dw (dynamic_cast<MultiDocumentPanelWindow*> (getChildComponent (i)));

                if (dw != nullptr)
                    dw->clearContentComponent();
            }

            addAndMakeVisible (components.getFirst());
        }
    }
    else
    {
        jassert (components.indexOf (component) >= 0);

        if (tabComponent != nullptr)
        {
            for (int i = tabComponent->getNumTabs(); --i >= 0;)
                if (tabComponent->getTabContentComponent (i) == component)
                    tabComponent->removeTab (i);
        }
        else
        {
            removeChildComponent (component);
        }

        if (shouldDelete)
            delete component;

        if (tabComponent != nullptr && tabComponent->getNumTabs() <= numDocsBeforeTabsUsed)
            tabComponent.reset();

        components.removeFirstMatchingValue (component);

        if (components.size() > 0 && tabComponent == nullptr)
            addAndMakeVisible (components.getFirst());
    }

    resized();

    // This ensures that the active tab is painted properly when a tab is closed!
    if (auto* activeComponent = getActiveDocument())
        setActiveDocument (activeComponent);

    activeDocumentChanged();

    JUCE_END_IGNORE_WARNINGS_MSVC
}

#if JUCE_MODAL_LOOPS_PERMITTED
bool MultiDocumentPanel::closeDocument (Component* component,
                                        const bool checkItsOkToCloseFirst)
{
    // Intellisense warns about component being uninitialised.
    // I'm not sure how a function argument could be uninitialised.
    JUCE_BEGIN_IGNORE_WARNINGS_MSVC (6001)

    if (component == nullptr)
        return true;

    if (components.contains (component))
    {
        if (checkItsOkToCloseFirst && ! tryToCloseDocument (component))
            return false;

        closeDocumentInternal (component);
    }
    else
    {
        jassertfalse;
    }

    return true;

    JUCE_END_IGNORE_WARNINGS_MSVC
}
#endif

void MultiDocumentPanel::closeDocumentAsync (Component* component,
                                             const bool checkItsOkToCloseFirst,
                                             std::function<void (bool)> callback)
{
    // Intellisense warns about component being uninitialised.
    // I'm not sure how a function argument could be uninitialised.
    JUCE_BEGIN_IGNORE_WARNINGS_MSVC (6001)

    if (component == nullptr)
    {
        if (callback != nullptr)
            callback (true);

        return;
    }

    if (components.contains (component))
    {
        if (checkItsOkToCloseFirst)
        {
            tryToCloseDocumentAsync (component,
                                     [parent = SafePointer<MultiDocumentPanel> { this }, component, callback] (bool closedSuccessfully)
            {
                if (parent == nullptr)
                    return;

                if (closedSuccessfully)
                    parent->closeDocumentInternal (component);

                if (callback != nullptr)
                    callback (closedSuccessfully);
            });

            return;
        }

        closeDocumentInternal (component);
    }
    else
    {
        jassertfalse;
    }

    if (callback != nullptr)
        callback (true);

    JUCE_END_IGNORE_WARNINGS_MSVC
}

int MultiDocumentPanel::getNumDocuments() const noexcept
{
    return components.size();
}

Component* MultiDocumentPanel::getDocument (const int index) const noexcept
{
    return components [index];
}

Component* MultiDocumentPanel::getActiveDocument() const noexcept
{
    if (mode == FloatingWindows)
    {
        for (auto* child : getChildren())
            if (auto* dw = dynamic_cast<MultiDocumentPanelWindow*> (child))
                if (dw->isActiveWindow())
                    return dw->getContentComponent();
    }

    return components.getLast();
}

void MultiDocumentPanel::setActiveDocument (Component* component)
{
    jassert (component != nullptr);

    if (mode == FloatingWindows)
    {
        component = getContainerComp (component);

        if (component != nullptr)
            component->toFront (true);
    }
    else if (tabComponent != nullptr)
    {
        jassert (components.indexOf (component) >= 0);

        for (int i = tabComponent->getNumTabs(); --i >= 0;)
        {
            if (tabComponent->getTabContentComponent (i) == component)
            {
                tabComponent->setCurrentTabIndex (i);
                break;
            }
        }
    }
    else
    {
        component->grabKeyboardFocus();
    }
}

void MultiDocumentPanel::activeDocumentChanged()
{
}

void MultiDocumentPanel::setMaximumNumDocuments (const int newNumber)
{
    maximumNumDocuments = newNumber;
}

void MultiDocumentPanel::useFullscreenWhenOneDocument (const bool shouldUseTabs)
{
    numDocsBeforeTabsUsed = shouldUseTabs ? 1 : 0;
}

bool MultiDocumentPanel::isFullscreenWhenOneDocument() const noexcept
{
    return numDocsBeforeTabsUsed != 0;
}

//==============================================================================
void MultiDocumentPanel::setLayoutMode (const LayoutMode newLayoutMode)
{
    if (mode != newLayoutMode)
    {
        mode = newLayoutMode;

        if (mode == FloatingWindows)
        {
            tabComponent.reset();
        }
        else
        {
            for (int i = getNumChildComponents(); --i >= 0;)
            {
                std::unique_ptr<MultiDocumentPanelWindow> dw (dynamic_cast<MultiDocumentPanelWindow*> (getChildComponent (i)));

                if (dw != nullptr)
                {
                    dw->getContentComponent()->getProperties().set ("mdiDocumentPos_", dw->getWindowStateAsString());
                    dw->clearContentComponent();
                }
            }
        }

        resized();

        auto tempComps = components;
        components.clear();

        for (auto* c : tempComps)
            addDocument (c,
                         Colour ((uint32) static_cast<int> (c->getProperties().getWithDefault ("mdiDocumentBkg_", (int) Colours::white.getARGB()))),
                         MultiDocHelpers::shouldDeleteComp (c));
    }
}

void MultiDocumentPanel::setBackgroundColour (Colour newBackgroundColour)
{
    if (backgroundColour != newBackgroundColour)
    {
        backgroundColour = newBackgroundColour;
        setOpaque (newBackgroundColour.isOpaque());
        repaint();
    }
}

//==============================================================================
void MultiDocumentPanel::paint (Graphics& g)
{
    g.fillAll (backgroundColour);
}

void MultiDocumentPanel::resized()
{
    if (mode == MaximisedWindowsWithTabs || components.size() == numDocsBeforeTabsUsed)
    {
        for (auto* child : getChildren())
            child->setBounds (getLocalBounds());
    }

    setWantsKeyboardFocus (components.size() == 0);
}

Component* MultiDocumentPanel::getContainerComp (Component* c) const
{
    if (mode == FloatingWindows)
    {
        for (auto* child : getChildren())
            if (auto* dw = dynamic_cast<MultiDocumentPanelWindow*> (child))
                if (dw->getContentComponent() == c)
                    return dw;
    }

    return c;
}

void MultiDocumentPanel::componentNameChanged (Component&)
{
    if (mode == FloatingWindows)
    {
        for (auto* child : getChildren())
            if (auto* dw = dynamic_cast<MultiDocumentPanelWindow*> (child))
                dw->setName (dw->getContentComponent()->getName());
    }
    else if (tabComponent != nullptr)
    {
        for (int i = tabComponent->getNumTabs(); --i >= 0;)
            tabComponent->setTabName (i, tabComponent->getTabContentComponent (i)->getName());
    }
}

void MultiDocumentPanel::updateOrder()
{
    auto oldList = components;

    if (mode == FloatingWindows)
    {
        components.clear();

        for (auto* child : getChildren())
            if (auto* dw = dynamic_cast<MultiDocumentPanelWindow*> (child))
                components.add (dw->getContentComponent());
    }
    else
    {
        if (tabComponent != nullptr)
        {
            if (auto* current = tabComponent->getCurrentContentComponent())
            {
                components.removeFirstMatchingValue (current);
                components.add (current);
            }
        }
    }

    if (components != oldList)
        activeDocumentChanged();
}

} // namespace juce
