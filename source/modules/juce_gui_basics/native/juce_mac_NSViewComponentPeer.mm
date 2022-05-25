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

#include "juce_mac_CGMetalLayerRenderer.h"

@interface NSEvent (DeviceDelta)
- (float)deviceDeltaX;
- (float)deviceDeltaY;
@end

//==============================================================================
namespace juce
{

using AppFocusChangeCallback = void (*)();
extern AppFocusChangeCallback appFocusChangeCallback;
using CheckEventBlockedByModalComps = bool (*) (NSEvent*);
extern CheckEventBlockedByModalComps isEventBlockedByModalComps;

//==============================================================================
static constexpr int translateVirtualToAsciiKeyCode (int keyCode) noexcept
{
    switch (keyCode)
    {
        // The virtual keycodes are from HIToolbox/Events.h
        case 0x00: return 'A';
        case 0x01: return 'S';
        case 0x02: return 'D';
        case 0x03: return 'F';
        case 0x04: return 'H';
        case 0x05: return 'G';
        case 0x06: return 'Z';
        case 0x07: return 'X';
        case 0x08: return 'C';
        case 0x09: return 'V';
        case 0x0B: return 'B';
        case 0x0C: return 'Q';
        case 0x0D: return 'W';
        case 0x0E: return 'E';
        case 0x0F: return 'R';
        case 0x10: return 'Y';
        case 0x11: return 'T';
        case 0x12: return '1';
        case 0x13: return '2';
        case 0x14: return '3';
        case 0x15: return '4';
        case 0x16: return '6';
        case 0x17: return '5';
        case 0x18: return '=';  // kVK_ANSI_Equal
        case 0x19: return '9';
        case 0x1A: return '7';
        case 0x1B: return '-';  // kVK_ANSI_Minus
        case 0x1C: return '8';
        case 0x1D: return '0';
        case 0x1E: return ']';  // kVK_ANSI_RightBracket
        case 0x1F: return 'O';
        case 0x20: return 'U';
        case 0x21: return '[';  // kVK_ANSI_LeftBracket
        case 0x22: return 'I';
        case 0x23: return 'P';
        case 0x25: return 'L';
        case 0x26: return 'J';
        case 0x27: return '"';  // kVK_ANSI_Quote
        case 0x28: return 'K';
        case 0x29: return ';';  // kVK_ANSI_Semicolon
        case 0x2A: return '\\'; // kVK_ANSI_Backslash
        case 0x2B: return ',';  // kVK_ANSI_Comma
        case 0x2C: return '/';  // kVK_ANSI_Slash
        case 0x2D: return 'N';
        case 0x2E: return 'M';
        case 0x2F: return '.';  // kVK_ANSI_Period
        case 0x32: return '`';  // kVK_ANSI_Grave

        default:   return keyCode;
    }
}

constexpr int extendedKeyModifier = 0x30000;

//==============================================================================
class NSViewComponentPeer  : public ComponentPeer
{
public:
    NSViewComponentPeer (Component& comp, const int windowStyleFlags, NSView* viewToAttachTo)
        : ComponentPeer (comp, windowStyleFlags),
          safeComponent (&comp),
          isSharedWindow (viewToAttachTo != nil),
          lastRepaintTime (Time::getMillisecondCounter())
    {
        appFocusChangeCallback = appFocusChanged;
        isEventBlockedByModalComps = checkEventBlockedByModalComps;

        auto r = makeNSRect (component.getLocalBounds());

        view = [createViewInstance() initWithFrame: r];
        setOwner (view, this);

        [view registerForDraggedTypes: getSupportedDragTypes()];

        const auto options = NSTrackingMouseEnteredAndExited
                           | NSTrackingMouseMoved
                           | NSTrackingEnabledDuringMouseDrag
                           | NSTrackingActiveAlways
                           | NSTrackingInVisibleRect;
        const NSUniquePtr<NSTrackingArea> trackingArea { [[NSTrackingArea alloc] initWithRect: r
                                                                                      options: options
                                                                                        owner: view
                                                                                     userInfo: nil] };
        [view addTrackingArea: trackingArea.get()];

        notificationCenter = [NSNotificationCenter defaultCenter];

        [notificationCenter  addObserver: view
                                selector: frameChangedSelector
                                    name: NSViewFrameDidChangeNotification
                                  object: view];

        [view setPostsFrameChangedNotifications: YES];

       #if USE_COREGRAPHICS_RENDERING
        if ((windowStyleFlags & ComponentPeer::windowRequiresSynchronousCoreGraphicsRendering) == 0)
        {
            if (@available (macOS 10.8, *))
            {
                [view setWantsLayer: YES];
                [[view layer] setDrawsAsynchronously: YES];
            }
        }
       #endif

        createCVDisplayLink();

        if (isSharedWindow)
        {
            window = [viewToAttachTo window];
            [viewToAttachTo addSubview: view];
        }
        else
        {
            r.origin.x = (CGFloat) component.getX();
            r.origin.y = (CGFloat) component.getY();
            r = flippedScreenRect (r);

            window = [createWindowInstance() initWithContentRect: r
                                                       styleMask: getNSWindowStyleMask (windowStyleFlags)
                                                         backing: NSBackingStoreBuffered
                                                           defer: YES];
            setOwner (window, this);

            if (@available (macOS 10.10, *))
                [window setAccessibilityElement: YES];

            [window orderOut: nil];
            [window setDelegate: (id<NSWindowDelegate>) window];

            [window setOpaque: component.isOpaque()];

            if (! [window isOpaque])
                [window setBackgroundColor: [NSColor clearColor]];

           if (@available (macOS 10.9, *))
                [view setAppearance: [NSAppearance appearanceNamed: NSAppearanceNameAqua]];

            [window setHasShadow: ((windowStyleFlags & windowHasDropShadow) != 0)];

            if (component.isAlwaysOnTop())
                setAlwaysOnTop (true);

            [window setContentView: view];

            // We'll both retain and also release this on closing because plugin hosts can unexpectedly
            // close the window for us, and also tend to get cause trouble if setReleasedWhenClosed is NO.
            [window setReleasedWhenClosed: YES];
            [window retain];

            [window setExcludedFromWindowsMenu: (windowStyleFlags & windowIsTemporary) != 0];
            [window setIgnoresMouseEvents: (windowStyleFlags & windowIgnoresMouseClicks) != 0];

            setCollectionBehaviour (false);

            [window setRestorable: NO];

           #if defined (MAC_OS_X_VERSION_10_12) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12)
            if (@available (macOS 10.12, *))
                [window setTabbingMode: NSWindowTabbingModeDisallowed];
           #endif

            [notificationCenter  addObserver: view
                                    selector: frameChangedSelector
                                        name: NSWindowDidMoveNotification
                                      object: window];

            [notificationCenter  addObserver: view
                                    selector: frameChangedSelector
                                        name: NSWindowDidMiniaturizeNotification
                                      object: window];

            [notificationCenter  addObserver: view
                                    selector: @selector (windowWillMiniaturize:)
                                        name: NSWindowWillMiniaturizeNotification
                                      object: window];

            [notificationCenter  addObserver: view
                                    selector: @selector (windowDidDeminiaturize:)
                                        name: NSWindowDidDeminiaturizeNotification
                                      object: window];
        }

        auto alpha = component.getAlpha();

        if (alpha < 1.0f)
            setAlpha (alpha);

        setTitle (component.getName());

        getNativeRealtimeModifiers = []
        {
            if ([NSEvent respondsToSelector: @selector (modifierFlags)])
                NSViewComponentPeer::updateModifiers ([NSEvent modifierFlags]);

            return ModifierKeys::currentModifiers;
        };
    }

    ~NSViewComponentPeer() override
    {
        CVDisplayLinkStop (displayLink);
        dispatch_source_cancel (displaySource);

        [notificationCenter removeObserver: view];
        setOwner (view, nullptr);

        if ([view superview] != nil)
        {
            redirectWillMoveToWindow (nullptr);
            [view removeFromSuperview];
        }

        if (! isSharedWindow)
        {
            setOwner (window, nullptr);
            [window setContentView: nil];
            [window close];
            [window release];
        }

        [view release];
    }

    //==============================================================================
    void* getNativeHandle() const override    { return view; }

    void setVisible (bool shouldBeVisible) override
    {
        if (isSharedWindow)
        {
            if (shouldBeVisible)
                [view setHidden: false];
            else if ([window firstResponder] != view || ([window firstResponder] == view && [window makeFirstResponder: nil]))
                [view setHidden: true];
        }
        else
        {
            if (shouldBeVisible)
            {
                ++insideToFrontCall;
                [window orderFront: nil];
                --insideToFrontCall;
                handleBroughtToFront();
            }
            else
            {
                [window orderOut: nil];
            }
        }
    }

    void setTitle (const String& title) override
    {
        JUCE_AUTORELEASEPOOL
        {
            if (! isSharedWindow)
                [window setTitle: juceStringToNS (title)];
        }
    }

    bool setDocumentEditedStatus (bool edited) override
    {
        if (! hasNativeTitleBar())
            return false;

        [window setDocumentEdited: edited];
        return true;
    }

    void setRepresentedFile (const File& file) override
    {
        if (! isSharedWindow)
        {
            [window setRepresentedFilename: juceStringToNS (file != File()
                                                                ? file.getFullPathName()
                                                                : String())];

            windowRepresentsFile = (file != File());
        }
    }

    void setBounds (const Rectangle<int>& newBounds, bool) override
    {
        auto r = makeNSRect (newBounds);
        auto oldViewSize = [view frame].size;

        if (isSharedWindow)
        {
            [view setFrame: r];
        }
        else
        {
            // Repaint behaviour of setFrame seemed to change in 10.11, and the drawing became synchronous,
            // causing performance issues. But sending an async update causes flickering in older versions,
            // hence this version check to use the old behaviour on pre 10.11 machines
            static bool isPre10_11 = SystemStats::getOperatingSystemType() <= SystemStats::MacOSX_10_10;

            [window setFrame: [window frameRectForContentRect: flippedScreenRect (r)]
                     display: isPre10_11];
        }

        if (oldViewSize.width != r.size.width || oldViewSize.height != r.size.height)
            [view setNeedsDisplay: true];
    }

    Rectangle<int> getBounds (const bool global) const
    {
        auto r = [view frame];
        NSWindow* viewWindow = [view window];

        if (global && viewWindow != nil)
        {
            r = [[view superview] convertRect: r toView: nil];
            r = [viewWindow convertRectToScreen: r];

            r = flippedScreenRect (r);
        }

        return convertToRectInt (r);
    }

    Rectangle<int> getBounds() const override
    {
        return getBounds (! isSharedWindow);
    }

    Point<float> localToGlobal (Point<float> relativePosition) override
    {
        return relativePosition + getBounds (true).getPosition().toFloat();
    }

    using ComponentPeer::localToGlobal;

    Point<float> globalToLocal (Point<float> screenPosition) override
    {
        return screenPosition - getBounds (true).getPosition().toFloat();
    }

    using ComponentPeer::globalToLocal;

    void setAlpha (float newAlpha) override
    {
        if (isSharedWindow)
            [view setAlphaValue: (CGFloat) newAlpha];
        else
            [window setAlphaValue: (CGFloat) newAlpha];
    }

    void setMinimised (bool shouldBeMinimised) override
    {
        if (! isSharedWindow)
        {
            if (shouldBeMinimised)
                [window miniaturize: nil];
            else
                [window deminiaturize: nil];
        }
    }

    bool isMinimised() const override
    {
        return [window isMiniaturized];
    }

    NSWindowCollectionBehavior getCollectionBehavior (bool forceFullScreen) const
    {
        if (forceFullScreen)
            return NSWindowCollectionBehaviorFullScreenPrimary;

        // Some SDK versions don't define NSWindowCollectionBehaviorFullScreenNone
        constexpr auto fullScreenNone = (NSUInteger) (1 << 9);

        return (getStyleFlags() & (windowHasMaximiseButton | windowIsResizable)) == (windowHasMaximiseButton | windowIsResizable)
             ? NSWindowCollectionBehaviorFullScreenPrimary
             : fullScreenNone;
    }

