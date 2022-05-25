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

bool juce_performDragDropFiles (const StringArray&, const bool copyFiles, bool& shouldStop);
bool juce_performDragDropText (const String&, bool& shouldStop);


//==============================================================================
class DragAndDropContainer::DragImageComponent  : public Component,
                                                  private Timer
{
public:
    DragImageComponent (const ScaledImage& im,
                        const var& desc,
                        Component* const sourceComponent,
                        const MouseInputSource* draggingSource,
                        DragAndDropContainer& ddc,
                        Point<int> offset)
        : sourceDetails (desc, sourceComponent, Point<int>()),
          image (im),
          owner (ddc),
          mouseDragSource (draggingSource->getComponentUnderMouse()),
          imageOffset (transformOffsetCoordinates (sourceComponent, offset)),
          originalInputSourceIndex (draggingSource->getIndex()),
          originalInputSourceType (draggingSource->getType())
    {
        updateSize();

        if (mouseDragSource == nullptr)
            mouseDragSource = sourceComponent;

        mouseDragSource->addMouseListener (this, false);

        startTimer (200);

        setInterceptsMouseClicks (false, false);
        setAlwaysOnTop (true);
    }

    ~DragImageComponent() override
    {
        owner.dragImageComponents.remove (owner.dragImageComponents.indexOf (this), false);

        if (mouseDragSource != nullptr)
        {
            mouseDragSource->removeMouseListener (this);

            if (auto* current = getCurrentlyOver())
                if (current->isInterestedInDragSource (sourceDetails))
                    current->itemDragExit (sourceDetails);
        }

        owner.dragOperationEnded (sourceDetails);
    }

    void paint (Graphics& g) override
    {
        if (isOpaque())
            g.fillAll (Colours::white);

        g.setOpacity (1.0f);
        g.drawImage (image.getImage(), getLocalBounds().toFloat());
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (e.originalComponent != this && isOriginalInputSource (e.source))
        {
            if (mouseDragSource != nullptr)
                mouseDragSource->removeMouseListener (this);

            // (note: use a local copy of this in case the callback runs
            // a modal loop and deletes this object before the method completes)
            auto details = sourceDetails;
            DragAndDropTarget* finalTarget = nullptr;

            auto wasVisible = isVisible();
            setVisible (false);
            Component* unused;
            finalTarget = findTarget (e.getScreenPosition(), details.localPosition, unused);

            if (wasVisible) // fade the component and remove it - it'll be deleted later by the timer callback
                dismissWithAnimation (finalTarget == nullptr);

            if (auto* parent = getParentComponent())
                parent->removeChildComponent (this);

            if (finalTarget != nullptr)
            {
                currentlyOverComp = nullptr;
                finalTarget->itemDropped (details);
            }

            // careful - this object could now be deleted..
        }
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (e.originalComponent != this && isOriginalInputSource (e.source))
            updateLocation (true, e.getScreenPosition());
    }

    void updateLocation (const bool canDoExternalDrag, Point<int> screenPos)
    {
        auto details = sourceDetails;

        setNewScreenPos (screenPos);

        Component* newTargetComp;
        auto* newTarget = findTarget (screenPos, details.localPosition, newTargetComp);

        setVisible (newTarget == nullptr || newTarget->shouldDrawDragImageWhenOver());

        if (newTargetComp != currentlyOverComp)
        {
            if (auto* lastTarget = getCurrentlyOver())
                if (details.sourceComponent != nullptr && lastTarget->isInterestedInDragSource (details))
                    lastTarget->itemDragExit (details);

            currentlyOverComp = newTargetComp;

            if (newTarget != nullptr
                  && newTarget->isInterestedInDragSource (details))
                newTarget->itemDragEnter (details);
        }

        sendDragMove (details);

        if (canDoExternalDrag)
        {
            auto now = Time::getCurrentTime();

            if (getCurrentlyOver() != nullptr)
                lastTimeOverTarget = now;
            else if (now > lastTimeOverTarget + RelativeTime::milliseconds (700))
                checkForExternalDrag (details, screenPos);
        }

        forceMouseCursorUpdate();
    }

    void updateImage (const ScaledImage& newImage)
    {
        image = newImage;
        updateSize();
        repaint();
    }

    void timerCallback() override
    {
        forceMouseCursorUpdate();

        if (sourceDetails.sourceComponent == nullptr)
        {
            deleteSelf();
        }
        else
        {
            for (auto& s : Desktop::getInstance().getMouseSources())
            {
                if (isOriginalInputSource (s) && ! s.isDragging())
                {
                    if (mouseDragSource != nullptr)
                        mouseDragSource->removeMouseListener (this);

                    deleteSelf();
                    break;
                }
            }
        }
    }

    bool keyPressed (const KeyPress& key) override
    {
        if (key == KeyPress::escapeKey)
        {
            dismissWithAnimation (true);
            deleteSelf();
            return true;
        }

        return false;
    }

    bool canModalEventBeSentToComponent (const Component* targetComponent) override
    {
        return targetComponent == mouseDragSource;
    }

    // (overridden to avoid beeps when dragging)
    void inputAttemptWhenModal() override {}

    DragAndDropTarget::SourceDetails sourceDetails;

private:
    ScaledImage image;
    DragAndDropContainer& owner;
    WeakReference<Component> mouseDragSource, currentlyOverComp;
    const Point<int> imageOffset;
    bool hasCheckedForExternalDrag = false;
    Time lastTimeOverTarget;
    int originalInputSourceIndex;
    MouseInputSource::InputSourceType originalInputSourceType;

    void updateSize()
    {
        const auto bounds = image.getScaledBounds().toNearestInt();
        setSize (bounds.getWidth(), bounds.getHeight());
    }

    void forceMouseCursorUpdate()
    {
        Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
    }

    DragAndDropTarget* getCurrentlyOver() const noexcept
    {
        return dynamic_cast<DragAndDropTarget*> (currentlyOverComp.get());
    }

    static Component* findDesktopComponentBelow (Point<int> screenPos)
    {
        auto& desktop = Desktop::getInstance();

        for (auto i = desktop.getNumComponents(); --i >= 0;)
        {
            auto* desktopComponent = desktop.getComponent (i);
            auto dPoint = desktopComponent->getLocalPoint (nullptr, screenPos);

            if (auto* c = desktopComponent->getComponentAt (dPoint))
            {
                auto cPoint = c->getLocalPoint (desktopComponent, dPoint);

                if (c->hitTest (cPoint.getX(), cPoint.getY()))
                    return c;
            }
        }

        return nullptr;
    }

    Point<int> transformOffsetCoordinates (const Component* const sourceComponent, Point<int> offsetInSource) const
    {
        return getLocalPoint (sourceComponent, offsetInSource) - getLocalPoint (sourceComponent, Point<int>());
    }

    DragAndDropTarget* findTarget (Point<int> screenPos, Point<int>& relativePos,
                                   Component*& resultComponent) const
    {
        auto* hit = getParentComponent();

        if (hit == nullptr)
            hit = findDesktopComponentBelow (screenPos);
        else
            hit = hit->getComponentAt (hit->getLocalPoint (nullptr, screenPos));

        // (note: use a local copy of this in case the callback runs
        // a modal loop and deletes this object before the method completes)
        auto details = sourceDetails;

        while (hit != nullptr)
        {
            if (auto* ddt = dynamic_cast<DragAndDropTarget*> (hit))
            {
                if (ddt->isInterestedInDragSource (details))
                {
                    relativePos = hit->getLocalPoint (nullptr, screenPos);
                    resultComponent = hit;
                    return ddt;
                }
            }

            hit = hit->getParentComponent();
        }

        resultComponent = nullptr;
        return nullptr;
    }

    void setNewScreenPos (Point<int> screenPos)
    {
        auto newPos = screenPos - imageOffset;

        if (auto* p = getParentComponent())
            newPos = p->getLocalPoint (nullptr, newPos);

        setTopLeftPosition (newPos);
    }

    void sendDragMove (DragAndDropTarget::SourceDetails& details) const
    {
        if (auto* target = getCurrentlyOver())
            if (target->isInterestedInDragSource (details))
                target->itemDragMove (details);
    }

    void checkForExternalDrag (DragAndDropTarget::SourceDetails& details, Point<int> screenPos)
    {
        if (! hasCheckedForExternalDrag)
        {
            if (Desktop::getInstance().findComponentAt (screenPos) == nullptr)
            {
                hasCheckedForExternalDrag = true;

                if (ComponentPeer::getCurrentModifiersRealtime().isAnyMouseButtonDown())
                {
                    StringArray files;
                    auto canMoveFiles = false;

                    if (owner.shouldDropFilesWhenDraggedExternally (details, files, canMoveFiles) && ! files.isEmpty())
                    {
                        MessageManager::callAsync ([=] { DragAndDropContainer::performExternalDragDropOfFiles (files, canMoveFiles); });
                        deleteSelf();
                        return;
                    }

                    String text;

                    if (owner.shouldDropTextWhenDraggedExternally (details, text) && text.isNotEmpty())
                    {
                        MessageManager::callAsync ([=] { DragAndDropContainer::performExternalDragDropOfText (text); });
                        deleteSelf();
                        return;
                    }
                }
            }
        }
    }

    void deleteSelf()
    {
        delete this;
    }

    void dismissWithAnimation (const bool shouldSnapBack)
    {
        setVisible (true);
        auto& animator = Desktop::getInstance().getAnimator();

        if (shouldSnapBack && sourceDetails.sourceComponent != nullptr)
        {
            auto target = sourceDetails.sourceComponent->localPointToGlobal (sourceDetails.sourceComponent->getLocalBounds().getCentre());
            auto ourCentre = localPointToGlobal (getLocalBounds().getCentre());

            animator.animateComponent (this,
                                       getBounds() + (target - ourCentre),
                                       0.0f, 120,
                                       true, 1.0, 1.0);
        }
        else
        {
            animator.fadeOut (this, 120);
        }
    }

    bool isOriginalInputSource (const MouseInputSource& sourceToCheck)
    {
        return (sourceToCheck.getType() == originalInputSourceType
                && sourceToCheck.getIndex() == originalInputSourceIndex);
    }

    JUCE_DECLARE_NON_COPYABLE (DragImageComponent)
};


