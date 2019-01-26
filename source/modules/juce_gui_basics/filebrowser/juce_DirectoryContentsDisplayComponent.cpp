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

DirectoryContentsDisplayComponent::DirectoryContentsDisplayComponent (DirectoryContentsList& l)
    : directoryContentsList (l)
{
}

DirectoryContentsDisplayComponent::~DirectoryContentsDisplayComponent()
{
}

//==============================================================================
FileBrowserListener::~FileBrowserListener()
{
}

void DirectoryContentsDisplayComponent::addListener (FileBrowserListener* l)    { listeners.add (l); }
void DirectoryContentsDisplayComponent::removeListener (FileBrowserListener* l) { listeners.remove (l); }

void DirectoryContentsDisplayComponent::sendSelectionChangeMessage()
{
    Component::BailOutChecker checker (dynamic_cast<Component*> (this));
    listeners.callChecked (checker, &FileBrowserListener::selectionChanged);
}

void DirectoryContentsDisplayComponent::sendMouseClickMessage (const File& file, const MouseEvent& e)
{
    if (directoryContentsList.getDirectory().exists())
    {
        Component::BailOutChecker checker (dynamic_cast<Component*> (this));
        listeners.callChecked (checker, &FileBrowserListener::fileClicked, file, e);
    }
}

void DirectoryContentsDisplayComponent::sendDoubleClickMessage (const File& file)
{
    if (directoryContentsList.getDirectory().exists())
    {
        Component::BailOutChecker checker (dynamic_cast<Component*> (this));
        listeners.callChecked (checker, &FileBrowserListener::fileDoubleClicked, file);
    }
}

} // namespace juce