    void setCollectionBehaviour (bool forceFullScreen) const
    {
        [window setCollectionBehavior: getCollectionBehavior (forceFullScreen)];
    }

    void setFullScreen (bool shouldBeFullScreen) override
    {
        if (isSharedWindow)
            return;

        if (shouldBeFullScreen)
            setCollectionBehaviour (true);

        if (isMinimised())
            setMinimised (false);

        if (shouldBeFullScreen != isFullScreen())
            [window toggleFullScreen: nil];
    }

    bool isFullScreen() const override
    {
        return ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
    }

    bool isKioskMode() const override
    {
        return isFullScreen() && ComponentPeer::isKioskMode();
    }

    static bool isWindowAtPoint (NSWindow* w, NSPoint screenPoint)
    {
        if ([NSWindow respondsToSelector: @selector (windowNumberAtPoint:belowWindowWithWindowNumber:)])
            return [NSWindow windowNumberAtPoint: screenPoint belowWindowWithWindowNumber: 0] == [w windowNumber];

        return true;
    }

    bool contains (Point<int> localPos, bool trueIfInAChildWindow) const override
    {
        NSRect viewFrame = [view frame];

        if (! (isPositiveAndBelow (localPos.getX(), viewFrame.size.width)
            && isPositiveAndBelow (localPos.getY(), viewFrame.size.height)))
            return false;

        if (! SystemStats::isRunningInAppExtensionSandbox())
        {
            if (NSWindow* const viewWindow = [view window])
            {
                NSRect windowFrame = [viewWindow frame];
                NSPoint windowPoint = [view convertPoint: NSMakePoint (localPos.x, localPos.y) toView: nil];
                NSPoint screenPoint = NSMakePoint (windowFrame.origin.x + windowPoint.x,
                                                   windowFrame.origin.y + windowPoint.y);

                if (! isWindowAtPoint (viewWindow, screenPoint))
                    return false;

            }
        }

        NSView* v = [view hitTest: NSMakePoint (viewFrame.origin.x + localPos.getX(),
                                                viewFrame.origin.y + localPos.getY())];

        return trueIfInAChildWindow ? (v != nil)
                                    : (v == view);
    }

    OptionalBorderSize getFrameSizeIfPresent() const override
    {
        if (! isSharedWindow)
        {
            BorderSize<int> b;

            NSRect v = [view convertRect: [view frame] toView: nil];
            NSRect w = [window frame];

            b.setTop ((int) (w.size.height - (v.origin.y + v.size.height)));
            b.setBottom ((int) v.origin.y);
            b.setLeft ((int) v.origin.x);
            b.setRight ((int) (w.size.width - (v.origin.x + v.size.width)));

            return OptionalBorderSize { b };
        }

        return {};
    }

    BorderSize<int> getFrameSize() const override
    {
        if (const auto frameSize = getFrameSizeIfPresent())
            return *frameSize;

        return {};
    }

    bool hasNativeTitleBar() const
    {
        return (getStyleFlags() & windowHasTitleBar) != 0;
    }

    bool setAlwaysOnTop (bool alwaysOnTop) override
    {
        if (! isSharedWindow)
        {
            [window setLevel: alwaysOnTop ? ((getStyleFlags() & windowIsTemporary) != 0 ? NSPopUpMenuWindowLevel
                                                                                        : NSFloatingWindowLevel)
                                          : NSNormalWindowLevel];

            isAlwaysOnTop = alwaysOnTop;
        }

        return true;
    }

    void toFront (bool makeActiveWindow) override
    {
        if (isSharedWindow)
        {
            NSView* superview = [view superview];
            NSMutableArray* subviews = [NSMutableArray arrayWithArray: [superview subviews]];

            const auto isFrontmost = [[subviews lastObject] isEqual: view];

            if (! isFrontmost)
            {
                [view retain];
                [subviews removeObject: view];
                [subviews addObject: view];

                [superview setSubviews: subviews];
                [view release];
            }
        }

        if (window != nil && component.isVisible())
        {
            ++insideToFrontCall;

            if (makeActiveWindow)
                [window makeKeyAndOrderFront: nil];
            else
                [window orderFront: nil];

            if (insideToFrontCall <= 1)
            {
                Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
                handleBroughtToFront();
            }

            --insideToFrontCall;
        }
    }

    void toBehind (ComponentPeer* other) override
    {
        if (auto* otherPeer = dynamic_cast<NSViewComponentPeer*> (other))
        {
            if (isSharedWindow)
            {
                NSView* superview = [view superview];
                NSMutableArray* subviews = [NSMutableArray arrayWithArray: [superview subviews]];

                const auto otherViewIndex = [subviews indexOfObject: otherPeer->view];

                if (otherViewIndex == NSNotFound)
                    return;

                const auto isBehind = [subviews indexOfObject: view] < otherViewIndex;

                if (! isBehind)
                {
                    [view retain];
                    [subviews removeObject: view];
                    [subviews insertObject: view
                                   atIndex: otherViewIndex];

                    [superview setSubviews: subviews];
                    [view release];
                }
            }
            else if (component.isVisible())
            {
                [window orderWindow: NSWindowBelow
                         relativeTo: [otherPeer->window windowNumber]];
            }
        }
        else
        {
            jassertfalse; // wrong type of window?
        }
    }

    void setIcon (const Image& newIcon) override
    {
        if (! isSharedWindow)
        {
            // need to set a dummy represented file here to show the file icon (which we then set to the new icon)
            if (! windowRepresentsFile)
                [window setRepresentedFilename: juceStringToNS (" ")]; // can't just use an empty string for some reason...

            auto img = NSUniquePtr<NSImage> { imageToNSImage (ScaledImage (newIcon)) };
            [[window standardWindowButton: NSWindowDocumentIconButton] setImage: img.get()];
        }
    }

    StringArray getAvailableRenderingEngines() override
    {
        StringArray s ("Software Renderer");

       #if USE_COREGRAPHICS_RENDERING
        s.add ("CoreGraphics Renderer");
       #endif

        return s;
    }

    int getCurrentRenderingEngine() const override
    {
        return usingCoreGraphics ? 1 : 0;
    }

    void setCurrentRenderingEngine (int index) override
    {
       #if USE_COREGRAPHICS_RENDERING
        if (usingCoreGraphics != (index > 0))
        {
            usingCoreGraphics = index > 0;
            [view setNeedsDisplay: true];
        }
       #else
        ignoreUnused (index);
       #endif
    }

    void redirectMouseDown (NSEvent* ev)
    {
        if (! Process::isForegroundProcess())
            Process::makeForegroundProcess();

        ModifierKeys::currentModifiers = ModifierKeys::currentModifiers.withFlags (getModifierForButtonNumber ([ev buttonNumber]));
        sendMouseEvent (ev);
    }

    void redirectMouseUp (NSEvent* ev)
    {
        ModifierKeys::currentModifiers = ModifierKeys::currentModifiers.withoutFlags (getModifierForButtonNumber ([ev buttonNumber]));
        sendMouseEvent (ev);
        showArrowCursorIfNeeded();
    }

    void redirectMouseDrag (NSEvent* ev)
    {
        ModifierKeys::currentModifiers = ModifierKeys::currentModifiers.withFlags (getModifierForButtonNumber ([ev buttonNumber]));
        sendMouseEvent (ev);
    }

    void redirectMouseMove (NSEvent* ev)
    {
        ModifierKeys::currentModifiers = ModifierKeys::currentModifiers.withoutMouseButtons();

        NSPoint windowPos = [ev locationInWindow];
        NSPoint screenPos = [[ev window] convertRectToScreen: NSMakeRect (windowPos.x, windowPos.y, 1.0f, 1.0f)].origin;

        if (isWindowAtPoint ([ev window], screenPos))
            sendMouseEvent (ev);
        else
            // moved into another window which overlaps this one, so trigger an exit
            handleMouseEvent (MouseInputSource::InputSourceType::mouse, MouseInputSource::offscreenMousePos, ModifierKeys::currentModifiers,
                              getMousePressure (ev), MouseInputSource::defaultOrientation, getMouseTime (ev));

        showArrowCursorIfNeeded();
    }

    void redirectMouseEnter (NSEvent* ev)
    {
        sendMouseEnterExit (ev);
    }

    void redirectMouseExit (NSEvent* ev)
    {
        sendMouseEnterExit (ev);
    }

    static float checkDeviceDeltaReturnValue (float v) noexcept
    {
        // (deviceDeltaX can fail and return NaN, so need to sanity-check the result)
        v *= 0.5f / 256.0f;
        return (v > -1000.0f && v < 1000.0f) ? v : 0.0f;
    }

    void redirectMouseWheel (NSEvent* ev)
    {
        updateModifiers (ev);

        MouseWheelDetails wheel;
        wheel.deltaX = 0;
        wheel.deltaY = 0;
        wheel.isReversed = false;
        wheel.isSmooth = false;
        wheel.isInertial = false;

        @try
        {
            if ([ev respondsToSelector: @selector (isDirectionInvertedFromDevice)])
                wheel.isReversed = [ev isDirectionInvertedFromDevice];

            wheel.isInertial = ([ev momentumPhase] != NSEventPhaseNone);

            if ([ev respondsToSelector: @selector (hasPreciseScrollingDeltas)])
            {
                if ([ev hasPreciseScrollingDeltas])
                {
                    const float scale = 0.5f / 256.0f;
                    wheel.deltaX = scale * (float) [ev scrollingDeltaX];
                    wheel.deltaY = scale * (float) [ev scrollingDeltaY];
                    wheel.isSmooth = true;
                }
            }
            else if ([ev respondsToSelector: @selector (deviceDeltaX)])
            {
                wheel.deltaX = checkDeviceDeltaReturnValue ([ev deviceDeltaX]);
                wheel.deltaY = checkDeviceDeltaReturnValue ([ev deviceDeltaY]);
            }
        }
        @catch (...)
        {}

        if (wheel.deltaX == 0.0f && wheel.deltaY == 0.0f)
        {
            const float scale = 10.0f / 256.0f;
            wheel.deltaX = scale * (float) [ev deltaX];
            wheel.deltaY = scale * (float) [ev deltaY];
        }

        handleMouseWheel (MouseInputSource::InputSourceType::mouse, getMousePos (ev, view), getMouseTime (ev), wheel);
    }

    void redirectMagnify (NSEvent* ev)
    {
        const float invScale = 1.0f - (float) [ev magnification];

        if (invScale > 0.0f)
            handleMagnifyGesture (MouseInputSource::InputSourceType::mouse, getMousePos (ev, view), getMouseTime (ev), 1.0f / invScale);
    }

    void redirectCopy      (NSObject*) { handleKeyPress (KeyPress ('c', ModifierKeys (ModifierKeys::commandModifier), 'c')); }
    void redirectPaste     (NSObject*) { handleKeyPress (KeyPress ('v', ModifierKeys (ModifierKeys::commandModifier), 'v')); }
    void redirectCut       (NSObject*) { handleKeyPress (KeyPress ('x', ModifierKeys (ModifierKeys::commandModifier), 'x')); }
    void redirectSelectAll (NSObject*) { handleKeyPress (KeyPress ('a', ModifierKeys (ModifierKeys::commandModifier), 'a')); }

    void redirectWillMoveToWindow (NSWindow* newWindow)
    {
        if (auto* currentWindow = [view window])
        {
            [notificationCenter removeObserver: view
                                          name: NSWindowDidMoveNotification
                                        object: currentWindow];

            [notificationCenter removeObserver: view
                                          name: NSWindowWillMiniaturizeNotification
                                        object: currentWindow];

            [notificationCenter removeObserver: view
                                          name: NSWindowDidBecomeKeyNotification
                                        object: currentWindow];

            [notificationCenter removeObserver: view
                                          name: NSWindowDidChangeScreenNotification
                                        object: currentWindow];
        }

        if (isSharedWindow && [view window] == window && newWindow == nullptr)
        {
            if (auto* comp = safeComponent.get())
                comp->setVisible (false);
        }
    }

    void sendMouseEvent (NSEvent* ev)
    {
        updateModifiers (ev);
        handleMouseEvent (MouseInputSource::InputSourceType::mouse, getMousePos (ev, view), ModifierKeys::currentModifiers,
                          getMousePressure (ev), MouseInputSource::defaultOrientation, getMouseTime (ev));
    }

