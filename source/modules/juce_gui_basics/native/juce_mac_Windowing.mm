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

void LookAndFeel::playAlertSound()
{
    NSBeep();
}

//==============================================================================
class OSXMessageBox  : private AsyncUpdater
{
public:
    OSXMessageBox (AlertWindow::AlertIconType type, const String& t, const String& m,
                   const char* b1, const char* b2, const char* b3,
                   ModalComponentManager::Callback* c, const bool runAsync)
        : iconType (type), title (t), message (m), callback (c),
          button1 (b1), button2 (b2), button3 (b3)
    {
        if (runAsync)
            triggerAsyncUpdate();
    }

    int getResult() const
    {
        switch (getRawResult())
        {
            case NSAlertFirstButtonReturn:  return 1;
            case NSAlertThirdButtonReturn:  return 2;
            default:                        return 0;
        }
    }

    static int show (AlertWindow::AlertIconType iconType, const String& title, const String& message,
                     ModalComponentManager::Callback* callback, const char* b1, const char* b2, const char* b3,
                     bool runAsync)
    {
        ScopedPointer<OSXMessageBox> mb (new OSXMessageBox (iconType, title, message, b1, b2, b3,
                                                            callback, runAsync));
        if (! runAsync)
            return mb->getResult();

        mb.release();
        return 0;
    }

private:
    AlertWindow::AlertIconType iconType;
    String title, message;
    ScopedPointer<ModalComponentManager::Callback> callback;
    const char* button1;
    const char* button2;
    const char* button3;

    void handleAsyncUpdate() override
    {
        const int result = getResult();

        if (callback != nullptr)
            callback->modalStateFinished (result);

        delete this;
    }

    NSInteger getRawResult() const
    {
        NSAlert* alert = [[[NSAlert alloc] init] autorelease];

        [alert setMessageText:     juceStringToNS (title)];
        [alert setInformativeText: juceStringToNS (message)];

        [alert setAlertStyle: iconType == AlertWindow::WarningIcon ? NSAlertStyleCritical
                                                                   : NSAlertStyleInformational];
        addButton (alert, button1);
        addButton (alert, button2);
        addButton (alert, button3);

        return [alert runModal];
    }

    static void addButton (NSAlert* alert, const char* button)
    {
        if (button != nullptr)
            [alert addButtonWithTitle: juceStringToNS (TRANS (button))];
    }
};

#if JUCE_MODAL_LOOPS_PERMITTED
void JUCE_CALLTYPE NativeMessageBox::showMessageBox (AlertWindow::AlertIconType iconType,
                                                     const String& title, const String& message,
                                                     Component* /*associatedComponent*/)
{
    OSXMessageBox::show (iconType, title, message, nullptr, "OK", nullptr, nullptr, false);
}
#endif

void JUCE_CALLTYPE NativeMessageBox::showMessageBoxAsync (AlertWindow::AlertIconType iconType,
                                                          const String& title, const String& message,
                                                          Component* /*associatedComponent*/,
                                                          ModalComponentManager::Callback* callback)
{
    OSXMessageBox::show (iconType, title, message, callback, "OK", nullptr, nullptr, true);
}

bool JUCE_CALLTYPE NativeMessageBox::showOkCancelBox (AlertWindow::AlertIconType iconType,
                                                      const String& title, const String& message,
                                                      Component* /*associatedComponent*/,
                                                      ModalComponentManager::Callback* callback)
{
    return OSXMessageBox::show (iconType, title, message, callback,
                                "OK", "Cancel", nullptr, callback != nullptr) == 1;
}

int JUCE_CALLTYPE NativeMessageBox::showYesNoCancelBox (AlertWindow::AlertIconType iconType,
                                                        const String& title, const String& message,
                                                        Component* /*associatedComponent*/,
                                                        ModalComponentManager::Callback* callback)
{
    return OSXMessageBox::show (iconType, title, message, callback,
                                "Yes", "Cancel", "No", callback != nullptr);
}

int JUCE_CALLTYPE NativeMessageBox::showYesNoBox (AlertWindow::AlertIconType iconType,
                                                  const String& title, const String& message,
                                                  Component* /*associatedComponent*/,
                                                  ModalComponentManager::Callback* callback)
{
    return OSXMessageBox::show (iconType, title, message, callback,
                                "Yes", "No", nullptr, callback != nullptr);
}


//==============================================================================
static NSRect getDragRect (NSView* view, NSEvent* event)
{
    auto eventPos = [event locationInWindow];

    return [view convertRect: NSMakeRect (eventPos.x - 16.0f, eventPos.y - 16.0f, 32.0f, 32.0f)
                    fromView: nil];
}