//==============================================================================
DragAndDropContainer::DragAndDropContainer() = default;

DragAndDropContainer::~DragAndDropContainer() = default;

void DragAndDropContainer::startDragging (const var& sourceDescription,
                                          Component* sourceComponent,
                                          const ScaledImage& dragImage,
                                          const bool allowDraggingToExternalWindows,
                                          const Point<int>* imageOffsetFromMouse,
                                          const MouseInputSource* inputSourceCausingDrag)
{
    if (isAlreadyDragging (sourceComponent))
        return;

    auto* draggingSource = getMouseInputSourceForDrag (sourceComponent, inputSourceCausingDrag);

    if (draggingSource == nullptr || ! draggingSource->isDragging())
    {
        jassertfalse;   // You must call startDragging() from within a mouseDown or mouseDrag callback!
        return;
    }

    const auto lastMouseDown = draggingSource->getLastMouseDownPosition().roundToInt();

    struct ImageAndOffset
    {
        ScaledImage image;
        Point<double> offset;
    };

    const auto imageToUse = [&]() -> ImageAndOffset
    {
        if (! dragImage.getImage().isNull())
            return { dragImage, imageOffsetFromMouse != nullptr ? dragImage.getScaledBounds().getConstrainedPoint (-imageOffsetFromMouse->toDouble())
                                                                : dragImage.getScaledBounds().getCentre() };

        const auto scaleFactor = 2.0;
        auto image = sourceComponent->createComponentSnapshot (sourceComponent->getLocalBounds(), true, (float) scaleFactor)
                                    .convertedToFormat (Image::ARGB);
        image.multiplyAllAlphas (0.6f);

        const auto relPos = sourceComponent->getLocalPoint (nullptr, lastMouseDown).toDouble();
        const auto clipped = (image.getBounds().toDouble() / scaleFactor).getConstrainedPoint (relPos);

        Image fade (Image::SingleChannel, image.getWidth(), image.getHeight(), true);
        Graphics fadeContext (fade);

        ColourGradient gradient;
        gradient.isRadial = true;
        gradient.point1 = clipped.toFloat() * scaleFactor;
        gradient.point2 = gradient.point1 + Point<float> (0.0f, scaleFactor * 400.0f);
        gradient.addColour (0.0, Colours::white);
        gradient.addColour (0.375, Colours::white);
        gradient.addColour (1.0, Colours::transparentWhite);

        fadeContext.setGradientFill (gradient);
        fadeContext.fillAll();

        Image composite (Image::ARGB, image.getWidth(), image.getHeight(), true);
        Graphics compositeContext (composite);

        compositeContext.reduceClipRegion (fade, {});
        compositeContext.drawImageAt (image, 0, 0);

        return { ScaledImage (composite, scaleFactor), clipped };
    }();

    auto* dragImageComponent = dragImageComponents.add (new DragImageComponent (imageToUse.image, sourceDescription, sourceComponent,
                                                                                draggingSource, *this, imageToUse.offset.roundToInt()));

    if (allowDraggingToExternalWindows)
    {
        if (! Desktop::canUseSemiTransparentWindows())
            dragImageComponent->setOpaque (true);

        dragImageComponent->addToDesktop (ComponentPeer::windowIgnoresMouseClicks
                                          | ComponentPeer::windowIsTemporary
                                          | ComponentPeer::windowIgnoresKeyPresses);
    }
    else
    {
        if (auto* thisComp = dynamic_cast<Component*> (this))
        {
            thisComp->addChildComponent (dragImageComponent);
        }
        else
        {
            jassertfalse;   // Your DragAndDropContainer needs to be a Component!
            return;
        }
    }

    dragImageComponent->sourceDetails.localPosition = sourceComponent->getLocalPoint (nullptr, lastMouseDown);
    dragImageComponent->updateLocation (false, lastMouseDown);

   #if JUCE_WINDOWS
    // Under heavy load, the layered window's paint callback can often be lost by the OS,
    // so forcing a repaint at least once makes sure that the window becomes visible..
    if (auto* peer = dragImageComponent->getPeer())
        peer->performAnyPendingRepaintsNow();
   #endif

    dragOperationStarted (dragImageComponent->sourceDetails);
}

