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
    A menu bar component.

    @see MenuBarModel

    @tags{GUI}
*/
class JUCE_API  MenuBarComponent  : public Component,
                                    private MenuBarModel::Listener,
                                    private Timer
{
public:
    //==============================================================================
    /** Creates a menu bar.

        @param model    the model object to use to control this bar. You can
                        pass omit the parameter or pass nullptr into this if you like,
                        and set the model later using the setModel() method.
    */
    MenuBarComponent (MenuBarModel* model = nullptr);

    /** Destructor. */
    ~MenuBarComponent() override;

    //==============================================================================
    /** Changes the model object to use to control the bar.

        This can be a null pointer, in which case the bar will be empty. Don't delete the object
        that is passed-in while it's still being used by this MenuBar.
    */
    void setModel (MenuBarModel* newModel);

    /** Returns the current menu bar model being used. */
    MenuBarModel* getModel() const noexcept;

    //==============================================================================
    /** Pops up one of the menu items.

        This lets you manually open one of the menus - it could be triggered by a
        key shortcut, for example.
    */
    void showMenu (int menuIndex);

    //==============================================================================
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseEnter (const MouseEvent&) override;
    /** @internal */
    void mouseExit (const MouseEvent&) override;
    /** @internal */
    void mouseDown (const MouseEvent&) override;
    /** @internal */
    void mouseDrag (const MouseEvent&) override;
    /** @internal */
    void mouseUp (const MouseEvent&) override;
    /** @internal */
    void mouseMove (const MouseEvent&) override;
    /** @internal */
    void handleCommandMessage (int commandId) override;
    /** @internal */
    bool keyPressed (const KeyPress&) override;
    /** @internal */
    void menuBarItemsChanged (MenuBarModel*) override;
    /** @internal */
    void menuCommandInvoked (MenuBarModel*, const ApplicationCommandTarget::InvocationInfo&) override;

private:
    //==============================================================================
    class AccessibleItemComponent;

    //==============================================================================
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;
    void timerCallback() override;

    int getItemAt (Point<int>);
    void setItemUnderMouse (int);
    void setOpenItem (int);
    void updateItemUnderMouse (Point<int>);
    void repaintMenuItem (int);
    void menuDismissed (int, int);

    void updateItemComponents (const StringArray&);
    int indexOfItemComponent (AccessibleItemComponent*) const;

    //==============================================================================
    MenuBarModel* model = nullptr;
    std::vector<std::unique_ptr<AccessibleItemComponent>> itemComponents;

    Point<int> lastMousePos;
    int itemUnderMouse = -1, currentPopupIndex = -1, topLevelIndexClicked = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuBarComponent)
};

} // namespace juce
