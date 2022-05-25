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
    Manages details about connected display devices.

    @tags{GUI}
*/
class JUCE_API  Displays
{
private:
    Displays (Desktop&);

public:
    //==============================================================================
    /** Represents a connected display device. */
    struct JUCE_API  Display
    {
        /** This will be true if this is the user's main display device. */
        bool isMain;

        /** The total area of this display in logical pixels including any OS-dependent objects
            like the taskbar, menu bar, etc.
        */
        Rectangle<int> totalArea;

        /** The total area of this display in logical pixels which isn't covered by OS-dependent
            objects like the taskbar, menu bar, etc.
        */
        Rectangle<int> userArea;

        /** Represents the area of this display in logical pixels that is not functional for
            displaying content.

            On mobile devices this may be the area covered by display cutouts and notches, where
            you still want to draw a background but should not position important content.
        */
        BorderSize<int> safeAreaInsets;

        /** The top-left of this display in physical coordinates. */
        Point<int> topLeftPhysical;

        /** The scale factor of this display.

            For higher-resolution displays, or displays with a user-defined scale factor set,
            this may be a value other than 1.0.

            This value is used to convert between physical and logical pixels. For example, a Component
            with size 10x10 will use 20x20 physical pixels on a display with a scale factor of 2.0.
        */
        double scale;

        /** The DPI of the display.

            This is the number of physical pixels per inch. To get the number of logical
            pixels per inch, divide this by the Display::scale value.
        */
        double dpi;
    };

    //==============================================================================
    /** Converts an integer Rectangle from physical to logical pixels.

        If useScaleFactorOfDisplay is not null then its scale factor will be used for the conversion
        regardless of the display that the Rectangle to be converted is on.
    */
    Rectangle<int> physicalToLogical (Rectangle<int> physicalRect,
                                      const Display* useScaleFactorOfDisplay = nullptr) const noexcept;

    /** Converts a floating-point Rectangle from physical to logical pixels.

        If useScaleFactorOfDisplay is not null then its scale factor will be used for the conversion
        regardless of the display that the Rectangle to be converted is on.
    */
    Rectangle<float> physicalToLogical (Rectangle<float> physicalRect,
                                        const Display* useScaleFactorOfDisplay = nullptr) const noexcept;

    /** Converts an integer Rectangle from logical to physical pixels.

        If useScaleFactorOfDisplay is not null then its scale factor will be used for the conversion
        regardless of the display that the Rectangle to be converted is on.
    */
    Rectangle<int> logicalToPhysical (Rectangle<int> logicalRect,
                                      const Display* useScaleFactorOfDisplay = nullptr) const noexcept;

    /** Converts a floating-point Rectangle from logical to physical pixels.

        If useScaleFactorOfDisplay is not null then its scale factor will be used for the conversion
        regardless of the display that the Rectangle to be converted is on.
    */
    Rectangle<float> logicalToPhysical (Rectangle<float> logicalRect,
                                        const Display* useScaleFactorOfDisplay = nullptr) const noexcept;

    /** Converts a Point from physical to logical pixels.

        If useScaleFactorOfDisplay is not null then its scale factor will be used for the conversion
        regardless of the display that the Point to be converted is on.
    */
    template <typename ValueType>
    Point<ValueType> physicalToLogical (Point<ValueType> physicalPoint,
                                        const Display* useScaleFactorOfDisplay = nullptr) const noexcept;

    /** Converts a Point from logical to physical pixels.

        If useScaleFactorOfDisplay is not null then its scale factor will be used for the conversion
        regardless of the display that the Point to be converted is on.
    */
    template <typename ValueType>
    Point<ValueType> logicalToPhysical (Point<ValueType> logicalPoint,
                                        const Display* useScaleFactorOfDisplay = nullptr) const noexcept;

    /** Returns the Display object representing the display containing a given Rectangle (either
        in logical or physical pixels), or nullptr if there are no connected displays.

        If the Rectangle lies outside all the displays then the nearest one will be returned.
    */
    const Display* getDisplayForRect (Rectangle<int> rect, bool isPhysical = false) const noexcept;

    /** Returns the Display object representing the display containing a given Point (either
        in logical or physical pixels), or nullptr if there are no connected displays.

        If the Point lies outside all the displays then the nearest one will be returned.
    */
    const Display* getDisplayForPoint (Point<int> point, bool isPhysical = false) const noexcept;

    /** Returns the Display object representing the display acting as the user's main screen, or nullptr
        if there are no connected displays.
    */
    const Display* getPrimaryDisplay() const noexcept;

    /** Returns a RectangleList made up of all the displays in LOGICAL pixels. */
    RectangleList<int> getRectangleList (bool userAreasOnly) const;

    /** Returns the smallest bounding box which contains all the displays in LOGICAL pixels. */
    Rectangle<int> getTotalBounds (bool userAreasOnly) const;

    /** An Array containing the Display objects for all of the connected displays. */
    Array<Display> displays;

   #ifndef DOXYGEN
    /** @internal */
    void refresh();

    [[deprecated ("Use the getDisplayForPoint or getDisplayForRect methods instead "
                 "as they can deal with converting between logical and physical pixels.")]]
    const Display& getDisplayContaining (Point<int> position) const noexcept;

    // These methods have been deprecated - use the methods which return a Display* instead as they will return
    // nullptr on headless systems with no connected displays
    [[deprecated]] const Display& findDisplayForRect (Rectangle<int>, bool isPhysical = false) const noexcept;
    [[deprecated]] const Display& findDisplayForPoint (Point<int>, bool isPhysical = false) const noexcept;
    [[deprecated]] const Display& getMainDisplay() const noexcept;
   #endif

private:
    friend class Desktop;

    void init (Desktop&);
    void findDisplays (float masterScale);

    void updateToLogical();

    Display emptyDisplay;
};

} // namespace juce