    bool handleKeyEvent (NSEvent* ev, bool isKeyDown)
    {
        auto unicode = nsStringToJuce ([ev characters]);
        auto keyCode = getKeyCodeFromEvent (ev);

       #if JUCE_DEBUG_KEYCODES
        DBG ("unicode: " + unicode + " " + String::toHexString ((int) unicode[0]));
        auto unmodified = nsStringToJuce ([ev charactersIgnoringModifiers]);
        DBG ("unmodified: " + unmodified + " " + String::toHexString ((int) unmodified[0]));
       #endif

        if (keyCode != 0 || unicode.isNotEmpty())
        {
            if (isKeyDown)
            {
                bool used = false;

                for (auto u = unicode.getCharPointer(); ! u.isEmpty();)
                {
                    auto textCharacter = u.getAndAdvance();

                    switch (keyCode)
                    {
                        case NSLeftArrowFunctionKey:
                        case NSRightArrowFunctionKey:
                        case NSUpArrowFunctionKey:
                        case NSDownArrowFunctionKey:
                        case NSPageUpFunctionKey:
                        case NSPageDownFunctionKey:
                        case NSEndFunctionKey:
                        case NSHomeFunctionKey:
                        case NSDeleteFunctionKey:
                            textCharacter = 0;
                            break; // (these all seem to generate unwanted garbage unicode strings)

                        default:
                            if (([ev modifierFlags] & NSEventModifierFlagCommand) != 0
                                 || (keyCode >= NSF1FunctionKey && keyCode <= NSF35FunctionKey))
                                textCharacter = 0;
                            break;
                    }

                    used = handleKeyUpOrDown (true) || used;
                    used = handleKeyPress (keyCode, textCharacter) || used;
                }

                return used;
            }

            if (handleKeyUpOrDown (false))
                return true;
        }

        return false;
    }

    bool redirectKeyDown (NSEvent* ev)
    {
        // (need to retain this in case a modal loop runs in handleKeyEvent and
        // our event object gets lost)
        const NSUniquePtr<NSEvent> r ([ev retain]);

        updateKeysDown (ev, true);
        bool used = handleKeyEvent (ev, true);

        if (([ev modifierFlags] & NSEventModifierFlagCommand) != 0)
        {
            // for command keys, the key-up event is thrown away, so simulate one..
            updateKeysDown (ev, false);
            used = (isValidPeer (this) && handleKeyEvent (ev, false)) || used;
        }

        // (If we're running modally, don't allow unused keystrokes to be passed
        // along to other blocked views..)
        if (Component::getCurrentlyModalComponent() != nullptr)
            used = true;

        return used;
    }

    bool redirectKeyUp (NSEvent* ev)
    {
        updateKeysDown (ev, false);
        return handleKeyEvent (ev, false)
                || Component::getCurrentlyModalComponent() != nullptr;
    }

    void redirectModKeyChange (NSEvent* ev)
    {
        // (need to retain this in case a modal loop runs and our event object gets lost)
        const NSUniquePtr<NSEvent> r ([ev retain]);

        keysCurrentlyDown.clear();
        handleKeyUpOrDown (true);

        updateModifiers (ev);
        handleModifierKeysChange();
    }