NSView* getNSViewForDragEvent (Component* sourceComp)
{
    if (sourceComp == nullptr)
        if (auto* draggingSource = Desktop::getInstance().getDraggingMouseSource(0))
            sourceComp = draggingSource->getComponentUnderMouse();

    if (sourceComp != nullptr)
            return (NSView*) sourceComp->getWindowHandle();

    jassertfalse;  // This method must be called in response to a component's mouseDown or mouseDrag event!
    return nil;
}

struct TextDragDataProviderClass   : public ObjCClass<NSObject>
{
    TextDragDataProviderClass()  : ObjCClass<NSObject> ("JUCE_NSTextDragDataProvider_")
    {
        addIvar<String*> ("text");
        addMethod (@selector (dealloc), dealloc, "v@:");
        addMethod (@selector (pasteboard:item:provideDataForType:), provideDataForType, "v@:@@@");
        addProtocol (@protocol (NSPasteboardItemDataProvider));
        registerClass();
    }

    static void setText (id self, const String& text)
    {
        object_setInstanceVariable (self, "text", new String (text));
    }

private:
    static void dealloc (id self, SEL)
    {
        delete getIvar<String*> (self, "text");
        sendSuperclassMessage (self, @selector (dealloc));
    }

    static void provideDataForType (id self, SEL, NSPasteboard* sender, NSPasteboardItem*, NSString* type)
    {
        if ([type compare: NSPasteboardTypeString] == NSOrderedSame)
            if (auto* text = getIvar<String*> (self, "text"))
                [sender setData: [juceStringToNS (*text) dataUsingEncoding: NSUTF8StringEncoding]
                        forType: NSPasteboardTypeString];
    }
};

bool DragAndDropContainer::performExternalDragDropOfText (const String& text, Component* sourceComponent)
{
    if (text.isEmpty())
        return false;

    if (auto* view = getNSViewForDragEvent (sourceComponent))
    {
        JUCE_AUTORELEASEPOOL
        {
            if (auto* event = [[view window] currentEvent])
            {
                static TextDragDataProviderClass dataProviderClass;
                id delegate = [dataProviderClass.createInstance() init];
                TextDragDataProviderClass::setText (delegate, text);

                auto* pasteboardItem = [[NSPasteboardItem new] autorelease];
                [pasteboardItem setDataProvider: delegate
                                       forTypes: [NSArray arrayWithObjects: NSPasteboardTypeString, nil]];

                auto* dragItem = [[[NSDraggingItem alloc] initWithPasteboardWriter: pasteboardItem] autorelease];

                NSImage* image = [[NSWorkspace sharedWorkspace] iconForFile: nsEmptyString()];
                [dragItem setDraggingFrame: getDragRect (view, event) contents: image];

                auto* draggingSession = [view beginDraggingSessionWithItems: [NSArray arrayWithObject: dragItem]
                                                                      event: event
                                                                     source: delegate];

                draggingSession.animatesToStartingPositionsOnCancelOrFail = YES;
                draggingSession.draggingFormation = NSDraggingFormationNone;
                return true;
            }
        }
    }

    return false;
}

class NSDraggingSourceHelper   : public ObjCClass <NSObject <NSDraggingSource>>
{
public:
    NSDraggingSourceHelper()
        : ObjCClass <NSObject <NSDraggingSource>> ("JUCENSDraggingSourceHelper_")
    {
        addMethod (@selector (draggingSession:sourceOperationMaskForDraggingContext:), sourceOperationMaskForDraggingContext, "c@:@@");

        registerClass();
    }

private:
    static NSDragOperation sourceOperationMaskForDraggingContext (id, SEL, NSDraggingSession*, NSDraggingContext)
    {
        return NSDragOperationCopy;
    }
};

static NSDraggingSourceHelper draggingSourceHelper;

bool DragAndDropContainer::performExternalDragDropOfFiles (const StringArray& files, bool /*canMoveFiles*/,
                                                           Component* sourceComponent)
{
    if (files.isEmpty())
        return false;

    if (auto* view = getNSViewForDragEvent (sourceComponent))
    {
        JUCE_AUTORELEASEPOOL
        {
            if (auto* event = [[view window] currentEvent])
            {
                auto* dragItems = [[[NSMutableArray alloc] init] autorelease];
                for (auto& filename : files)
                {
                    auto* nsFilename = juceStringToNS (filename);
                    auto* fileURL = [NSURL fileURLWithPath: nsFilename];
                    auto* dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter: fileURL];

                    auto eventPos = [event locationInWindow];
                    auto dragRect = [view convertRect: NSMakeRect (eventPos.x - 16.0f, eventPos.y - 16.0f, 32.0f, 32.0f)
                                             fromView: nil];
                    auto *dragImage = [[NSWorkspace sharedWorkspace] iconForFile: nsFilename];
                    [dragItem setDraggingFrame: dragRect
                                      contents: dragImage];

                    [dragItems addObject: dragItem];
                    [dragItem release];
                }

                auto* helper = [draggingSourceHelper.createInstance() autorelease];

                if (! [view beginDraggingSessionWithItems: dragItems
                                                    event: event
                                                   source: helper])
                    return false;

                return true;
            }
        }
    }

    return false;
}

