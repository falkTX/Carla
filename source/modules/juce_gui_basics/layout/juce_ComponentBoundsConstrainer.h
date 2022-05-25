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

//==============================================================================
/**
    A class that imposes restrictions on a Component's size or position.

    This is used by classes such as ResizableCornerComponent,
    ResizableBorderComponent and ResizableWindow.

    The base class can impose some basic size and position limits, but you can
    also subclass this for custom uses.

    @see ResizableCornerComponent, ResizableBorderComponent, ResizableWindow

    @tags{GUI}
*/
class JUCE_API  ComponentBoundsConstrainer
{
public:
    //==============================================================================
    /** When first created, the object will not impose any restrictions on the components. */
    ComponentBoundsConstrainer() noexcept;

    /** Destructor. */
    virtual ~ComponentBoundsConstrainer();

    //==============================================================================
    /** Imposes a minimum width limit. */
    void setMinimumWidth (int minimumWidth) noexcept;

    /** Returns the current minimum width. */
    int getMinimumWidth() const noexcept                        { return minW; }

    /** Imposes a maximum width limit. */
    void setMaximumWidth (int maximumWidth) noexcept;

    /** Returns the current maximum width. */
    int getMaximumWidth() const noexcept                        { return maxW; }

    /** Imposes a minimum height limit. */
    void setMinimumHeight (int minimumHeight) noexcept;

    /** Returns the current minimum height. */
    int getMinimumHeight() const noexcept                       { return minH; }

    /** Imposes a maximum height limit. */
    void setMaximumHeight (int maximumHeight) noexcept;

    /** Returns the current maximum height. */
    int getMaximumHeight() const noexcept                       { return maxH; }

    /** Imposes a minimum width and height limit. */
    void setMinimumSize (int minimumWidth,
                         int minimumHeight) noexcept;

    /** Imposes a maximum width and height limit. */
    void setMaximumSize (int maximumWidth,
                         int maximumHeight) noexcept;

    /** Set all the maximum and minimum dimensions. */
    void setSizeLimits (int minimumWidth,
                        int minimumHeight,
                        int maximumWidth,
                        int maximumHeight) noexcept;

    //==============================================================================
    /** Sets the amount by which the component is allowed to go off-screen.

        The values indicate how many pixels must remain on-screen when dragged off
        one of its parent's edges, so e.g. if minimumWhenOffTheTop is set to 10, then
        when the component goes off the top of the screen, its y-position will be
        clipped so that there are always at least 10 pixels on-screen. In other words,
        the lowest y-position it can take would be (10 - the component's height).

        If you pass 0 or less for one of these amounts, the component is allowed
        to move beyond that edge completely, with no restrictions at all.

        If you pass a very large number (i.e. larger that the dimensions of the
        component itself), then the component won't be allowed to overlap that
        edge at all. So e.g. setting minimumWhenOffTheLeft to 0xffffff will mean that
        the component will bump into the left side of the screen and go no further.
    */
    void setMinimumOnscreenAmounts (int minimumWhenOffTheTop,
                                    int minimumWhenOffTheLeft,
                                    int minimumWhenOffTheBottom,
                                    int minimumWhenOffTheRight) noexcept;


    /** Returns the minimum distance the bounds can be off-screen. @see setMinimumOnscreenAmounts */
    int getMinimumWhenOffTheTop() const noexcept        { return minOffTop; }
    /** Returns the minimum distance the bounds can be off-screen. @see setMinimumOnscreenAmounts */
    int getMinimumWhenOffTheLeft() const noexcept       { return minOffLeft; }
    /** Returns the minimum distance the bounds can be off-screen. @see setMinimumOnscreenAmounts */
    int getMinimumWhenOffTheBottom() const noexcept     { return minOffBottom; }
    /** Returns the minimum distance the bounds can be off-screen. @see setMinimumOnscreenAmounts */
    int getMinimumWhenOffTheRight() const noexcept      { return minOffRight; }

    //==============================================================================
    /** Specifies a width-to-height ratio that the resizer should always maintain.

        If the value is 0, no aspect ratio is enforced. If it's non-zero, the width
        will always be maintained as this multiple of the height.

        @see setResizeLimits
    */
    void setFixedAspectRatio (double widthOverHeight) noexcept;

    /** Returns the aspect ratio that was set with setFixedAspectRatio().

        If no aspect ratio is being enforced, this will return 0.
    */
    double getFixedAspectRatio() const noexcept;


    //==============================================================================
    /** This callback changes the given coordinates to impose whatever the current
        constraints are set to be.

        @param bounds               the target position that should be examined and adjusted
        @param previousBounds       the component's current size
        @param limits               the region in which the component can be positioned
        @param isStretchingTop      whether the top edge of the component is being resized
        @param isStretchingLeft     whether the left edge of the component is being resized
        @param isStretchingBottom   whether the bottom edge of the component is being resized
        @param isStretchingRight    whether the right edge of the component is being resized
    */
    virtual void checkBounds (Rectangle<int>& bounds,
                              const Rectangle<int>& previousBounds,
                              const Rectangle<int>& limits,
                              bool isStretchingTop,
                              bool isStretchingLeft,
                              bool isStretchingBottom,
                              bool isStretchingRight);

    /** This callback happens when the resizer is about to start dragging. */
    virtual void resizeStart();

    /** This callback happens when the resizer has finished dragging. */
    virtual void resizeEnd();

    /** Checks the given bounds, and then sets the component to the corrected size. */
    void setBoundsForComponent (Component* component,
                                Rectangle<int> bounds,
                                bool isStretchingTop,
                                bool isStretchingLeft,
                                bool isStretchingBottom,
                                bool isStretchingRight);

    /** Performs a check on the current size of a component, and moves or resizes
        it if it fails the constraints.
    */
    void checkComponentBounds (Component* component);

    /** Called by setBoundsForComponent() to apply a new constrained size to a
        component.

        By default this just calls setBounds(), but is virtual in case it's needed for
        extremely cunning purposes.
    */
    virtual void applyBoundsToComponent (Component&, Rectangle<int> bounds);

private:
    //==============================================================================
    int minW = 0, maxW = 0x3fffffff, minH = 0, maxH = 0x3fffffff;
    int minOffTop = 0, minOffLeft = 0, minOffBottom = 0, minOffRight = 0;
    double aspectRatio = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentBoundsConstrainer)
};

} // namespace juce