    //==============================================================================
    void drawRect (NSRect r)
    {
        if (r.size.width < 1.0f || r.size.height < 1.0f)
            return;

        auto cg = []
        {
            if (@available (macOS 10.10, *))
                return (CGContextRef) [[NSGraphicsContext currentContext] CGContext];

            JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
            return (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
            JUCE_END_IGNORE_WARNINGS_GCC_LIKE
        }();

        drawRectWithContext (cg, r);
    }

    void drawRectWithContext (CGContextRef cg, NSRect r)
    {
        if (! component.isOpaque())
            CGContextClearRect (cg, CGContextGetClipBoundingBox (cg));

        float displayScale = 1.0f;
        NSScreen* screen = [[view window] screen];

        if ([screen respondsToSelector: @selector (backingScaleFactor)])
            displayScale = (float) screen.backingScaleFactor;

        auto invalidateTransparentWindowShadow = [this]
        {
            // transparent NSWindows with a drop-shadow need to redraw their shadow when the content
            // changes to avoid stale shadows being drawn behind the window
            if (! isSharedWindow && ! [window isOpaque] && [window hasShadow])
                [window invalidateShadow];
        };

       #if USE_COREGRAPHICS_RENDERING && JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS
        // This was a workaround for a CGContext not having a way of finding whether a rectangle
        // falls within its clip region. However Apple removed the capability of
        // [view getRectsBeingDrawn: ...] sometime around 10.13, so on later versions of macOS
        // numRects will always be 1 and you'll need to use a CoreGraphicsMetalLayerRenderer
        // to avoid CoreGraphics consolidating disparate rects.
        if (usingCoreGraphics && metalRenderer == nullptr)
        {
            const NSRect* rects = nullptr;
            NSInteger numRects = 0;
            [view getRectsBeingDrawn: &rects count: &numRects];

            if (numRects > 1)
            {
                for (int i = 0; i < numRects; ++i)
                {
                    NSRect rect = rects[i];
                    CGContextSaveGState (cg);
                    CGContextClipToRect (cg, CGRectMake (rect.origin.x, rect.origin.y, rect.size.width, rect.size.height));
                    renderRect (cg, rect, displayScale);
                    CGContextRestoreGState (cg);
                }

                invalidateTransparentWindowShadow();
                return;
            }
        }
       #endif

        renderRect (cg, r, displayScale);
        invalidateTransparentWindowShadow();
    }

    void renderRect (CGContextRef cg, NSRect r, float displayScale)
    {
       #if USE_COREGRAPHICS_RENDERING
        if (usingCoreGraphics)
        {
            const auto height = getComponent().getHeight();
            CGContextConcatCTM (cg, CGAffineTransformMake (1, 0, 0, -1, 0, height));
            CoreGraphicsContext context (cg, (float) height);
            handlePaint (context);
        }
        else
       #endif
        {
            const Point<int> offset (-roundToInt (r.origin.x), -roundToInt (r.origin.y));
            auto clipW = (int) (r.size.width  + 0.5f);
            auto clipH = (int) (r.size.height + 0.5f);

            RectangleList<int> clip;
            getClipRects (clip, offset, clipW, clipH);

            if (! clip.isEmpty())
            {
                Image temp (component.isOpaque() ? Image::RGB : Image::ARGB,
                            roundToInt (clipW * displayScale),
                            roundToInt (clipH * displayScale),
                            ! component.isOpaque());

                {
                    auto intScale = roundToInt (displayScale);

                    if (intScale != 1)
                        clip.scaleAll (intScale);

                    auto context = component.getLookAndFeel()
                                            .createGraphicsContext (temp, offset * intScale, clip);

                    if (intScale != 1)
                        context->addTransform (AffineTransform::scale (displayScale));

                    handlePaint (*context);
                }

                detail::ColorSpacePtr colourSpace { CGColorSpaceCreateWithName (kCGColorSpaceSRGB) };
                CGImageRef image = juce_createCoreGraphicsImage (temp, colourSpace.get());
                CGContextConcatCTM (cg, CGAffineTransformMake (1, 0, 0, -1, r.origin.x, r.origin.y + clipH));
                CGContextDrawImage (cg, CGRectMake (0.0f, 0.0f, clipW, clipH), image);
                CGImageRelease (image);
            }
        }
    }

    void repaint (const Rectangle<int>& area) override
    {
        // In 10.11 changes were made to the way the OS handles repaint regions, and it seems that it can
        // no longer be trusted to coalesce all the regions, or to even remember them all without losing
        // a few when there's a lot of activity.
        // As a work around for this, we use a RectangleList to do our own coalescing of regions before
        // asynchronously asking the OS to repaint them.
        deferredRepaints.add (area.toFloat());
    }

    static bool shouldThrottleRepaint()
    {
        return areAnyWindowsInLiveResize();
    }

    void setNeedsDisplayRectangles()
    {
        if (deferredRepaints.isEmpty())
            return;

        auto now = Time::getMillisecondCounter();
        auto msSinceLastRepaint = (lastRepaintTime >= now) ? now - lastRepaintTime
                                                           : (std::numeric_limits<uint32>::max() - lastRepaintTime) + now;

        constexpr uint32 minimumRepaintInterval = 1000 / 30; // 30fps

        // When windows are being resized, artificially throttling high-frequency repaints helps
        // to stop the event queue getting clogged, and keeps everything working smoothly.
        // For some reason Logic also needs this throttling to record parameter events correctly.
        if (msSinceLastRepaint < minimumRepaintInterval && shouldThrottleRepaint())
            return;

       #if USE_COREGRAPHICS_RENDERING && JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS
        // We require macOS 10.14 to use the Metal layer renderer
        if (@available (macOS 10.14, *))
        {
            const auto& comp = getComponent();

            // If we are resizing we need to fall back to synchronous drawing to avoid artefacts
            if (areAnyWindowsInLiveResize())
            {
                if (metalRenderer != nullptr)
                {
                    metalRenderer.reset();
                    view.wantsLayer = NO;
                    view.layer = nil;
                    deferredRepaints = comp.getLocalBounds().toFloat();
                }
            }
            else
            {
                if (metalRenderer == nullptr)
                {
                    view.wantsLayer = YES;
                    view.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
                    view.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
                    view.layer = [CAMetalLayer layer];
                    metalRenderer = std::make_unique<CoreGraphicsMetalLayerRenderer> ((CAMetalLayer*) view.layer, getComponent());
                    deferredRepaints = comp.getLocalBounds().toFloat();
                }
            }
        }
       #endif

        auto dispatchRectangles = [this] ()
        {
           #if USE_COREGRAPHICS_RENDERING
            if (@available (macOS 10.14, *))
            {
                if (metalRenderer != nullptr)
                {
                    return metalRenderer->drawRectangleList ((CAMetalLayer*) view.layer,
                                                             (float) [[view window] backingScaleFactor],
                                                             view.frame,
                                                             getComponent(),
                                                             [this] (CGContextRef ctx, CGRect r) { drawRectWithContext (ctx, r); },
                                                             deferredRepaints);
                }
            }
           #endif

            for (auto& i : deferredRepaints)
                [view setNeedsDisplayInRect: makeNSRect (i)];

            return true;
        };

        if (dispatchRectangles())
        {
            lastRepaintTime = Time::getMillisecondCounter();
            deferredRepaints.clear();
        }
    }

    void performAnyPendingRepaintsNow() override
    {
        [view displayIfNeeded];
    }

    static bool areAnyWindowsInLiveResize() noexcept
    {
        for (NSWindow* w in [NSApp windows])
            if ([w inLiveResize])
                return true;

        return false;
    }

    //==============================================================================
    bool isBlockedByModalComponent()
    {
        if (auto* modal = Component::getCurrentlyModalComponent())
        {
            if (insideToFrontCall == 0
                 && (! getComponent().isParentOf (modal))
                 && getComponent().isCurrentlyBlockedByAnotherModalComponent())
            {
                return true;
            }
        }

        return false;
    }

    enum class KeyWindowChanged { no, yes };

    void sendModalInputAttemptIfBlocked (KeyWindowChanged keyChanged)
    {
        if (! isBlockedByModalComponent())
            return;

        if (auto* modal = Component::getCurrentlyModalComponent())
        {
            if (auto* otherPeer = modal->getPeer())
            {
                const auto modalPeerIsTemporary = (otherPeer->getStyleFlags() & ComponentPeer::windowIsTemporary) != 0;

                if (! modalPeerIsTemporary)
                    return;

                // When a peer resigns key status, it might be because we just created a modal
                // component that is now key.
                // In this case, we should only dismiss the modal component if it isn't key,
                // implying that a third window has become key.
                const auto modalPeerIsKey = [NSApp keyWindow] == static_cast<NSViewComponentPeer*> (otherPeer)->window;

                if (keyChanged == KeyWindowChanged::yes && modalPeerIsKey)
                    return;

                modal->inputAttemptWhenModal();
            }
        }
    }

    bool canBecomeKeyWindow()
    {
        return component.isVisible() && (getStyleFlags() & ComponentPeer::windowIgnoresKeyPresses) == 0;
    }

    bool canBecomeMainWindow()
    {
        return component.isVisible() && dynamic_cast<ResizableWindow*> (&component) != nullptr;
    }

    bool worksWhenModal() const
    {
        // In plugins, the host could put our plugin window inside a modal window, so this
        // allows us to successfully open other popups. Feels like there could be edge-case
        // problems caused by this, so let us know if you spot any issues..
        return ! JUCEApplication::isStandaloneApp();
    }

    void becomeKeyWindow()
    {
        handleBroughtToFront();
        grabFocus();
    }

    void resignKeyWindow()
    {
        viewFocusLoss();
    }

    bool windowShouldClose()
    {
        if (! isValidPeer (this))
            return YES;

        handleUserClosingWindow();
        return NO;
    }

    void redirectMovedOrResized()
    {
        handleMovedOrResized();
        setNeedsDisplayRectangles();
    }

    void windowDidChangeScreen()
    {
        updateCVDisplayLinkScreen();
    }

    void viewMovedToWindow()
    {
        if (isSharedWindow)
        {
            auto newWindow = [view window];
            bool shouldSetVisible = (window == nullptr && newWindow != nullptr);

            window = newWindow;

            if (shouldSetVisible)
                getComponent().setVisible (true);
        }

        if (auto* currentWindow = [view window])
        {
            [notificationCenter addObserver: view
                                   selector: dismissModalsSelector
                                       name: NSWindowWillMoveNotification
                                     object: currentWindow];

            [notificationCenter addObserver: view
                                   selector: dismissModalsSelector
                                       name: NSWindowWillMiniaturizeNotification
                                     object: currentWindow];

            [notificationCenter addObserver: view
                                   selector: becomeKeySelector
                                       name: NSWindowDidBecomeKeyNotification
                                     object: currentWindow];

            [notificationCenter addObserver: view
                                   selector: resignKeySelector
                                       name: NSWindowDidResignKeyNotification
                                     object: currentWindow];

            [notificationCenter addObserver: view
                                   selector: @selector (windowDidChangeScreen:)
                                       name: NSWindowDidChangeScreenNotification
                                     object: currentWindow];

            updateCVDisplayLinkScreen();
        }
    }

    void dismissModals()
    {
        if (hasNativeTitleBar() || isSharedWindow)
            sendModalInputAttemptIfBlocked (KeyWindowChanged::no);
    }

    void becomeKey()
    {
        component.repaint();
    }

    void resignKey()
    {
        viewFocusLoss();
        sendModalInputAttemptIfBlocked (KeyWindowChanged::yes);
    }

    void liveResizingStart()
    {
        if (constrainer == nullptr)
            return;

        constrainer->resizeStart();
        isFirstLiveResize = true;

        setFullScreenSizeConstraints (*constrainer);
    }

    void liveResizingEnd()
    {
        if (constrainer != nullptr)
            constrainer->resizeEnd();
    }

    NSRect constrainRect (const NSRect r)
    {
        if (constrainer == nullptr || isKioskMode() || isFullScreen())
            return r;

        const auto scale = getComponent().getDesktopScaleFactor();

        auto pos            = ScalingHelpers::unscaledScreenPosToScaled (scale, convertToRectInt (flippedScreenRect (r)));
        const auto original = ScalingHelpers::unscaledScreenPosToScaled (scale, convertToRectInt (flippedScreenRect ([window frame])));

        const auto screenBounds = Desktop::getInstance().getDisplays().getTotalBounds (true);

        const bool inLiveResize = [window inLiveResize];

        if (! inLiveResize || isFirstLiveResize)
        {
            isFirstLiveResize = false;

            isStretchingTop    = (pos.getY() != original.getY() && pos.getBottom() == original.getBottom());
            isStretchingLeft   = (pos.getX() != original.getX() && pos.getRight()  == original.getRight());
            isStretchingBottom = (pos.getY() == original.getY() && pos.getBottom() != original.getBottom());
            isStretchingRight  = (pos.getX() == original.getX() && pos.getRight()  != original.getRight());
        }

        constrainer->checkBounds (pos, original, screenBounds,
                                  isStretchingTop, isStretchingLeft, isStretchingBottom, isStretchingRight);

        return flippedScreenRect (makeNSRect (ScalingHelpers::scaledScreenPosToUnscaled (scale, pos)));
    }

    static void showArrowCursorIfNeeded()
    {
        auto& desktop = Desktop::getInstance();
        auto mouse = desktop.getMainMouseSource();

        if (mouse.getComponentUnderMouse() == nullptr
             && desktop.findComponentAt (mouse.getScreenPosition().roundToInt()) == nullptr)
        {
            [[NSCursor arrowCursor] set];
        }
    }

    static void updateModifiers (NSEvent* e)
    {
        updateModifiers ([e modifierFlags]);
    }

    static void updateModifiers (const NSUInteger flags)
    {
        int m = 0;

        if ((flags & NSEventModifierFlagShift) != 0)        m |= ModifierKeys::shiftModifier;
        if ((flags & NSEventModifierFlagControl) != 0)      m |= ModifierKeys::ctrlModifier;
        if ((flags & NSEventModifierFlagOption) != 0)       m |= ModifierKeys::altModifier;
        if ((flags & NSEventModifierFlagCommand) != 0)      m |= ModifierKeys::commandModifier;

        ModifierKeys::currentModifiers = ModifierKeys::currentModifiers.withOnlyMouseButtons().withFlags (m);
    }

    static void updateKeysDown (NSEvent* ev, bool isKeyDown)
    {
        updateModifiers (ev);

        if (auto keyCode = getKeyCodeFromEvent (ev))
        {
            if (isKeyDown)
                keysCurrentlyDown.addIfNotAlreadyThere (keyCode);
            else
                keysCurrentlyDown.removeFirstMatchingValue (keyCode);
        }
    }

    static int getKeyCodeFromEvent (NSEvent* ev)
    {
        // Unfortunately, charactersIgnoringModifiers does not ignore the shift key.
        // Using [ev keyCode] is not a solution either as this will,
        // for example, return VK_KEY_Y if the key is pressed which
        // is typically located at the Y key position on a QWERTY
        // keyboard. However, on international keyboards this might not
        // be the key labeled Y (for example, on German keyboards this key
        // has a Z label). Therefore, we need to query the current keyboard
        // layout to figure out what character the key would have produced
        // if the shift key was not pressed
        String unmodified = nsStringToJuce ([ev charactersIgnoringModifiers]);
        auto keyCode = (int) unmodified[0];

        if (keyCode == 0x19) // (backwards-tab)
            keyCode = '\t';
        else if (keyCode == 0x03) // (enter)
            keyCode = '\r';
        else
            keyCode = (int) CharacterFunctions::toUpperCase ((juce_wchar) keyCode);

        // The purpose of the keyCode is to provide information about non-printing characters to facilitate
        // keyboard control over the application.
        //
        // So when keyCode is decoded as a printing character outside the ASCII range we need to replace it.
        // This holds when the keyCode is larger than 0xff and not part one of the two MacOS specific
        // non-printing ranges.
        if (keyCode > 0xff
            && ! (keyCode >= NSUpArrowFunctionKey && keyCode <= NSModeSwitchFunctionKey)
            && ! (keyCode >= extendedKeyModifier))
        {
            keyCode = translateVirtualToAsciiKeyCode ([ev keyCode]);
        }

        if (([ev modifierFlags] & NSEventModifierFlagNumericPad) != 0)
        {
            const int numPadConversions[] = { '0', KeyPress::numberPad0, '1', KeyPress::numberPad1,
                                              '2', KeyPress::numberPad2, '3', KeyPress::numberPad3,
                                              '4', KeyPress::numberPad4, '5', KeyPress::numberPad5,
                                              '6', KeyPress::numberPad6, '7', KeyPress::numberPad7,
                                              '8', KeyPress::numberPad8, '9', KeyPress::numberPad9,
                                              '+', KeyPress::numberPadAdd, '-', KeyPress::numberPadSubtract,
                                              '*', KeyPress::numberPadMultiply, '/', KeyPress::numberPadDivide,
                                              '.', KeyPress::numberPadDecimalPoint,
                                              ',', KeyPress::numberPadDecimalPoint, // (to deal with non-english kbds)
                                              '=', KeyPress::numberPadEquals };

            for (int i = 0; i < numElementsInArray (numPadConversions); i += 2)
                if (keyCode == numPadConversions [i])
                    keyCode = numPadConversions [i + 1];
        }

        return keyCode;
    }

    static int64 getMouseTime (NSEvent* e) noexcept
    {
        return (Time::currentTimeMillis() - Time::getMillisecondCounter())
                 + (int64) ([e timestamp] * 1000.0);
    }

    static float getMousePressure (NSEvent* e) noexcept
    {
        @try
        {
            if (e.type != NSEventTypeMouseEntered && e.type != NSEventTypeMouseExited)
                return (float) e.pressure;
        }
        @catch (NSException* e) {}
        @finally {}

        return 0.0f;
    }

    static Point<float> getMousePos (NSEvent* e, NSView* view)
    {
        NSPoint p = [view convertPoint: [e locationInWindow] fromView: nil];
        return { (float) p.x, (float) p.y };
    }

    static int getModifierForButtonNumber (const NSInteger num)
    {
        return num == 0 ? ModifierKeys::leftButtonModifier
                        : (num == 1 ? ModifierKeys::rightButtonModifier
                                    : (num == 2 ? ModifierKeys::middleButtonModifier : 0));
    }

    static unsigned int getNSWindowStyleMask (const int flags) noexcept
    {
        unsigned int style = (flags & windowHasTitleBar) != 0 ? NSWindowStyleMaskTitled
                                                              : NSWindowStyleMaskBorderless;

        if ((flags & windowHasMinimiseButton) != 0)  style |= NSWindowStyleMaskMiniaturizable;
        if ((flags & windowHasCloseButton) != 0)     style |= NSWindowStyleMaskClosable;
        if ((flags & windowIsResizable) != 0)        style |= NSWindowStyleMaskResizable;
        return style;
    }

    static NSArray* getSupportedDragTypes()
    {
        const auto type = []
        {
           #if defined (MAC_OS_X_VERSION_10_13) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_13
            if (@available (macOS 10.13, *))
                return NSPasteboardTypeFileURL;
           #endif

            JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
            return (NSString*) kUTTypeFileURL;
            JUCE_END_IGNORE_WARNINGS_GCC_LIKE
        }();

        return [NSArray arrayWithObjects: type, (NSString*) kPasteboardTypeFileURLPromise, NSPasteboardTypeString, nil];
    }

    BOOL sendDragCallback (bool (ComponentPeer::* callback) (const DragInfo&), id <NSDraggingInfo> sender)
    {
        NSPasteboard* pasteboard = [sender draggingPasteboard];
        NSString* contentType = [pasteboard availableTypeFromArray: getSupportedDragTypes()];

        if (contentType == nil)
            return false;

        const auto p = localToGlobal (convertToPointFloat ([view convertPoint: [sender draggingLocation] fromView: nil]));

        ComponentPeer::DragInfo dragInfo;
        dragInfo.position = ScalingHelpers::screenPosToLocalPos (component, p).roundToInt();

        if (contentType == NSPasteboardTypeString)
            dragInfo.text = nsStringToJuce ([pasteboard stringForType: NSPasteboardTypeString]);
        else
            dragInfo.files = getDroppedFiles (pasteboard, contentType);

        if (! dragInfo.isEmpty())
            return (this->*callback) (dragInfo);

        return false;
    }

    StringArray getDroppedFiles (NSPasteboard* pasteboard, NSString* contentType)
    {
        StringArray files;
        NSString* iTunesPasteboardType = nsStringLiteral ("CorePasteboardFlavorType 0x6974756E"); // 'itun'

        if ([contentType isEqualToString: (NSString*) kPasteboardTypeFileURLPromise]
             && [[pasteboard types] containsObject: iTunesPasteboardType])
        {
            id list = [pasteboard propertyListForType: iTunesPasteboardType];

            if ([list isKindOfClass: [NSDictionary class]])
            {
                NSDictionary* iTunesDictionary = (NSDictionary*) list;
                NSArray* tracks = [iTunesDictionary valueForKey: nsStringLiteral ("Tracks")];
                NSEnumerator* enumerator = [tracks objectEnumerator];
                NSDictionary* track;

                while ((track = [enumerator nextObject]) != nil)
                {
                    if (id value = [track valueForKey: nsStringLiteral ("Location")])
                    {
                        NSURL* url = [NSURL URLWithString: value];

                        if ([url isFileURL])
                            files.add (nsStringToJuce ([url path]));
                    }
                }
            }
        }
        else
        {
            NSArray* items = [pasteboard readObjectsForClasses:@[[NSURL class]] options: nil];

            for (unsigned int i = 0; i < [items count]; ++i)
            {
                NSURL* url = [items objectAtIndex: i];

                if ([url isFileURL])
                    files.add (nsStringToJuce ([url path]));
            }
        }

        return files;
    }

    //==============================================================================
    void viewFocusGain()
    {
        if (currentlyFocusedPeer != this)
        {
            if (ComponentPeer::isValidPeer (currentlyFocusedPeer))
                currentlyFocusedPeer->handleFocusLoss();

            currentlyFocusedPeer = this;
            handleFocusGain();
        }
    }

    void viewFocusLoss()
    {
        if (currentlyFocusedPeer == this)
        {
            currentlyFocusedPeer = nullptr;
            handleFocusLoss();
        }
    }

    bool isFocused() const override
    {
        return (isSharedWindow || ! JUCEApplication::isStandaloneApp())
                    ? this == currentlyFocusedPeer
                    : [window isKeyWindow];
    }

    void grabFocus() override
    {
        if (window != nil)
        {
            [window makeKeyWindow];
            [window makeFirstResponder: view];

            viewFocusGain();
        }
    }

    void textInputRequired (Point<int>, TextInputTarget&) override {}

    void closeInputMethodContext() override
    {
        stringBeingComposed.clear();
        const auto* inputContext = [NSTextInputContext currentInputContext];

        if (inputContext != nil)
            [inputContext discardMarkedText];
    }

    void resetWindowPresentation()
    {
        if (hasNativeTitleBar())
        {
            [window setStyleMask: (NSViewComponentPeer::getNSWindowStyleMask (getStyleFlags()))];
            setTitle (getComponent().getName()); // required to force the OS to update the title
        }

        [NSApp setPresentationOptions: NSApplicationPresentationDefault];
        setCollectionBehaviour (isFullScreen());
    }

    void setHasChangedSinceSaved (bool b) override
    {
        if (! isSharedWindow)
            [window setDocumentEdited: b];
    }

    //==============================================================================
    NSWindow* window = nil;
    NSView* view = nil;
    WeakReference<Component> safeComponent;
    bool isSharedWindow = false;
   #if USE_COREGRAPHICS_RENDERING
    bool usingCoreGraphics = true;
   #else
    bool usingCoreGraphics = false;
   #endif
    bool isZooming = false, isFirstLiveResize = false, textWasInserted = false;
    bool isStretchingTop = false, isStretchingLeft = false, isStretchingBottom = false, isStretchingRight = false;
    bool windowRepresentsFile = false;
    bool isAlwaysOnTop = false, wasAlwaysOnTop = false;
    String stringBeingComposed;
    NSNotificationCenter* notificationCenter = nil;

    Rectangle<float> lastSizeBeforeZoom;
    RectangleList<float> deferredRepaints;
    uint32 lastRepaintTime;

    static ComponentPeer* currentlyFocusedPeer;
    static Array<int> keysCurrentlyDown;
    static int insideToFrontCall;

    static const SEL dismissModalsSelector;
    static const SEL frameChangedSelector;
    static const SEL asyncMouseDownSelector;
    static const SEL asyncMouseUpSelector;
    static const SEL becomeKeySelector;
    static const SEL resignKeySelector;

private:
    static NSView* createViewInstance();
    static NSWindow* createWindowInstance();

    void sendMouseEnterExit (NSEvent* ev)
    {
        if (auto* area = [ev trackingArea])
            if (! [[view trackingAreas] containsObject: area])
                return;

        sendMouseEvent (ev);
    }

    static void setOwner (id viewOrWindow, NSViewComponentPeer* newOwner)
    {
        object_setInstanceVariable (viewOrWindow, "owner", newOwner);
    }

    void getClipRects (RectangleList<int>& clip, Point<int> offset, int clipW, int clipH)
    {
        const NSRect* rects = nullptr;
        NSInteger numRects = 0;
        [view getRectsBeingDrawn: &rects count: &numRects];

        const Rectangle<int> clipBounds (clipW, clipH);

        clip.ensureStorageAllocated ((int) numRects);

        for (int i = 0; i < numRects; ++i)
            clip.addWithoutMerging (clipBounds.getIntersection (Rectangle<int> (roundToInt (rects[i].origin.x) + offset.x,
                                                                                roundToInt (rects[i].origin.y) + offset.y,
                                                                                roundToInt (rects[i].size.width),
                                                                                roundToInt (rects[i].size.height))));
    }

    static void appFocusChanged()
    {
        keysCurrentlyDown.clear();

        if (isValidPeer (currentlyFocusedPeer))
        {
            if (Process::isForegroundProcess())
            {
                currentlyFocusedPeer->handleFocusGain();
                ModalComponentManager::getInstance()->bringModalComponentsToFront();
            }
            else
            {
                currentlyFocusedPeer->handleFocusLoss();
            }
        }
    }

    static bool checkEventBlockedByModalComps (NSEvent* e)
    {
        if (Component::getNumCurrentlyModalComponents() == 0)
            return false;

        NSWindow* const w = [e window];

        if (w == nil || [w worksWhenModal])
            return false;

        bool isKey = false, isInputAttempt = false;

        switch ([e type])
        {
            case NSEventTypeKeyDown:
            case NSEventTypeKeyUp:
                isKey = isInputAttempt = true;
                break;

            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
                isInputAttempt = true;
                break;

            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
            case NSEventTypeOtherMouseDragged:
                if (Desktop::getInstance().getDraggingMouseSource(0) != nullptr)
                    return false;
                break;

            case NSEventTypeMouseMoved:
            case NSEventTypeMouseEntered:
            case NSEventTypeMouseExited:
            case NSEventTypeCursorUpdate:
            case NSEventTypeScrollWheel:
            case NSEventTypeTabletPoint:
            case NSEventTypeTabletProximity:
                break;

            case NSEventTypeFlagsChanged:
            case NSEventTypeAppKitDefined:
            case NSEventTypeSystemDefined:
            case NSEventTypeApplicationDefined:
            case NSEventTypePeriodic:
            case NSEventTypeGesture:
            case NSEventTypeMagnify:
            case NSEventTypeSwipe:
            case NSEventTypeRotate:
            case NSEventTypeBeginGesture:
            case NSEventTypeEndGesture:
            case NSEventTypeQuickLook:
           #if JUCE_64BIT
            case NSEventTypeSmartMagnify:
            case NSEventTypePressure:
           #endif
          #if defined (MAC_OS_X_VERSION_10_12) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
           #if JUCE_64BIT
            case NSEventTypeDirectTouch:
           #endif
           #if defined (MAC_OS_X_VERSION_10_15) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_15
            case NSEventTypeChangeMode:
           #endif
          #endif
            default:
                return false;
        }

        for (int i = ComponentPeer::getNumPeers(); --i >= 0;)
        {
            if (auto* peer = dynamic_cast<NSViewComponentPeer*> (ComponentPeer::getPeer (i)))
            {
                if ([peer->view window] == w)
                {
                    if (isKey)
                    {
                        if (peer->view == [w firstResponder])
                            return false;
                    }
                    else
                    {
                        if (peer->isSharedWindow
                               ? NSPointInRect ([peer->view convertPoint: [e locationInWindow] fromView: nil], [peer->view bounds])
                               : NSPointInRect ([e locationInWindow], NSMakeRect (0, 0, [w frame].size.width, [w frame].size.height)))
                            return false;
                    }
                }
            }
        }

        if (isInputAttempt)
        {
            if (! [NSApp isActive])
                [NSApp activateIgnoringOtherApps: YES];

            if (auto* modal = Component::getCurrentlyModalComponent())
                modal->inputAttemptWhenModal();
        }

        return true;
    }

    void setFullScreenSizeConstraints (const ComponentBoundsConstrainer& c)
    {
        if (@available (macOS 10.11, *))
        {
            const auto minSize = NSMakeSize (static_cast<float> (c.getMinimumWidth()),
                                             0.0f);
            [window setMinFullScreenContentSize: minSize];
            [window setMaxFullScreenContentSize: NSMakeSize (100000, 100000)];
        }
    }

    //==============================================================================
    void onDisplaySourceCallback()
    {
        setNeedsDisplayRectangles();
    }

    void onDisplayLinkCallback()
    {
        dispatch_source_merge_data (displaySource, 1);
    }

    static CVReturn displayLinkCallback (CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*,
                                         CVOptionFlags, CVOptionFlags*, void* context)
    {
        static_cast<NSViewComponentPeer*> (context)->onDisplayLinkCallback();
        return kCVReturnSuccess;
    }

    void updateCVDisplayLinkScreen()
    {
        auto viewDisplayID = (CGDirectDisplayID) [[window.screen.deviceDescription objectForKey: @"NSScreenNumber"] unsignedIntegerValue];
        auto result = CVDisplayLinkSetCurrentCGDisplay (displayLink, viewDisplayID);
        jassertquiet (result == kCVReturnSuccess);
    }

    void createCVDisplayLink()
    {
        displaySource = dispatch_source_create (DISPATCH_SOURCE_TYPE_DATA_ADD, 0, 0, dispatch_get_main_queue());
        dispatch_source_set_event_handler (displaySource, ^(){ onDisplaySourceCallback(); });
        dispatch_resume (displaySource);

        auto cvReturn = CVDisplayLinkCreateWithActiveCGDisplays (&displayLink);
        jassertquiet (cvReturn == kCVReturnSuccess);

        cvReturn = CVDisplayLinkSetOutputCallback (displayLink, &displayLinkCallback, this);
        jassertquiet (cvReturn == kCVReturnSuccess);

        CVDisplayLinkStart (displayLink);
    }

    CVDisplayLinkRef displayLink = nullptr;
    dispatch_source_t displaySource = nullptr;

   #if USE_COREGRAPHICS_RENDERING
    std::unique_ptr<CoreGraphicsMetalLayerRenderer> metalRenderer;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NSViewComponentPeer)
};

int NSViewComponentPeer::insideToFrontCall = 0;

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
const SEL NSViewComponentPeer::dismissModalsSelector  = @selector (dismissModals);
const SEL NSViewComponentPeer::frameChangedSelector   = @selector (frameChanged:);
const SEL NSViewComponentPeer::asyncMouseDownSelector = @selector (asyncMouseDown:);
const SEL NSViewComponentPeer::asyncMouseUpSelector   = @selector (asyncMouseUp:);
const SEL NSViewComponentPeer::becomeKeySelector      = @selector (becomeKey:);
const SEL NSViewComponentPeer::resignKeySelector      = @selector (resignKey:);
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

//==============================================================================
template <typename Base>
struct NSViewComponentPeerWrapper  : public Base
{
    explicit NSViewComponentPeerWrapper (const char* baseName)
        : Base (baseName)
    {
        Base::template addIvar<NSViewComponentPeer*> ("owner");
    }

