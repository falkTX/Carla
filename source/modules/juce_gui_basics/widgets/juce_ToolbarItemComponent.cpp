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

ToolbarItemFactory::ToolbarItemFactory() {}
ToolbarItemFactory::~ToolbarItemFactory() {}

//==============================================================================
class ToolbarItemComponent::ItemDragAndDropOverlayComponent    : public Component
{
public:
    ItemDragAndDropOverlayComponent()
        : isDragging (false)
    {
        setAlwaysOnTop (true);
        setRepaintsOnMouseActivity (true);
        setMouseCursor (MouseCursor::DraggingHandCursor);
    }

    void paint (Graphics& g) override
    {
        if (ToolbarItemComponent* const tc = getToolbarItemComponent())
        {
            if (isMouseOverOrDragging()
                  && tc->getEditingMode() == ToolbarItemComponent::editableOnToolbar)
            {
                g.setColour (findColour (Toolbar::editingModeOutlineColourId, true));
                g.drawRect (getLocalBounds(), jmin (2, (getWidth() - 1) / 2,
                                                       (getHeight() - 1) / 2));
            }
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        isDragging = false;

        if (ToolbarItemComponent* const tc = getToolbarItemComponent())
        {
            tc->dragOffsetX = e.x;
            tc->dragOffsetY = e.y;
        }
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown() && ! isDragging)
        {
            isDragging = true;

            if (DragAndDropContainer* const dnd = DragAndDropContainer::findParentDragContainerFor (this))
            {
                dnd->startDragging (Toolbar::toolbarDragDescriptor, getParentComponent(), Image(), true);

                if (ToolbarItemComponent* const tc = getToolbarItemComponent())
                {
                    tc->isBeingDragged = true;

                    if (tc->getEditingMode() == ToolbarItemComponent::editableOnToolbar)
                        tc->setVisible (false);
                }
            }
        }
    }

    void mouseUp (const MouseEvent&) override
    {
        isDragging = false;

        if (ToolbarItemComponent* const tc = getToolbarItemComponent())
        {
            tc->isBeingDragged = false;

            if (Toolbar* const tb = tc->getToolbar())
                tb->updateAllItemPositions (true);
            else if (tc->getEditingMode() == ToolbarItemComponent::editableOnToolbar)
                delete tc;
        }
    }

    void parentSizeChanged() override
    {
        setBounds (0, 0, getParentWidth(), getParentHeight());
    }

private:
    //==============================================================================
    bool isDragging;

    ToolbarItemComponent* getToolbarItemComponent() const noexcept
    {
        return dynamic_cast<ToolbarItemComponent*> (getParentComponent());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ItemDragAndDropOverlayComponent)
};


//==============================================================================
ToolbarItemComponent::ToolbarItemComponent (const int itemId_,
                                            const String& labelText,
                                            const bool isBeingUsedAsAButton_)
    : Button (labelText),
      itemId (itemId_),
      mode (normalMode),
      toolbarStyle (Toolbar::iconsOnly),
      dragOffsetX (0),
      dragOffsetY (0),
      isActive (true),
      isBeingDragged (false),
      isBeingUsedAsAButton (isBeingUsedAsAButton_)
{
    // Your item ID can't be 0!
    jassert (itemId_ != 0);
}

ToolbarItemComponent::~ToolbarItemComponent()
{
    overlayComp = nullptr;
}

Toolbar* ToolbarItemComponent::getToolbar() const
{
    return dynamic_cast<Toolbar*> (getParentComponent());
}

bool ToolbarItemComponent::isToolbarVertical() const
{
    const Toolbar* const t = getToolbar();
    return t != nullptr && t->isVertical();
}

void ToolbarItemComponent::setStyle (const Toolbar::ToolbarItemStyle& newStyle)
{
    if (toolbarStyle != newStyle)
    {
        toolbarStyle = newStyle;
        repaint();
        resized();
    }
}

void ToolbarItemComponent::paintButton (Graphics& g, const bool over, const bool down)
{
    if (isBeingUsedAsAButton)
        getLookAndFeel().paintToolbarButtonBackground (g, getWidth(), getHeight(),
                                                       over, down, *this);

    if (toolbarStyle != Toolbar::iconsOnly)
    {
        const int indent = contentArea.getX();
        int y = indent;
        int h = getHeight() - indent * 2;

        if (toolbarStyle == Toolbar::iconsWithText)
        {
            y = contentArea.getBottom() + indent / 2;
            h -= contentArea.getHeight();
        }

        getLookAndFeel().paintToolbarButtonLabel (g, indent, y, getWidth() - indent * 2, h,
                                                  getButtonText(), *this);
    }

    if (! contentArea.isEmpty())
    {
        Graphics::ScopedSaveState ss (g);

        g.reduceClipRegion (contentArea);
        g.setOrigin (contentArea.getPosition());

        paintButtonArea (g, contentArea.getWidth(), contentArea.getHeight(), over, down);
    }
}

void ToolbarItemComponent::resized()
{
    if (toolbarStyle != Toolbar::textOnly)
    {
        const int indent = jmin (proportionOfWidth (0.08f),
                                 proportionOfHeight (0.08f));

        contentArea = Rectangle<int> (indent, indent,
                                      getWidth() - indent * 2,
                                      toolbarStyle == Toolbar::iconsWithText ? proportionOfHeight (0.55f)
                                                                             : (getHeight() - indent * 2));
    }
    else
    {
        contentArea = Rectangle<int>();
    }

    contentAreaChanged (contentArea);
}

void ToolbarItemComponent::setEditingMode (const ToolbarEditingMode newMode)
{
    if (mode != newMode)
    {
        mode = newMode;
        repaint();

        if (mode == normalMode)
        {
            overlayComp = nullptr;
        }
        else if (overlayComp == nullptr)
        {
            addAndMakeVisible (overlayComp = new ItemDragAndDropOverlayComponent());
            overlayComp->parentSizeChanged();
        }

        resized();
    }
}

} // namespace juce
