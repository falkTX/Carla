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

//==============================================================================
/**
    Controls the order in which focus moves between components.

    The default algorithm used by this class to work out the order of traversal
    is as follows:
    - if two components both have an explicit focus order specified, then the
      one with the lowest number comes first (see the Component::setExplicitFocusOrder()
      method).
    - any component with an explicit focus order greater than 0 comes before ones
      that don't have an order specified.
    - any unspecified components are traversed in a left-to-right, then top-to-bottom
      order.

    If you need traversal in a more customised way, you can create a subclass
    of KeyboardFocusTraverser that uses your own algorithm, and use
    Component::createFocusTraverser() to create it.

    @see Component::setExplicitFocusOrder, Component::createFocusTraverser
*/
class JUCE_API  KeyboardFocusTraverser
{
public:
    KeyboardFocusTraverser();

    /** Destructor. */
    virtual ~KeyboardFocusTraverser();

    /** Returns the component that should be given focus after the specified one
        when moving "forwards".

        The default implementation will return the next component which is to the
        right of or below this one.

        This may return nullptr if there's no suitable candidate.
    */
    virtual Component* getNextComponent (Component* current);

    /** Returns the component that should be given focus after the specified one
        when moving "backwards".

        The default implementation will return the next component which is to the
        left of or above this one.

        This may return nullptr if there's no suitable candidate.
    */
    virtual Component* getPreviousComponent (Component* current);

    /** Returns the component that should receive focus be default within the given
        parent component.

        The default implementation will just return the foremost child component that
        wants focus.

        This may return nullptr if there's no suitable candidate.
    */
    virtual Component* getDefaultComponent (Component* parentComponent);
};

} // namespace juce