    static NSViewComponentPeer* getOwner (id self)
    {
        return getIvar<NSViewComponentPeer*> (self, "owner");
    }

    static id getAccessibleChild (id self)
    {
        if (auto* owner = getOwner (self))
            if (auto* handler = owner->getComponent().getAccessibilityHandler())
                return (id) handler->getNativeImplementation();

        return nil;
    }
};

struct JuceNSViewClass   : public NSViewComponentPeerWrapper<ObjCClass<NSView>>
{
    JuceNSViewClass()  : NSViewComponentPeerWrapper ("JUCEView_")
    {
        addMethod (@selector (isOpaque),                      isOpaque);
        addMethod (@selector (drawRect:),                     drawRect);
        addMethod (@selector (mouseDown:),                    mouseDown);
        addMethod (@selector (mouseUp:),                      mouseUp);
        addMethod (@selector (mouseDragged:),                 mouseDragged);
        addMethod (@selector (mouseMoved:),                   mouseMoved);
        addMethod (@selector (mouseEntered:),                 mouseEntered);
        addMethod (@selector (mouseExited:),                  mouseExited);
        addMethod (@selector (rightMouseDown:),               mouseDown);
        addMethod (@selector (rightMouseDragged:),            mouseDragged);
        addMethod (@selector (rightMouseUp:),                 mouseUp);
        addMethod (@selector (otherMouseDown:),               mouseDown);
        addMethod (@selector (otherMouseDragged:),            mouseDragged);
        addMethod (@selector (otherMouseUp:),                 mouseUp);
        addMethod (@selector (scrollWheel:),                  scrollWheel);
        addMethod (@selector (magnifyWithEvent:),             magnify);
        addMethod (@selector (acceptsFirstMouse:),            acceptsFirstMouse);
        addMethod (@selector (windowWillMiniaturize:),        windowWillMiniaturize);
        addMethod (@selector (windowDidDeminiaturize:),       windowDidDeminiaturize);
        addMethod (@selector (windowDidChangeScreen:),        windowDidChangeScreen);
        addMethod (@selector (wantsDefaultClipping),          wantsDefaultClipping);
        addMethod (@selector (worksWhenModal),                worksWhenModal);
        addMethod (@selector (viewDidMoveToWindow),           viewDidMoveToWindow);
        addMethod (@selector (viewWillDraw),                  viewWillDraw);
        addMethod (@selector (keyDown:),                      keyDown);
        addMethod (@selector (keyUp:),                        keyUp);
        addMethod (@selector (insertText:),                   insertText);
        addMethod (@selector (doCommandBySelector:),          doCommandBySelector);
        addMethod (@selector (setMarkedText:selectedRange:),  setMarkedText);
        addMethod (@selector (unmarkText),                    unmarkText);
        addMethod (@selector (hasMarkedText),                 hasMarkedText);
        addMethod (@selector (conversationIdentifier),        conversationIdentifier);
        addMethod (@selector (attributedSubstringFromRange:), attributedSubstringFromRange);
        addMethod (@selector (markedRange),                   markedRange);
        addMethod (@selector (selectedRange),                 selectedRange);
        addMethod (@selector (firstRectForCharacterRange:),   firstRectForCharacterRange);
        addMethod (@selector (characterIndexForPoint:),       characterIndexForPoint);
        addMethod (@selector (validAttributesForMarkedText),  validAttributesForMarkedText);
        addMethod (@selector (flagsChanged:),                 flagsChanged);

        addMethod (@selector (becomeFirstResponder),          becomeFirstResponder);
        addMethod (@selector (resignFirstResponder),          resignFirstResponder);
        addMethod (@selector (acceptsFirstResponder),         acceptsFirstResponder);

        addMethod (@selector (draggingEntered:),              draggingEntered);
        addMethod (@selector (draggingUpdated:),              draggingUpdated);
        addMethod (@selector (draggingEnded:),                draggingEnded);
        addMethod (@selector (draggingExited:),               draggingExited);
        addMethod (@selector (prepareForDragOperation:),      prepareForDragOperation);
        addMethod (@selector (performDragOperation:),         performDragOperation);
        addMethod (@selector (concludeDragOperation:),        concludeDragOperation);

        addMethod (@selector (paste:),                        paste);
        addMethod (@selector (copy:),                         copy);
        addMethod (@selector (cut:),                          cut);
        addMethod (@selector (selectAll:),                    selectAll);

        addMethod (@selector (viewWillMoveToWindow:),         willMoveToWindow);

        addMethod (@selector (isAccessibilityElement),        getIsAccessibilityElement);
        addMethod (@selector (accessibilityChildren),         getAccessibilityChildren);
        addMethod (@selector (accessibilityHitTest:),         accessibilityHitTest);
        addMethod (@selector (accessibilityFocusedUIElement), getAccessibilityFocusedUIElement);

        // deprecated methods required for backwards compatibility
        addMethod (@selector (accessibilityIsIgnored),        getAccessibilityIsIgnored);
        addMethod (@selector (accessibilityAttributeValue:),  getAccessibilityAttributeValue);

        addMethod (@selector (isFlipped),                     isFlipped);

        addMethod (NSViewComponentPeer::dismissModalsSelector,  dismissModals);
        addMethod (NSViewComponentPeer::asyncMouseDownSelector, asyncMouseDown);
        addMethod (NSViewComponentPeer::asyncMouseUpSelector,   asyncMouseUp);
        addMethod (NSViewComponentPeer::frameChangedSelector,   frameChanged);
        addMethod (NSViewComponentPeer::becomeKeySelector,      becomeKey);
        addMethod (NSViewComponentPeer::resignKeySelector,      resignKey);

        addMethod (@selector (performKeyEquivalent:),           performKeyEquivalent);

        addProtocol (@protocol (NSTextInput));

        registerClass();
    }

private:
    static void mouseDown (id self, SEL s, NSEvent* ev)
    {
        if (JUCEApplicationBase::isStandaloneApp())
        {
            asyncMouseDown (self, s, ev);
        }
        else
        {
            // In some host situations, the host will stop modal loops from working
            // correctly if they're called from a mouse event, so we'll trigger
            // the event asynchronously..
            [self performSelectorOnMainThread: NSViewComponentPeer::asyncMouseDownSelector
                                   withObject: ev
                                waitUntilDone: NO];
        }
    }

