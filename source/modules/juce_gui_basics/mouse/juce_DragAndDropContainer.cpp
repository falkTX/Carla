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

bool juce_performDragDropFiles (const StringArray&, const bool copyFiles, bool& shouldStop);
bool juce_performDragDropText (const String&, bool& shouldStop);


//==============================================================================
class DragAndDropContainer::DragImageComponent  : public Component,
                                                  private Timer
{
public:
    DragImageComponent (const Image& im,
                        const var& desc,
                        Component* const sourceComponent,
                        Component* const mouseSource,
                        DragAndDropContainer& ddc,
                        Point<int> offset)
        : sourceDetails (desc, sourceComponent, Point<int>()),
          image (im), owner (ddc),
          mouseDragSource (mouseSource),
          imageOffset (offset)
    {
        updateSize();

        if (mouseDragSource == nullptr)
            mouseDragSource = sourceComponent;

        mouseDragSource->addMouseListener (this, false);

        startTimer (200);

        setInterceptsMouseClicks (false, false);
        setAlwaysOnTop (true);
    }

    ~DragImageComponent()
    {
        if (owner.dragImageComponent == this)
            owner.dragImageComponent.release();

        if (mouseDragSource != nullptr)
        {
            mouseDragSource->removeMouseListener (this);

            if (DragAndDropTarget* const current = getCurrentlyOver())
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
        g.drawImageAt (image, 0, 0);
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (e.originalComponent != this)
        {
            if (mouseDragSource != nullptr)
                mouseDragSource->removeMouseListener (this);

            // (note: use a local copy of this in case the callback runs
            // a modal loop and deletes this object before the method completes)
            DragAndDropTarget::SourceDetails details (sourceDetails);
            DragAndDropTarget* finalTarget = nullptr;

            const bool wasVisible = isVisible();
            setVisible (false);
            Component* unused;
            finalTarget = findTarget (e.getScreenPosition(), details.localPosition, unused);

            if (wasVisible) // fade the component and remove it - it'll be deleted later by the timer callback
                dismissWithAnimation (finalTarget == nullptr);

            if (Component* parent = getParentComponent())
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
        if (e.originalComponent != this)
            updateLocation (true, e.getScreenPosition());
    }

    void updateLocation (const bool canDoExternalDrag, Point<int> screenPos)
    {
        DragAndDropTarget::SourceDetails details (sourceDetails);

        setNewScreenPos (screenPos);

        Component* newTargetComp;
        DragAndDropTarget* const newTarget = findTarget (screenPos, details.localPosition, newTargetComp);

        setVisible (newTarget == nullptr || newTarget->shouldDrawDragImageWhenOver());

        if (newTargetComp != currentlyOverComp)
        {
            if (DragAndDropTarget* const lastTarget = getCurrentlyOver())
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
            const Time now (Time::getCurrentTime());

            if (getCurrentlyOver() != nullptr)
                lastTimeOverTarget = now;
            else if (now > lastTimeOverTarget + RelativeTime::milliseconds (700))
                checkForExternalDrag (details, screenPos);
        }

        forceMouseCursorUpdate();
    }

    void updateImage (const Image& newImage)
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
        else if (! isMouseButtonDownAnywhere())
        {
            if (mouseDragSource != nullptr)
                mouseDragSource->removeMouseListener (this);

            deleteSelf();
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
    Image image;
    DragAndDropContainer& owner;
    WeakReference<Component> mouseDragSource, currentlyOverComp;
    const Point<int> imageOffset;
    bool hasCheckedForExternalDrag = false;
    Time lastTimeOverTarget;

    void updateSize()
    {
        setSize (image.getWidth(), image.getHeight());
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
        Desktop& desktop = Desktop::getInstance();

        for (int i = desktop.getNumComponents(); --i >= 0;)
        {
            Component* c = desktop.getComponent(i);

            if (Component* hit = c->getComponentAt (c->getLocalPoint (nullptr, screenPos)))
                return hit;
        }

        return nullptr;
    }

    DragAndDropTarget* findTarget (Point<int> screenPos, Point<int>& relativePos,
                                   Component*& resultComponent) const
    {
        Component* hit = getParentComponent();

        if (hit == nullptr)
            hit = findDesktopComponentBelow (screenPos);
        else
            hit = hit->getComponentAt (hit->getLocalPoint (nullptr, screenPos));

        // (note: use a local copy of this in case the callback runs
        // a modal loop and deletes this object before the method completes)
        const DragAndDropTarget::SourceDetails details (sourceDetails);

        while (hit != nullptr)
        {
            if (DragAndDropTarget* const ddt = dynamic_cast<DragAndDropTarget*> (hit))
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
        Point<int> newPos (screenPos - imageOffset);

        if (Component* p = getParentComponent())
            newPos = p->getLocalPoint (nullptr, newPos);

        setTopLeftPosition (newPos);
    }

    void sendDragMove (DragAndDropTarget::SourceDetails& details) const
    {
        if (DragAndDropTarget* const target = getCurrentlyOver())
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

                if (ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown())
                {
                    StringArray files;
                    bool canMoveFiles = false;

                    if (owner.shouldDropFilesWhenDraggedExternally (details, files, canMoveFiles) && ! files.isEmpty())
                    {
                        MessageManager::callAsync ([=]()  { DragAndDropContainer::performExternalDragDropOfFiles (files, canMoveFiles); });
                        deleteSelf();
                        return;
                    }

                    String text;

                    if (owner.shouldDropTextWhenDraggedExternally (details, text) && text.isNotEmpty())
                    {
                        MessageManager::callAsync ([=]()  { DragAndDropContainer::performExternalDragDropOfText (text); });
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
        ComponentAnimator& animator = Desktop::getInstance().getAnimator();

        if (shouldSnapBack && sourceDetails.sourceComponent != nullptr)
        {
            const Point<int> target (sourceDetails.sourceComponent->localPointToGlobal (sourceDetails.sourceComponent->getLocalBounds().getCentre()));
            const Point<int> ourCentre (localPointToGlobal (getLocalBounds().getCentre()));

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

    JUCE_DECLARE_NON_COPYABLE (DragImageComponent)
};


//==============================================================================
DragAndDropContainer::DragAndDropContainer()
{
}

DragAndDropContainer::~DragAndDropContainer()
{
    dragImageComponent = nullptr;
}

void DragAndDropContainer::startDragging (const var& sourceDescription,
                                          Component* sourceComponent,
                                          Image dragImage,
                                          const bool allowDraggingToExternalWindows,
                                          const Point<int>* imageOffsetFromMouse)
{
    if (dragImageComponent == nullptr)
    {
        MouseInputSource* const draggingSource = Desktop::getInstance().getDraggingMouseSource (0);

        if (draggingSource == nullptr || ! draggingSource->isDragging())
        {
            jassertfalse;   // You must call startDragging() from within a mouseDown or mouseDrag callback!
            return;
        }

        const Point<int> lastMouseDown (draggingSource->getLastMouseDownPosition().roundToInt());
        Point<int> imageOffset;

        if (dragImage.isNull())
        {
            dragImage = sourceComponent->createComponentSnapshot (sourceComponent->getLocalBounds())
                            .convertedToFormat (Image::ARGB);

            dragImage.multiplyAllAlphas (0.6f);

            const int lo = 150;
            const int hi = 400;

            Point<int> relPos (sourceComponent->getLocalPoint (nullptr, lastMouseDown));
            Point<int> clipped (dragImage.getBounds().getConstrainedPoint (relPos));
            Random random;

            for (int y = dragImage.getHeight(); --y >= 0;)
            {
                const double dy = (y - clipped.getY()) * (y - clipped.getY());

                for (int x = dragImage.getWidth(); --x >= 0;)
                {
                    const int dx = x - clipped.getX();
                    const int distance = roundToInt (std::sqrt (dx * dx + dy));

                    if (distance > lo)
                    {
                        const float alpha = (distance > hi) ? 0
                                                            : (hi - distance) / (float) (hi - lo)
                                                                + random.nextFloat() * 0.008f;

                        dragImage.multiplyAlphaAt (x, y, alpha);
                    }
                }
            }

            imageOffset = clipped;
        }
        else
        {
            if (imageOffsetFromMouse == nullptr)
                imageOffset = dragImage.getBounds().getCentre();
            else
                imageOffset = dragImage.getBounds().getConstrainedPoint (-*imageOffsetFromMouse);
        }

        dragImageComponent = new DragImageComponent (dragImage, sourceDescription, sourceComponent,
                                                     draggingSource->getComponentUnderMouse(), *this, imageOffset);

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
            if (Component* const thisComp = dynamic_cast<Component*> (this))
            {
                thisComp->addChildComponent (dragImageComponent);
            }
            else
            {
                jassertfalse;   // Your DragAndDropContainer needs to be a Component!
                return;
            }
        }

        static_cast<DragImageComponent*> (dragImageComponent.get())->updateLocation (false, lastMouseDown);
        dragImageComponent->enterModalState();

       #if JUCE_WINDOWS
        // Under heavy load, the layered window's paint callback can often be lost by the OS,
        // so forcing a repaint at least once makes sure that the window becomes visible..
        if (ComponentPeer* const peer = dragImageComponent->getPeer())
            peer->performAnyPendingRepaintsNow();
       #endif

        dragOperationStarted (dragImageComponent->sourceDetails);
    }
}

bool DragAndDropContainer::isDragAndDropActive() const
{
    return dragImageComponent != nullptr;
}

var DragAndDropContainer::getCurrentDragDescription() const
{
    return dragImageComponent != nullptr ? dragImageComponent->sourceDetails.description
                                         : var();
}

void DragAndDropContainer::setCurrentDragImage (const Image& newImage)
{
    if (dragImageComponent != nullptr)
        dragImageComponent->updateImage (newImage);
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
