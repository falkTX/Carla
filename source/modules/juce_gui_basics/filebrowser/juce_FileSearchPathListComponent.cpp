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

FileSearchPathListComponent::FileSearchPathListComponent()
    : addButton ("+"),
      removeButton ("-"),
      changeButton (TRANS ("change...")),
      upButton (String(), DrawableButton::ImageOnButtonBackground),
      downButton (String(), DrawableButton::ImageOnButtonBackground)
{
    listBox.setModel (this);
    addAndMakeVisible (listBox);
    listBox.setColour (ListBox::backgroundColourId, Colours::black.withAlpha (0.02f));
    listBox.setColour (ListBox::outlineColourId, Colours::black.withAlpha (0.1f));
    listBox.setOutlineThickness (1);

    addAndMakeVisible (addButton);
    addButton.addListener (this);
    addButton.setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnBottom | Button::ConnectedOnTop);

    addAndMakeVisible (removeButton);
    removeButton.addListener (this);
    removeButton.setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnBottom | Button::ConnectedOnTop);

    addAndMakeVisible (changeButton);
    changeButton.addListener (this);

    addAndMakeVisible (upButton);
    upButton.addListener (this);

    {
        Path arrowPath;
        arrowPath.addArrow (Line<float> (50.0f, 100.0f, 50.0f, 0.0f), 40.0f, 100.0f, 50.0f);
        DrawablePath arrowImage;
        arrowImage.setFill (Colours::black.withAlpha (0.4f));
        arrowImage.setPath (arrowPath);

        upButton.setImages (&arrowImage);
    }

    addAndMakeVisible (downButton);
    downButton.addListener (this);

    {
        Path arrowPath;
        arrowPath.addArrow (Line<float> (50.0f, 0.0f, 50.0f, 100.0f), 40.0f, 100.0f, 50.0f);
        DrawablePath arrowImage;
        arrowImage.setFill (Colours::black.withAlpha (0.4f));
        arrowImage.setPath (arrowPath);

        downButton.setImages (&arrowImage);
    }

    updateButtons();
}

FileSearchPathListComponent::~FileSearchPathListComponent()
{
}

void FileSearchPathListComponent::updateButtons()
{
    const bool anythingSelected = listBox.getNumSelectedRows() > 0;

    removeButton.setEnabled (anythingSelected);
    changeButton.setEnabled (anythingSelected);
    upButton.setEnabled (anythingSelected);
    downButton.setEnabled (anythingSelected);
}

void FileSearchPathListComponent::changed()
{
    listBox.updateContent();
    listBox.repaint();
    updateButtons();
}

//==============================================================================
void FileSearchPathListComponent::setPath (const FileSearchPath& newPath)
{
    if (newPath.toString() != path.toString())
    {
        path = newPath;
        changed();
    }
}

void FileSearchPathListComponent::setDefaultBrowseTarget (const File& newDefaultDirectory)
{
    defaultBrowseTarget = newDefaultDirectory;
}

//==============================================================================
int FileSearchPathListComponent::getNumRows()
{
    return path.getNumPaths();
}

void FileSearchPathListComponent::paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll (findColour (TextEditor::highlightColourId));

    g.setColour (findColour (ListBox::textColourId));
    Font f (height * 0.7f);
    f.setHorizontalScale (0.9f);
    g.setFont (f);

    g.drawText (path [rowNumber].getFullPathName(),
                4, 0, width - 6, height,
                Justification::centredLeft, true);
}

void FileSearchPathListComponent::deleteKeyPressed (int row)
{
    if (isPositiveAndBelow (row, path.getNumPaths()))
    {
        path.remove (row);
        changed();
    }
}

void FileSearchPathListComponent::returnKeyPressed (int row)
{
   #if JUCE_MODAL_LOOPS_PERMITTED
    FileChooser chooser (TRANS("Change folder..."), path [row], "*");

    if (chooser.browseForDirectory())
    {
        path.remove (row);
        path.add (chooser.getResult(), row);
        changed();
    }
   #else
    ignoreUnused (row);
   #endif
}

void FileSearchPathListComponent::listBoxItemDoubleClicked (int row, const MouseEvent&)
{
    returnKeyPressed (row);
}

void FileSearchPathListComponent::selectedRowsChanged (int)
{
    updateButtons();
}

void FileSearchPathListComponent::paint (Graphics& g)
{
    g.fillAll (findColour (backgroundColourId));
}

void FileSearchPathListComponent::resized()
{
    const int buttonH = 22;
    const int buttonY = getHeight() - buttonH - 4;
    listBox.setBounds (2, 2, getWidth() - 4, buttonY - 5);

    addButton.setBounds (2, buttonY, buttonH, buttonH);
    removeButton.setBounds (addButton.getRight(), buttonY, buttonH, buttonH);

    changeButton.changeWidthToFitText (buttonH);
    downButton.setSize (buttonH * 2, buttonH);
    upButton.setSize (buttonH * 2, buttonH);

    downButton.setTopRightPosition (getWidth() - 2, buttonY);
    upButton.setTopRightPosition (downButton.getX() - 4, buttonY);
    changeButton.setTopRightPosition (upButton.getX() - 8, buttonY);
}

bool FileSearchPathListComponent::isInterestedInFileDrag (const StringArray&)
{
    return true;
}

void FileSearchPathListComponent::filesDropped (const StringArray& filenames, int, int mouseY)
{
    for (int i = filenames.size(); --i >= 0;)
    {
        const File f (filenames[i]);

        if (f.isDirectory())
        {
            const int row = listBox.getRowContainingPosition (0, mouseY - listBox.getY());
            path.add (f, row);
            changed();
        }
    }
}

void FileSearchPathListComponent::buttonClicked (Button* button)
{
    const int currentRow = listBox.getSelectedRow();

    if (button == &removeButton)
    {
        deleteKeyPressed (currentRow);
    }
    else if (button == &addButton)
    {
        File start (defaultBrowseTarget);

        if (start == File())
            start = path [0];

        if (start == File())
            start = File::getCurrentWorkingDirectory();

       #if JUCE_MODAL_LOOPS_PERMITTED
        FileChooser chooser (TRANS("Add a folder..."), start, "*");

        if (chooser.browseForDirectory())
            path.add (chooser.getResult(), currentRow);
       #else
        jassertfalse; // needs rewriting to deal with non-modal environments
       #endif
    }
    else if (button == &changeButton)
    {
        returnKeyPressed (currentRow);
    }
    else if (button == &upButton)
    {
        if (currentRow > 0 && currentRow < path.getNumPaths())
        {
            const File f (path[currentRow]);
            path.remove (currentRow);
            path.add (f, currentRow - 1);
            listBox.selectRow (currentRow - 1);
        }
    }
    else if (button == &downButton)
    {
        if (currentRow >= 0 && currentRow < path.getNumPaths() - 1)
        {
            const File f (path[currentRow]);
            path.remove (currentRow);
            path.add (f, currentRow + 1);
            listBox.selectRow (currentRow + 1);
        }
    }

    changed();
}

} // namespace juce