    static void mouseUp (id self, SEL s, NSEvent* ev)
    {
        if (JUCEApplicationBase::isStandaloneApp())
        {
            asyncMouseUp (self, s, ev);
        }
        else
        {
            // In some host situations, the host will stop modal loops from working
            // correctly if they're called from a mouse event, so we'll trigger
            // the event asynchronously..
            [self performSelectorOnMainThread: NSViewComponentPeer::asyncMouseUpSelector
                                   withObject: ev
                                waitUntilDone: NO];
        }
    }

    static void asyncMouseDown   (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMouseDown,        ev); }
    static void asyncMouseUp     (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMouseUp,          ev); }
    static void mouseDragged     (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMouseDrag,        ev); }
    static void mouseMoved       (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMouseMove,        ev); }
    static void mouseEntered     (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMouseEnter,       ev); }
    static void mouseExited      (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMouseExit,        ev); }
    static void scrollWheel      (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMouseWheel,       ev); }
    static void magnify          (id self, SEL, NSEvent* ev)   { callOnOwner (self, &NSViewComponentPeer::redirectMagnify,          ev); }
    static void copy             (id self, SEL, NSObject* s)   { callOnOwner (self, &NSViewComponentPeer::redirectCopy,             s);  }
    static void paste            (id self, SEL, NSObject* s)   { callOnOwner (self, &NSViewComponentPeer::redirectPaste,            s);  }
    static void cut              (id self, SEL, NSObject* s)   { callOnOwner (self, &NSViewComponentPeer::redirectCut,              s);  }
    static void selectAll        (id self, SEL, NSObject* s)   { callOnOwner (self, &NSViewComponentPeer::redirectSelectAll,        s);  }
    static void willMoveToWindow (id self, SEL, NSWindow* w)   { callOnOwner (self, &NSViewComponentPeer::redirectWillMoveToWindow, w);  }

    static BOOL acceptsFirstMouse (id, SEL, NSEvent*)          { return YES; }
    static BOOL wantsDefaultClipping (id, SEL)                 { return YES; } // (this is the default, but may want to customise it in future)
    static BOOL worksWhenModal (id self, SEL)                  { if (auto* p = getOwner (self)) return p->worksWhenModal(); return NO; }

    static void drawRect (id self, SEL, NSRect r)              { callOnOwner (self, &NSViewComponentPeer::drawRect, r); }
    static void frameChanged (id self, SEL, NSNotification*)   { callOnOwner (self, &NSViewComponentPeer::redirectMovedOrResized); }
    static void viewDidMoveToWindow (id self, SEL)             { callOnOwner (self, &NSViewComponentPeer::viewMovedToWindow); }
    static void dismissModals (id self, SEL)                   { callOnOwner (self, &NSViewComponentPeer::dismissModals); }
    static void becomeKey (id self, SEL)                       { callOnOwner (self, &NSViewComponentPeer::becomeKey); }
    static void resignKey (id self, SEL)                       { callOnOwner (self, &NSViewComponentPeer::resignKey); }

    static BOOL isFlipped (id, SEL)                            { return true; }

    static void viewWillDraw (id self, SEL)
    {
        // Without setting contentsFormat macOS Big Sur will always set the invalid area
        // to be the entire frame.
       #if defined (MAC_OS_X_VERSION_10_12) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
        if (@available (macOS 10.12, *))
        {
            CALayer* layer = ((NSView*) self).layer;
            layer.contentsFormat = kCAContentsFormatRGBA8Uint;
        }
       #endif

        sendSuperclassMessage<void> (self, @selector (viewWillDraw));
    }

    static void windowWillMiniaturize (id self, SEL, NSNotification*)
    {
        if (auto* p = getOwner (self))
        {
            if (p->isAlwaysOnTop)
            {
                // there is a bug when restoring minimised always on top windows so we need
                // to remove this behaviour before minimising and restore it afterwards
                p->setAlwaysOnTop (false);
                p->wasAlwaysOnTop = true;
            }
        }
    }

    static void windowDidDeminiaturize (id self, SEL, NSNotification*)
    {
        if (auto* p = getOwner (self))
        {
            if (p->wasAlwaysOnTop)
                p->setAlwaysOnTop (true);

            p->redirectMovedOrResized();
        }
    }

    static void windowDidChangeScreen (id self, SEL, NSNotification*)
    {
        if (auto* p = getOwner (self))
            p->windowDidChangeScreen();
    }

    static BOOL isOpaque (id self, SEL)
    {
        auto* owner = getOwner (self);
        return owner == nullptr || owner->getComponent().isOpaque();
    }

    //==============================================================================
    static void keyDown (id self, SEL, NSEvent* ev)
    {
        if (auto* owner = getOwner (self))
        {
            auto* target = owner->findCurrentTextInputTarget();
            owner->textWasInserted = false;

            if (target != nullptr)
                [(NSView*) self interpretKeyEvents: [NSArray arrayWithObject: ev]];
            else
                owner->stringBeingComposed.clear();

            if (! (owner->textWasInserted || owner->redirectKeyDown (ev)))
                sendSuperclassMessage<void> (self, @selector (keyDown:), ev);
        }
    }

    static void keyUp (id self, SEL, NSEvent* ev)
    {
        auto* owner = getOwner (self);

        if (! owner->redirectKeyUp (ev))
            sendSuperclassMessage<void> (self, @selector (keyUp:), ev);
    }

    //==============================================================================
    static void insertText (id self, SEL, id aString)
    {
        // This commits multi-byte text when return is pressed, or after every keypress for western keyboards
        if (auto* owner = getOwner (self))
        {
            NSString* newText = [aString isKindOfClass: [NSAttributedString class]] ? [aString string] : aString;

            if ([newText length] > 0)
            {
                if (auto* target = owner->findCurrentTextInputTarget())
                {
                    target->insertTextAtCaret (nsStringToJuce (newText));
                    owner->textWasInserted = true;
                }
            }

            owner->stringBeingComposed.clear();
        }
    }

    static void doCommandBySelector (id, SEL, SEL) {}

    static void setMarkedText (id self, SEL, id aString, NSRange)
    {
        if (auto* owner = getOwner (self))
        {
            owner->stringBeingComposed = nsStringToJuce ([aString isKindOfClass: [NSAttributedString class]]
                                                           ? [aString string] : aString);

            if (auto* target = owner->findCurrentTextInputTarget())
            {
                auto currentHighlight = target->getHighlightedRegion();
                target->insertTextAtCaret (owner->stringBeingComposed);
                target->setHighlightedRegion (currentHighlight.withLength (owner->stringBeingComposed.length()));
                owner->textWasInserted = true;
            }
        }
    }

    static void unmarkText (id self, SEL)
    {
        if (auto* owner = getOwner (self))
        {
            if (owner->stringBeingComposed.isNotEmpty())
            {
                if (auto* target = owner->findCurrentTextInputTarget())
                {
                    target->insertTextAtCaret (owner->stringBeingComposed);
                    owner->textWasInserted = true;
                }

                owner->stringBeingComposed.clear();
            }
        }
    }

    static BOOL hasMarkedText (id self, SEL)
    {
        auto* owner = getOwner (self);
        return owner != nullptr && owner->stringBeingComposed.isNotEmpty();
    }

    static long conversationIdentifier (id self, SEL)
    {
        return (long) (pointer_sized_int) self;
    }

    static NSAttributedString* attributedSubstringFromRange (id self, SEL, NSRange theRange)
    {
        if (auto* owner = getOwner (self))
        {
            if (auto* target = owner->findCurrentTextInputTarget())
            {
                Range<int> r ((int) theRange.location,
                              (int) (theRange.location + theRange.length));

                return [[[NSAttributedString alloc] initWithString: juceStringToNS (target->getTextInRange (r))] autorelease];
            }
        }

        return nil;
    }

    static NSRange markedRange (id self, SEL)
    {
        if (auto* owner = getOwner (self))
            if (owner->stringBeingComposed.isNotEmpty())
                return NSMakeRange (0, (NSUInteger) owner->stringBeingComposed.length());

        return NSMakeRange (NSNotFound, 0);
    }

    static NSRange selectedRange (id self, SEL)
    {
        if (auto* owner = getOwner (self))
        {
            if (auto* target = owner->findCurrentTextInputTarget())
            {
                auto highlight = target->getHighlightedRegion();

                if (! highlight.isEmpty())
                    return NSMakeRange ((NSUInteger) highlight.getStart(),
                                        (NSUInteger) highlight.getLength());
            }
        }

        return NSMakeRange (NSNotFound, 0);
    }

    static NSRect firstRectForCharacterRange (id self, SEL, NSRange)
    {
        if (auto* owner = getOwner (self))
            if (auto* comp = dynamic_cast<Component*> (owner->findCurrentTextInputTarget()))
                return flippedScreenRect (makeNSRect (comp->getScreenBounds()));

        return NSZeroRect;
    }

    static NSUInteger characterIndexForPoint (id, SEL, NSPoint)     { return NSNotFound; }
    static NSArray* validAttributesForMarkedText (id, SEL)          { return [NSArray array]; }

    //==============================================================================
    static void flagsChanged (id self, SEL, NSEvent* ev)
    {
        callOnOwner (self, &NSViewComponentPeer::redirectModKeyChange, ev);
    }

    static BOOL becomeFirstResponder (id self, SEL)
    {
        callOnOwner (self, &NSViewComponentPeer::viewFocusGain);
        return YES;
    }

    static BOOL resignFirstResponder (id self, SEL)
    {
        callOnOwner (self, &NSViewComponentPeer::viewFocusLoss);
        return YES;
    }

    static BOOL acceptsFirstResponder (id self, SEL)
    {
        auto* owner = getOwner (self);
        return owner != nullptr && owner->canBecomeKeyWindow();
    }

    //==============================================================================
    static NSDragOperation draggingEntered (id self, SEL s, id<NSDraggingInfo> sender)
    {
        return draggingUpdated (self, s, sender);
    }

    static NSDragOperation draggingUpdated (id self, SEL, id<NSDraggingInfo> sender)
    {
        if (auto* owner = getOwner (self))
            if (owner->sendDragCallback (&NSViewComponentPeer::handleDragMove, sender))
                return NSDragOperationGeneric;

        return NSDragOperationNone;
    }

    static void draggingEnded (id self, SEL s, id<NSDraggingInfo> sender)
    {
        draggingExited (self, s, sender);
    }

    static void draggingExited (id self, SEL, id<NSDraggingInfo> sender)
    {
        callOnOwner (self, &NSViewComponentPeer::sendDragCallback, &NSViewComponentPeer::handleDragExit, sender);
    }

    static BOOL prepareForDragOperation (id, SEL, id<NSDraggingInfo>)
    {
        return YES;
    }

    static BOOL performDragOperation (id self, SEL, id<NSDraggingInfo> sender)
    {
        auto* owner = getOwner (self);
        return owner != nullptr && owner->sendDragCallback (&NSViewComponentPeer::handleDragDrop, sender);
    }

    static void concludeDragOperation (id, SEL, id<NSDraggingInfo>) {}

    //==============================================================================
    static BOOL getIsAccessibilityElement (id, SEL)
    {
        return NO;
    }

    static NSArray* getAccessibilityChildren (id self, SEL)
    {
        return NSAccessibilityUnignoredChildrenForOnlyChild (getAccessibleChild (self));
    }

    static id accessibilityHitTest (id self, SEL, NSPoint point)
    {
        return [getAccessibleChild (self) accessibilityHitTest: point];
    }

    static id getAccessibilityFocusedUIElement (id self, SEL)
    {
        return [getAccessibleChild (self) accessibilityFocusedUIElement];
    }

    static BOOL getAccessibilityIsIgnored (id, SEL)
    {
        return YES;
    }

    static id getAccessibilityAttributeValue (id self, SEL, NSString* attribute)
    {
        if ([attribute isEqualToString: NSAccessibilityChildrenAttribute])
            return getAccessibilityChildren (self, {});

        return sendSuperclassMessage<id> (self, @selector (accessibilityAttributeValue:), attribute);
    }

    static bool tryPassingKeyEventToPeer (NSEvent* e)
    {
        if ([e type] != NSEventTypeKeyDown && [e type] != NSEventTypeKeyUp)
            return false;

        if (auto* focused = Component::getCurrentlyFocusedComponent())
        {
            if (auto* peer = dynamic_cast<NSViewComponentPeer*> (focused->getPeer()))
            {
                return [e type] == NSEventTypeKeyDown ? peer->redirectKeyDown (e)
                                                      : peer->redirectKeyUp (e);
            }
        }

        return false;
    }

    static BOOL performKeyEquivalent (id self, SEL s, NSEvent* event)
    {
        // We try passing shortcut keys to the currently focused component first.
        // If the component doesn't want the event, we'll fall back to the superclass
        // implementation, which will pass the event to the main menu.
        if (tryPassingKeyEventToPeer (event))
            return YES;

        return sendSuperclassMessage<BOOL> (self, s, event);
    }

    template <typename Func, typename... Args>
    static void callOnOwner (id self, Func&& func, Args&&... args)
    {
        if (auto* owner = getOwner (self))
            (owner->*func) (std::forward<Args> (args)...);
    }
};