bool DragAndDropContainer::isDragAndDropActive() const
{
    return dragImageComponents.size() > 0;
}

int DragAndDropContainer::getNumCurrentDrags() const
{
    return dragImageComponents.size();
}

var DragAndDropContainer::getCurrentDragDescription() const
{
    // If you are performing drag and drop in a multi-touch environment then
    // you should use the getDragDescriptionForIndex() method instead!
    jassert (dragImageComponents.size() < 2);

    return dragImageComponents.size() != 0 ? dragImageComponents[0]->sourceDetails.description
                                           : var();
}

var DragAndDropContainer::getDragDescriptionForIndex (int index) const
{
    if (! isPositiveAndBelow (index, dragImageComponents.size()))
        return {};

    return dragImageComponents.getUnchecked (index)->sourceDetails.description;
}

void DragAndDropContainer::setCurrentDragImage (const ScaledImage& newImage)
{
    // If you are performing drag and drop in a multi-touch environment then
    // you should use the setDragImageForIndex() method instead!
    jassert (dragImageComponents.size() < 2);

    dragImageComponents[0]->updateImage (newImage);
}

void DragAndDropContainer::setDragImageForIndex (int index, const ScaledImage& newImage)
{
    if (isPositiveAndBelow (index, dragImageComponents.size()))
        dragImageComponents.getUnchecked (index)->updateImage (newImage);
}

