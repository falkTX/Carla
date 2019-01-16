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

/**
    Describes the properties of an item inside a FlexBox container.

    @see FlexBox
*/
class JUCE_API  FlexItem
{
public:
    //==============================================================================
    /** Creates an item with default parameters, and zero size. */
    FlexItem() noexcept;

    /** Creates an item with the given size. */
    FlexItem (float width, float height) noexcept;

    /** Creates an item with the given size and target component. */
    FlexItem (float width, float height, Component& targetComponent) noexcept;

    /** Creates an item that represents an embedded FlexBox with a given size. */
    FlexItem (float width, float height, FlexBox& flexBoxToControl) noexcept;

    /** Creates an item with a given target component. */
    FlexItem (Component& componentToControl) noexcept;

    /** Creates an item that represents an embedded FlexBox. */
    FlexItem (FlexBox& flexBoxToControl) noexcept;

    //==============================================================================
    /** The item's current bounds. */
    Rectangle<float> currentBounds;

    /** If this is non-null, it represents a Component whose bounds are controlled by this item. */
    Component* associatedComponent = nullptr;

    /** If this is non-null, it represents a FlexBox whose bounds are controlled by this item. */
    FlexBox* associatedFlexBox = nullptr;

    /** Determines the order used to lay out items in their flex container.
        Elements are laid out in ascending order of thus order value. Elements with the same order value
        are laid out in the order in which they appear in the array.
    */
    int order = 0;

    /** Specifies the flex grow factor of this item.
        This indicates the amount of space inside the flex container the item should take up.
    */
    float flexGrow = 0.0f;

    /** Specifies the flex shrink factor of the item.
        This indicates the rate at which the item shrinks if there is insufficient space in
        the container.
    */
    float flexShrink = 1.0f;

    /** Specifies the flex-basis of the item.
        This is the initial main size of a flex item in the direction of flow. It determines the size
        of the content-box unless specified otherwise using box-sizing.
    */
    float flexBasis = 0.0f;

    /** Possible value for the alignSelf property */
    enum class AlignSelf  { autoAlign, flexStart, flexEnd, center, stretch };

    /** This is the aligh-self property of the item.
        This determines the alignment of the item along the corss-axis (perpendicular to the direction
        of flow).
    */
    AlignSelf alignSelf = AlignSelf::stretch;

    //==============================================================================
    /** This constant can be used for sizes to indicate that 'auto' mode should be used. */
    static const int autoValue    = -2;
    /** This constant can be used for sizes to indicate that no value has been set. */
    static const int notAssigned  = -1;

    float width     = (float) notAssigned;  /**< The item's width. */
    float minWidth  = 0.0f;                 /**< The item's minimum width */
    float maxWidth  = (float) notAssigned;  /**< The item's maximum width */

    float height    = (float) notAssigned;  /**< The item's height */
    float minHeight = 0.0f;                 /**< The item's minimum height */
    float maxHeight = (float) notAssigned;  /**< The item's maximum height */

    /** Represents a margin. */
    struct Margin
    {
        Margin() noexcept;              /**< Creates a margin of size zero. */
        Margin (float size) noexcept;   /**< Creates a margin with this size on all sides. */
        Margin (float top, float right, float bottom, float left) noexcept;   /**< Creates a margin with these sizes. */

        float left;   /**< Left margin size */
        float right;  /**< Right margin size */
        float top;    /**< Top margin size */
        float bottom; /**< Bottom margin size */
    };

    /** The margin to leave around this item. */
    Margin margin;

    //==============================================================================
    /** Returns a copy of this object with a new flex-grow value. */
    FlexItem withFlex (float newFlexGrow) const noexcept;

    /** Returns a copy of this object with new flex-grow and flex-shrink values. */
    FlexItem withFlex (float newFlexGrow, float newFlexShrink) const noexcept;

    /** Returns a copy of this object with new flex-grow, flex-shrink and flex-basis values. */
    FlexItem withFlex (float newFlexGrow, float newFlexShrink, float newFlexBasis) const noexcept;

    /** Returns a copy of this object with a new width. */
    FlexItem withWidth (float newWidth) const noexcept;

    /** Returns a copy of this object with a new minimum width. */
    FlexItem withMinWidth (float newMinWidth) const noexcept;

    /** Returns a copy of this object with a new maximum width. */
    FlexItem withMaxWidth (float newMaxWidth) const noexcept;

    /** Returns a copy of this object with a new height. */
    FlexItem withHeight (float newHeight) const noexcept;

    /** Returns a copy of this object with a new minimum height. */
    FlexItem withMinHeight (float newMinHeight) const noexcept;

    /** Returns a copy of this object with a new maximum height. */
    FlexItem withMaxHeight (float newMaxHeight) const noexcept;

    /** Returns a copy of this object with a new margin. */
    FlexItem withMargin (Margin) const noexcept;

    /** Returns a copy of this object with a new order. */
    FlexItem withOrder (int newOrder) const noexcept;

    /** Returns a copy of this object with a new alignSelf value. */
    FlexItem withAlignSelf (AlignSelf newAlignSelf) const noexcept;
};

} // namespace juce