//==============================================================================
struct JuceNSWindowClass   : public NSViewComponentPeerWrapper<ObjCClass<NSWindow>>
{
    JuceNSWindowClass()  : NSViewComponentPeerWrapper ("JUCEWindow_")
    {
        addMethod (@selector (canBecomeKeyWindow),                  canBecomeKeyWindow);
        addMethod (@selector (canBecomeMainWindow),                 canBecomeMainWindow);
        addMethod (@selector (becomeKeyWindow),                     becomeKeyWindow);
        addMethod (@selector (resignKeyWindow),                     resignKeyWindow);
        addMethod (@selector (windowShouldClose:),                  windowShouldClose);
        addMethod (@selector (constrainFrameRect:toScreen:),        constrainFrameRect);
        addMethod (@selector (windowWillResize:toSize:),            windowWillResize);
        addMethod (@selector (windowDidExitFullScreen:),            windowDidExitFullScreen);
        addMethod (@selector (windowWillEnterFullScreen:),          windowWillEnterFullScreen);
        addMethod (@selector (windowWillExitFullScreen:),           windowWillExitFullScreen);
        addMethod (@selector (windowWillStartLiveResize:),          windowWillStartLiveResize);
        addMethod (@selector (windowDidEndLiveResize:),             windowDidEndLiveResize);
        addMethod (@selector (window:shouldPopUpDocumentPathMenu:), shouldPopUpPathMenu);
        addMethod (@selector (isFlipped),                           isFlipped);
        addMethod (@selector (windowWillUseStandardFrame:defaultFrame:), windowWillUseStandardFrame);
        addMethod (@selector (windowShouldZoom:toFrame:),                windowShouldZoomToFrame);

        addMethod (@selector (accessibilityTitle),                  getAccessibilityTitle);
        addMethod (@selector (accessibilityLabel),                  getAccessibilityLabel);
        addMethod (@selector (accessibilityTopLevelUIElement),      getAccessibilityWindow);
        addMethod (@selector (accessibilityWindow),                 getAccessibilityWindow);
        addMethod (@selector (accessibilityRole),                   getAccessibilityRole);
        addMethod (@selector (accessibilitySubrole),                getAccessibilitySubrole);

        addMethod (@selector (keyDown:),                            keyDown);

        addMethod (@selector (window:shouldDragDocumentWithEvent:from:withPasteboard:), shouldAllowIconDrag);

        addMethod (@selector (toggleFullScreen:),                                         toggleFullScreen);

        addProtocol (@protocol (NSWindowDelegate));

        registerClass();
    }

private:
    //==============================================================================
    static BOOL isFlipped (id, SEL) { return true; }

    // Key events will be processed by the peer's component.
    // If the component is unable to use the event, it will be re-sent
    // to performKeyEquivalent.
    // performKeyEquivalent will send the event to the view's superclass,
    // which will try passing the event to the main menu.
    // If the event still hasn't been processed, it will be passed to the
    // next responder in the chain, which will be the NSWindow for a peer
    // that is on the desktop.
    // If the NSWindow still doesn't handle the event, the Apple docs imply
    // that the event should be sent to the NSApp for processing, but this
    // doesn't seem to happen for keyDown events.
    // Instead, the NSWindow attempts to process the event, fails, and
    // triggers an annoying NSBeep.
    // Overriding keyDown to "handle" the event seems to suppress the beep.
    static void keyDown (id, SEL, NSEvent* ev)
    {
        ignoreUnused (ev);

       #if JUCE_DEBUG_UNHANDLED_KEYPRESSES
        DBG ("unhandled key down event with keycode: " << [ev keyCode]);
       #endif
    }

    static NSRect windowWillUseStandardFrame (id self, SEL, NSWindow* window, NSRect r)
    {
        if (auto* owner = getOwner (self))
        {
            if (auto* constrainer = owner->getConstrainer())
            {
                if (auto* screen = [window screen])
                {
                    const auto safeScreenBounds = convertToRectFloat (flippedScreenRect (owner->hasNativeTitleBar() ? r : [screen visibleFrame]));
                    const auto originalBounds = owner->getFrameSize().addedTo (owner->getComponent().getScreenBounds()).toFloat();
                    const auto expanded = originalBounds.withWidth  ((float) constrainer->getMaximumWidth())
                                                        .withHeight ((float) constrainer->getMaximumHeight());
                    const auto constrained = expanded.constrainedWithin (safeScreenBounds);

                    return flippedScreenRect (makeNSRect ([&]
                    {
                        if (constrained == owner->getBounds().toFloat())
                            return owner->lastSizeBeforeZoom.toFloat();

                        owner->lastSizeBeforeZoom = owner->getBounds().toFloat();
                        return constrained;
                    }()));
                }
            }
        }

        return r;
    }

    static BOOL windowShouldZoomToFrame (id self, SEL, NSWindow*, NSRect)
    {
        if (auto* owner = getOwner (self))
            if (owner->hasNativeTitleBar() && (owner->getStyleFlags() & ComponentPeer::windowIsResizable) == 0)
                return NO;

        return YES;
    }

    static BOOL canBecomeKeyWindow (id self, SEL)
    {
        auto* owner = getOwner (self);

        return owner != nullptr
                && owner->canBecomeKeyWindow()
                && ! owner->isBlockedByModalComponent();
    }

    static BOOL canBecomeMainWindow (id self, SEL)
    {
        auto* owner = getOwner (self);

        return owner != nullptr
                && owner->canBecomeMainWindow()
                && ! owner->isBlockedByModalComponent();
    }