//==============================================================================
bool Desktop::canUseSemiTransparentWindows() noexcept
{
    return true;
}

Point<float> MouseInputSource::getCurrentRawMousePosition()
{
    JUCE_AUTORELEASEPOOL
    {
        auto p = [NSEvent mouseLocation];
        return { (float) p.x, (float) (getMainScreenHeight() - p.y) };
    }
}

void MouseInputSource::setRawMousePosition (Point<float> newPosition)
{
    // this rubbish needs to be done around the warp call, to avoid causing a
    // bizarre glitch..
    CGAssociateMouseAndMouseCursorPosition (false);
    CGWarpMouseCursorPosition (convertToCGPoint (newPosition));
    CGAssociateMouseAndMouseCursorPosition (true);
}

double Desktop::getDefaultMasterScale()
{
    return 1.0;
}

Desktop::DisplayOrientation Desktop::getCurrentOrientation() const
{
    return upright;
}

//==============================================================================
#if defined (MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7)
 #define JUCE_USE_IOPM_SCREENSAVER_DEFEAT 1
#endif

#if ! (defined (JUCE_USE_IOPM_SCREENSAVER_DEFEAT) || defined (__POWER__))
 extern "C"  { extern OSErr UpdateSystemActivity (UInt8); } // Some versions of the SDK omit this function..
#endif

class ScreenSaverDefeater   : public Timer
{
public:
   #if JUCE_USE_IOPM_SCREENSAVER_DEFEAT
    ScreenSaverDefeater()
    {
        startTimer (5000);
        timerCallback();
    }

    void timerCallback() override
    {
        if (Process::isForegroundProcess())
        {
            if (assertion == nullptr)
                assertion = new PMAssertion();
        }
        else
        {
            assertion = nullptr;
        }
    }

    struct PMAssertion
    {
        PMAssertion()  : assertionID (kIOPMNullAssertionID)
        {
            IOReturn res = IOPMAssertionCreateWithName (kIOPMAssertionTypePreventUserIdleDisplaySleep,
                                                        kIOPMAssertionLevelOn,
                                                        CFSTR ("JUCE Playback"),
                                                        &assertionID);
            jassert (res == kIOReturnSuccess); ignoreUnused (res);
        }

        ~PMAssertion()
        {
            if (assertionID != kIOPMNullAssertionID)
                IOPMAssertionRelease (assertionID);
        }

        IOPMAssertionID assertionID;
    };

    ScopedPointer<PMAssertion> assertion;
   #else
    ScreenSaverDefeater()
    {
        startTimer (10000);
        timerCallback();
    }

    void timerCallback() override
    {
        if (Process::isForegroundProcess())
            UpdateSystemActivity (1 /*UsrActivity*/);
    }
   #endif
};

static ScopedPointer<ScreenSaverDefeater> screenSaverDefeater;

void Desktop::setScreenSaverEnabled (const bool isEnabled)
{
    if (isEnabled)
        screenSaverDefeater = nullptr;
    else if (screenSaverDefeater == nullptr)
        screenSaverDefeater = new ScreenSaverDefeater();
}

bool Desktop::isScreenSaverEnabled()
{
    return screenSaverDefeater == nullptr;
}

//==============================================================================
class DisplaySettingsChangeCallback  : private DeletedAtShutdown
{
public:
    DisplaySettingsChangeCallback()
    {
        CGDisplayRegisterReconfigurationCallback (displayReconfigurationCallBack, 0);
    }

    ~DisplaySettingsChangeCallback()
    {
        CGDisplayRemoveReconfigurationCallback (displayReconfigurationCallBack, 0);
        clearSingletonInstance();
    }

    static void displayReconfigurationCallBack (CGDirectDisplayID, CGDisplayChangeSummaryFlags, void*)
    {
        const_cast<Desktop::Displays&> (Desktop::getInstance().getDisplays()).refresh();
    }

    juce_DeclareSingleton_SingleThreaded_Minimal (DisplaySettingsChangeCallback)

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DisplaySettingsChangeCallback)
};

juce_ImplementSingleton_SingleThreaded (DisplaySettingsChangeCallback)

static Rectangle<int> convertDisplayRect (NSRect r, CGFloat mainScreenBottom)
{
    r.origin.y = mainScreenBottom - (r.origin.y + r.size.height);
    return convertToRectInt (r);
}

