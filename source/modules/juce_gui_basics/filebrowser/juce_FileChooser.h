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
    Creates a dialog box to choose a file or directory to load or save.

    To use a FileChooser:
    - create one (as a local stack variable is the neatest way)
    - call one of its browseFor.. methods
    - if this returns true, the user has selected a file, so you can retrieve it
      with the getResult() method.

    e.g. @code
    void loadMooseFile()
    {
        FileChooser myChooser ("Please select the moose you want to load...",
                               File::getSpecialLocation (File::userHomeDirectory),
                               "*.moose");

        if (myChooser.browseForFileToOpen())
        {
            File mooseFile (myChooser.getResult());

            loadMoose (mooseFile);
        }
    }
    @endcode
*/
class JUCE_API  FileChooser
{
public:
    //==============================================================================
    /** Creates a FileChooser.

        After creating one of these, use one of the browseFor... methods to display it.

        @param dialogBoxTitle                 a text string to display in the dialog box to
                                              tell the user what's going on
        @param initialFileOrDirectory         the file or directory that should be selected
                                              when the dialog box opens. If this parameter is
                                              set to File(), a sensible default directory will
                                              be used instead.
        @param filePatternsAllowed            a set of file patterns to specify which files
                                              can be selected - each pattern should be
                                              separated by a comma or semi-colon, e.g. "*" or
                                              "*.jpg;*.gif". An empty string means that all
                                              files are allowed
        @param useOSNativeDialogBox           if true, then a native dialog box will be used
                                              if possible; if false, then a Juce-based
                                              browser dialog box will always be used
        @param treatFilePackagesAsDirectories if true, then the file chooser will allow the
                                              selection of files inside packages when
                                              invoked on OS X and when using native dialog
                                              boxes.

        @see browseForFileToOpen, browseForFileToSave, browseForDirectory
    */
    FileChooser (const String& dialogBoxTitle,
                 const File& initialFileOrDirectory = File(),
                 const String& filePatternsAllowed = String(),
                 bool useOSNativeDialogBox = true,
                 bool treatFilePackagesAsDirectories = false);

    /** Destructor. */
    ~FileChooser();

    //==============================================================================
    /** Shows a dialog box to choose a file to open.

        This will display the dialog box modally, using an "open file" mode, so that
        it won't allow non-existent files or directories to be chosen.

        @param previewComponent   an optional component to display inside the dialog
                                  box to show special info about the files that the user
                                  is browsing. The component will not be deleted by this
                                  object, so the caller must take care of it.
        @returns    true if the user selected a file, in which case, use the getResult()
                    method to find out what it was. Returns false if they cancelled instead.
        @see browseForFileToSave, browseForDirectory
    */
    bool browseForFileToOpen (FilePreviewComponent* previewComponent = nullptr);

    /** Same as browseForFileToOpen, but allows the user to select multiple files.

        The files that are returned can be obtained by calling getResults(). See
        browseForFileToOpen() for more info about the behaviour of this method.
    */
    bool browseForMultipleFilesToOpen (FilePreviewComponent* previewComponent = nullptr);

    /** Shows a dialog box to choose a file to save.

        This will display the dialog box modally, using an "save file" mode, so it
        will allow non-existent files to be chosen, but not directories.

        @param warnAboutOverwritingExistingFiles     if true, the dialog box will ask
                    the user if they're sure they want to overwrite a file that already
                    exists
        @returns    true if the user chose a file and pressed 'ok', in which case, use
                    the getResult() method to find out what the file was. Returns false
                    if they cancelled instead.
        @see browseForFileToOpen, browseForDirectory
    */
    bool browseForFileToSave (bool warnAboutOverwritingExistingFiles);

    /** Shows a dialog box to choose a directory.

        This will display the dialog box modally, using an "open directory" mode, so it
        will only allow directories to be returned, not files.

        @returns    true if the user chose a directory and pressed 'ok', in which case, use
                    the getResult() method to find out what they chose. Returns false
                    if they cancelled instead.
        @see browseForFileToOpen, browseForFileToSave
    */
    bool browseForDirectory();

    /** Same as browseForFileToOpen, but allows the user to select multiple files and directories.

        The files that are returned can be obtained by calling getResults(). See
        browseForFileToOpen() for more info about the behaviour of this method.
    */
    bool browseForMultipleFilesOrDirectories (FilePreviewComponent* previewComponent = nullptr);

    //==============================================================================
    /** Runs a dialog box for the given set of option flags.
        The flag values used are those in FileBrowserComponent::FileChooserFlags.

        @returns    true if the user chose a directory and pressed 'ok', in which case, use
                    the getResult() method to find out what they chose. Returns false
                    if they cancelled instead.
        @see FileBrowserComponent::FileChooserFlags
    */
    bool showDialog (int flags, FilePreviewComponent* previewComponent);

    //==============================================================================
    /** Returns the last file that was chosen by one of the browseFor methods.

        After calling the appropriate browseFor... method, this method lets you
        find out what file or directory they chose.

        Note that the file returned is only valid if the browse method returned true (i.e.
        if the user pressed 'ok' rather than cancelling).

        If you're using a multiple-file select, then use the getResults() method instead,
        to obtain the list of all files chosen.

        @see getResults
    */
    File getResult() const;

    /** Returns a list of all the files that were chosen during the last call to a
        browse method.

        This array may be empty if no files were chosen, or can contain multiple entries
        if multiple files were chosen.

        @see getResult
    */
    const Array<File>& getResults() const noexcept      { return results; }

private:
    //==============================================================================
    String title, filters;
    const File startingFile;
    Array<File> results;
    const bool useNativeDialogBox;
    const bool treatFilePackagesAsDirs;

    static void showPlatformDialog (Array<File>& results, const String& title, const File& file,
                                    const String& filters, bool selectsDirectories, bool selectsFiles,
                                    bool isSave, bool warnAboutOverwritingExistingFiles, bool selectMultipleFiles,
                                    bool treatFilePackagesAsDirs, FilePreviewComponent* previewComponent);
    static bool isPlatformDialogAvailable();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileChooser)
};

} // namespace juce