DragAndDropContainer* DragAndDropContainer::findParentDragContainerFor (Component* c)
{
    return c != nullptr ? c->findParentComponentOfClass<DragAndDropContainer>() : nullptr;
}

bool DragAndDropContainer::shouldDropFilesWhenDraggedExternally (const DragAndDropTarget::SourceDetails&, StringArray&, bool&)
{
    return false;
}

bool DragAndDropContainer::shouldDropTextWhenDraggedExternally (const DragAndDropTarget::SourceDetails&, String&)
{
    return false;
}

void DragAndDropContainer::dragOperationStarted (const DragAndDropTarget::SourceDetails&)  {}
void DragAndDropContainer::dragOperationEnded (const DragAndDropTarget::SourceDetails&)    {}

const MouseInputSource* DragAndDropContainer::getMouseInputSourceForDrag (Component* sourceComponent,
                                                                          const MouseInputSource* inputSourceCausingDrag)
{
    if (inputSourceCausingDrag == nullptr)
    {
        auto minDistance = std::numeric_limits<float>::max();
        auto& desktop = Desktop::getInstance();

        auto centrePoint = sourceComponent ? sourceComponent->getScreenBounds().getCentre().toFloat() : Point<float>();
        auto numDragging = desktop.getNumDraggingMouseSources();

        for (auto i = 0; i < numDragging; ++i)
        {
            if (auto* ms = desktop.getDraggingMouseSource (i))
            {
                auto distance =  ms->getScreenPosition().getDistanceSquaredFrom (centrePoint);

                if (distance < minDistance)
                {
                    minDistance = distance;
                    inputSourceCausingDrag = ms;
                }
            }
        }
    }

    // You must call startDragging() from within a mouseDown or mouseDrag callback!
    jassert (inputSourceCausingDrag != nullptr && inputSourceCausingDrag->isDragging());

    return inputSourceCausingDrag;
}

bool DragAndDropContainer::isAlreadyDragging (Component* component) const noexcept
{
    for (auto* dragImageComp : dragImageComponents)
    {
        if (dragImageComp->sourceDetails.sourceComponent == component)
            return true;
    }

    return false;
}

//==============================================================================
DragAndDropTarget::SourceDetails::SourceDetails (const var& desc, Component* comp, Point<int> pos) noexcept
    : description (desc),
      sourceComponent (comp),
      localPosition (pos)
{
}

void DragAndDropTarget::itemDragEnter (const SourceDetails&)  {}
void DragAndDropTarget::itemDragMove  (const SourceDetails&)  {}
void DragAndDropTarget::itemDragExit  (const SourceDetails&)  {}
bool DragAndDropTarget::shouldDrawDragImageWhenOver()         { return true; }

//==============================================================================
void FileDragAndDropTarget::fileDragEnter (const StringArray&, int, int)  {}
void FileDragAndDropTarget::fileDragMove  (const StringArray&, int, int)  {}
void FileDragAndDropTarget::fileDragExit  (const StringArray&)            {}

void TextDragAndDropTarget::textDragEnter (const String&, int, int)  {}
void TextDragAndDropTarget::textDragMove  (const String&, int, int)  {}
void TextDragAndDropTarget::textDragExit  (const String&)            {}

} // namespace juce