static Desktop::Displays::Display getDisplayFromScreen (NSScreen* s, CGFloat& mainScreenBottom, const float masterScale)
{
    Desktop::Displays::Display d;

    d.isMain = (mainScreenBottom == 0);

    if (d.isMain)
        mainScreenBottom = [s frame].size.height;

    d.userArea  = convertDisplayRect ([s visibleFrame], mainScreenBottom) / masterScale;
    d.totalArea = convertDisplayRect ([s frame], mainScreenBottom) / masterScale;
    d.scale = masterScale;

   #if defined (MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)
    if ([s respondsToSelector: @selector (backingScaleFactor)])
        d.scale *= s.backingScaleFactor;
   #endif

    NSSize dpi = [[[s deviceDescription] objectForKey: NSDeviceResolution] sizeValue];
    d.dpi = (dpi.width + dpi.height) / 2.0;

    return d;
}

void Desktop::Displays::findDisplays (const float masterScale)
{
    JUCE_AUTORELEASEPOOL
    {
        DisplaySettingsChangeCallback::getInstance();

        CGFloat mainScreenBottom = 0;

        for (NSScreen* s in [NSScreen screens])
            displays.add (getDisplayFromScreen (s, mainScreenBottom, masterScale));
    }
}

//==============================================================================
bool juce_areThereAnyAlwaysOnTopWindows()
{
    for (NSWindow* window in [NSApp windows])
        if ([window level] > NSNormalWindowLevel)
            return true;

    return false;
}

//==============================================================================
static void selectImageForDrawing (const Image& image)
{
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext: [NSGraphicsContext graphicsContextWithGraphicsPort: juce_getImageContext (image)
                                                                                     flipped: false]];
}

static void releaseImageAfterDrawing()
{
    [[NSGraphicsContext currentContext] flushGraphics];
    [NSGraphicsContext restoreGraphicsState];
}

Image juce_createIconForFile (const File& file)
{
    JUCE_AUTORELEASEPOOL
    {
        NSImage* image = [[NSWorkspace sharedWorkspace] iconForFile: juceStringToNS (file.getFullPathName())];

        Image result (Image::ARGB, (int) [image size].width, (int) [image size].height, true);

        selectImageForDrawing (result);
        [image drawAtPoint: NSMakePoint (0, 0)
                  fromRect: NSMakeRect (0, 0, [image size].width, [image size].height)
                 operation: NSCompositingOperationSourceOver fraction: 1.0f];
        releaseImageAfterDrawing();

        return result;
    }
}

static Image createNSWindowSnapshot (NSWindow* nsWindow)
{
    JUCE_AUTORELEASEPOOL
    {
        CGImageRef screenShot = CGWindowListCreateImage (CGRectNull,
                                                         kCGWindowListOptionIncludingWindow,
                                                         (CGWindowID) [nsWindow windowNumber],
                                                         kCGWindowImageBoundsIgnoreFraming);

        NSBitmapImageRep* bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage: screenShot];

        Image result (Image::ARGB, (int) [bitmapRep size].width, (int) [bitmapRep size].height, true);

        selectImageForDrawing (result);
        [bitmapRep drawAtPoint: NSMakePoint (0, 0)];
        releaseImageAfterDrawing();

        [bitmapRep release];
        CGImageRelease (screenShot);

        return result;
    }
}

Image createSnapshotOfNativeWindow (void* nativeWindowHandle)
{
    if (id windowOrView = (id) nativeWindowHandle)
    {
        if ([windowOrView isKindOfClass: [NSWindow class]])
            return createNSWindowSnapshot ((NSWindow*) windowOrView);

        if ([windowOrView isKindOfClass: [NSView class]])
            return createNSWindowSnapshot ([(NSView*) windowOrView window]);
    }

    return Image();
}

//==============================================================================
void SystemClipboard::copyTextToClipboard (const String& text)
{
    NSPasteboard* pb = [NSPasteboard generalPasteboard];

    [pb declareTypes: [NSArray arrayWithObject: NSStringPboardType]
               owner: nil];

    [pb setString: juceStringToNS (text)
          forType: NSStringPboardType];
}

String SystemClipboard::getTextFromClipboard()
{
    return nsStringToJuce ([[NSPasteboard generalPasteboard] stringForType: NSStringPboardType]);
}

void Process::setDockIconVisible (bool isVisible)
{
    ProcessSerialNumber psn { 0, kCurrentProcess };
    ProcessApplicationTransformState state = isVisible
        ? kProcessTransformToForegroundApplication
        : kProcessTransformToUIElementApplication;

    OSStatus err = TransformProcessType (&psn, state);

    jassert (err == 0);
    ignoreUnused (err);
}

bool Desktop::isOSXDarkModeActive()
{
    return [[[NSUserDefaults standardUserDefaults] stringForKey: nsStringLiteral ("AppleInterfaceStyle")]
                isEqualToString: nsStringLiteral ("Dark")];
}

} // namespace juce