    static void becomeKeyWindow (id self, SEL)
    {
        sendSuperclassMessage<void> (self, @selector (becomeKeyWindow));

        if (auto* owner = getOwner (self))
        {
            if (owner->canBecomeKeyWindow())
            {
                owner->becomeKeyWindow();
                return;
            }

            // this fixes a bug causing hidden windows to sometimes become visible when the app regains focus
            if (! owner->getComponent().isVisible())
                [(NSWindow*) self orderOut: nil];
        }
    }

    static void resignKeyWindow (id self, SEL)
    {
        sendSuperclassMessage<void> (self, @selector (resignKeyWindow));

        if (auto* owner = getOwner (self))
            owner->resignKeyWindow();
    }

    static BOOL windowShouldClose (id self, SEL, id /*window*/)
    {
        auto* owner = getOwner (self);
        return owner == nullptr || owner->windowShouldClose();
    }

    static NSRect constrainFrameRect (id self, SEL, NSRect frameRect, NSScreen* screen)
    {
        if (auto* owner = getOwner (self))
        {
            frameRect = sendSuperclassMessage<NSRect, NSRect, NSScreen*> (self, @selector (constrainFrameRect:toScreen:),
                                                                          frameRect, screen);

            frameRect = owner->constrainRect (frameRect);
        }

        return frameRect;
    }

    static NSSize windowWillResize (id self, SEL, NSWindow*, NSSize proposedFrameSize)
    {
        auto* owner = getOwner (self);

        if (owner == nullptr || owner->isZooming)
            return proposedFrameSize;

        NSRect frameRect = flippedScreenRect ([(NSWindow*) self frame]);
        frameRect.size = proposedFrameSize;

        frameRect = owner->constrainRect (flippedScreenRect (frameRect));

        owner->dismissModals();

        return frameRect.size;
    }

    static void toggleFullScreen (id self, SEL name, id sender)
    {
        if (auto* owner = getOwner (self))
        {
            const auto isFullScreen = owner->isFullScreen();

            if (! isFullScreen)
                owner->lastSizeBeforeZoom = owner->getBounds().toFloat();

            sendSuperclassMessage<void> (self, name, sender);

            if (isFullScreen)
            {
                [NSApp setPresentationOptions: NSApplicationPresentationDefault];
                owner->setBounds (owner->lastSizeBeforeZoom.toNearestInt(), false);
            }
        }
    }

    static void windowDidExitFullScreen (id self, SEL, NSNotification*)
    {
        if (auto* owner = getOwner (self))
            owner->resetWindowPresentation();
    }

    static void windowWillExitFullScreen (id self, SEL, NSNotification*)
    {
        // The exit-fullscreen animation looks bad on Monterey if the window isn't resizable...
        if (auto* owner = getOwner (self))
            if (auto* window = owner->window)
                [window setStyleMask: [window styleMask] | NSWindowStyleMaskResizable];
    }

    static void windowWillEnterFullScreen (id self, SEL, NSNotification*)
    {
        if (SystemStats::getOperatingSystemType() <= SystemStats::MacOSX_10_9)
            return;

        if (auto* owner = getOwner (self))
            if (owner->hasNativeTitleBar() && (owner->getStyleFlags() & ComponentPeer::windowIsResizable) == 0)
                [owner->window setStyleMask: NSWindowStyleMaskBorderless];
    }

    static void windowWillStartLiveResize (id self, SEL, NSNotification*)
    {
        if (auto* owner = getOwner (self))
            owner->liveResizingStart();
    }

    static void windowDidEndLiveResize (id self, SEL, NSNotification*)
    {
        if (auto* owner = getOwner (self))
            owner->liveResizingEnd();
    }

    static bool shouldPopUpPathMenu (id self, SEL, id /*window*/, NSMenu*)
    {
        if (auto* owner = getOwner (self))
            return owner->windowRepresentsFile;

        return false;
    }

    static bool shouldAllowIconDrag (id self, SEL, id /*window*/, NSEvent*, NSPoint, NSPasteboard*)
    {
        if (auto* owner = getOwner (self))
            return owner->windowRepresentsFile;

        return false;
    }

    static NSString* getAccessibilityTitle (id self, SEL)
    {
        return [self title];
    }

    static NSString* getAccessibilityLabel (id self, SEL)
    {
        return [getAccessibleChild (self) accessibilityLabel];
    }

    static id getAccessibilityWindow (id self, SEL)
    {
        return self;
    }

    static NSAccessibilityRole getAccessibilityRole (id, SEL)
    {
        return NSAccessibilityWindowRole;
    }

    static NSAccessibilityRole getAccessibilitySubrole (id self, SEL)
    {
        if (@available (macOS 10.10, *))
            return [getAccessibleChild (self) accessibilitySubrole];

        return nil;
    }
};

NSView* NSViewComponentPeer::createViewInstance()
{
    static JuceNSViewClass cls;
    return cls.createInstance();
}

NSWindow* NSViewComponentPeer::createWindowInstance()
{
    static JuceNSWindowClass cls;
    return cls.createInstance();
}


//==============================================================================
ComponentPeer* NSViewComponentPeer::currentlyFocusedPeer = nullptr;
Array<int> NSViewComponentPeer::keysCurrentlyDown;

//==============================================================================
bool KeyPress::isKeyCurrentlyDown (int keyCode)
{
    if (NSViewComponentPeer::keysCurrentlyDown.contains (keyCode))
        return true;

    if (keyCode >= 'A' && keyCode <= 'Z'
         && NSViewComponentPeer::keysCurrentlyDown.contains ((int) CharacterFunctions::toLowerCase ((juce_wchar) keyCode)))
        return true;

    if (keyCode >= 'a' && keyCode <= 'z'
         && NSViewComponentPeer::keysCurrentlyDown.contains ((int) CharacterFunctions::toUpperCase ((juce_wchar) keyCode)))
        return true;

    return false;
}

//==============================================================================
bool MouseInputSource::SourceList::addSource()
{
    if (sources.size() == 0)
    {
        addSource (0, MouseInputSource::InputSourceType::mouse);
        return true;
    }

    return false;
}

bool MouseInputSource::SourceList::canUseTouch()
{
    return false;
}

//==============================================================================
void Desktop::setKioskComponent (Component* kioskComp, bool shouldBeEnabled, bool allowMenusAndBars)
{
    auto* peer = dynamic_cast<NSViewComponentPeer*> (kioskComp->getPeer());
    jassert (peer != nullptr); // (this should have been checked by the caller)

    if (peer->hasNativeTitleBar())
    {
        if (shouldBeEnabled && ! allowMenusAndBars)
            [NSApp setPresentationOptions: NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar];
        else if (! shouldBeEnabled)
            [NSApp setPresentationOptions: NSApplicationPresentationDefault];

        peer->setFullScreen (true);
    }
    else
    {
        if (shouldBeEnabled)
        {
            [NSApp setPresentationOptions: (allowMenusAndBars ? (NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar)
                                                              : (NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar))];

            kioskComp->setBounds (getDisplays().getDisplayForRect (kioskComp->getScreenBounds())->totalArea);
            peer->becomeKeyWindow();
        }
        else
        {
            peer->resetWindowPresentation();
        }
    }
}

void Desktop::allowedOrientationsChanged() {}

//==============================================================================
ComponentPeer* Component::createNewPeer (int styleFlags, void* windowToAttachTo)
{
    return new NSViewComponentPeer (*this, styleFlags, (NSView*) windowToAttachTo);
}

//==============================================================================
const int KeyPress::spaceKey        = ' ';
const int KeyPress::returnKey       = 0x0d;
const int KeyPress::escapeKey       = 0x1b;
const int KeyPress::backspaceKey    = 0x7f;
const int KeyPress::leftKey         = NSLeftArrowFunctionKey;
const int KeyPress::rightKey        = NSRightArrowFunctionKey;
const int KeyPress::upKey           = NSUpArrowFunctionKey;
const int KeyPress::downKey         = NSDownArrowFunctionKey;
const int KeyPress::pageUpKey       = NSPageUpFunctionKey;
const int KeyPress::pageDownKey     = NSPageDownFunctionKey;
const int KeyPress::endKey          = NSEndFunctionKey;
const int KeyPress::homeKey         = NSHomeFunctionKey;
const int KeyPress::deleteKey       = NSDeleteFunctionKey;
const int KeyPress::insertKey       = -1;
const int KeyPress::tabKey          = 9;
const int KeyPress::F1Key           = NSF1FunctionKey;
const int KeyPress::F2Key           = NSF2FunctionKey;
const int KeyPress::F3Key           = NSF3FunctionKey;
const int KeyPress::F4Key           = NSF4FunctionKey;
const int KeyPress::F5Key           = NSF5FunctionKey;
const int KeyPress::F6Key           = NSF6FunctionKey;
const int KeyPress::F7Key           = NSF7FunctionKey;
const int KeyPress::F8Key           = NSF8FunctionKey;
const int KeyPress::F9Key           = NSF9FunctionKey;
const int KeyPress::F10Key          = NSF10FunctionKey;
const int KeyPress::F11Key          = NSF11FunctionKey;
const int KeyPress::F12Key          = NSF12FunctionKey;
const int KeyPress::F13Key          = NSF13FunctionKey;
const int KeyPress::F14Key          = NSF14FunctionKey;
const int KeyPress::F15Key          = NSF15FunctionKey;
const int KeyPress::F16Key          = NSF16FunctionKey;
const int KeyPress::F17Key          = NSF17FunctionKey;
const int KeyPress::F18Key          = NSF18FunctionKey;
const int KeyPress::F19Key          = NSF19FunctionKey;
const int KeyPress::F20Key          = NSF20FunctionKey;
const int KeyPress::F21Key          = NSF21FunctionKey;
const int KeyPress::F22Key          = NSF22FunctionKey;
const int KeyPress::F23Key          = NSF23FunctionKey;
const int KeyPress::F24Key          = NSF24FunctionKey;
const int KeyPress::F25Key          = NSF25FunctionKey;
const int KeyPress::F26Key          = NSF26FunctionKey;
const int KeyPress::F27Key          = NSF27FunctionKey;
const int KeyPress::F28Key          = NSF28FunctionKey;
const int KeyPress::F29Key          = NSF29FunctionKey;
const int KeyPress::F30Key          = NSF30FunctionKey;
const int KeyPress::F31Key          = NSF31FunctionKey;
const int KeyPress::F32Key          = NSF32FunctionKey;
const int KeyPress::F33Key          = NSF33FunctionKey;
const int KeyPress::F34Key          = NSF34FunctionKey;
const int KeyPress::F35Key          = NSF35FunctionKey;

const int KeyPress::numberPad0              = extendedKeyModifier + 0x20;
const int KeyPress::numberPad1              = extendedKeyModifier + 0x21;
const int KeyPress::numberPad2              = extendedKeyModifier + 0x22;
const int KeyPress::numberPad3              = extendedKeyModifier + 0x23;
const int KeyPress::numberPad4              = extendedKeyModifier + 0x24;
const int KeyPress::numberPad5              = extendedKeyModifier + 0x25;
const int KeyPress::numberPad6              = extendedKeyModifier + 0x26;
const int KeyPress::numberPad7              = extendedKeyModifier + 0x27;
const int KeyPress::numberPad8              = extendedKeyModifier + 0x28;
const int KeyPress::numberPad9              = extendedKeyModifier + 0x29;
const int KeyPress::numberPadAdd            = extendedKeyModifier + 0x2a;
const int KeyPress::numberPadSubtract       = extendedKeyModifier + 0x2b;
const int KeyPress::numberPadMultiply       = extendedKeyModifier + 0x2c;
const int KeyPress::numberPadDivide         = extendedKeyModifier + 0x2d;
const int KeyPress::numberPadSeparator      = extendedKeyModifier + 0x2e;
const int KeyPress::numberPadDecimalPoint   = extendedKeyModifier + 0x2f;
const int KeyPress::numberPadEquals         = extendedKeyModifier + 0x30;
const int KeyPress::numberPadDelete         = extendedKeyModifier + 0x31;
const int KeyPress::playKey                 = extendedKeyModifier + 0x00;
const int KeyPress::stopKey                 = extendedKeyModifier + 0x01;
const int KeyPress::fastForwardKey          = extendedKeyModifier + 0x02;
const int KeyPress::rewindKey               = extendedKeyModifier + 0x03;

} // namespace juce
