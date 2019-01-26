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
    typedef void (*AppFocusChangeCallback)();
    extern AppFocusChangeCallback appFocusChangeCallback;
    typedef bool (*CheckEventBlockedByModalComps) (NSEvent*);
    extern CheckEventBlockedByModalComps isEventBlockedByModalComps;
}

//==============================================================================
#if ! (defined (MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7)

@interface NSEvent (JuceDeviceDelta)
 - (CGFloat) scrollingDeltaX;
 - (CGFloat) scrollingDeltaY;
 - (BOOL) hasPreciseScrollingDeltas;
 - (BOOL) isDirectionInvertedFromDevice;
@end

#endif

namespace juce
{

//==============================================================================
static CGFloat getMainScreenHeight() noexcept
{
    return [[[NSScreen screens] objectAtIndex: 0] frame].size.height;
}

static void flipScreenRect (NSRect& r) noexcept
{
    r.origin.y = getMainScreenHeight() - (r.origin.y + r.size.height);
}

static NSRect flippedScreenRect (NSRect r) noexcept
{
    flipScreenRect (r);
    return r;
}

#if JUCE_MODULE_AVAILABLE_juce_opengl
void componentPeerAboutToChange (Component&, bool);
#endif

//==============================================================================
class NSViewComponentPeer  : public ComponentPeer,
                             private Timer
{
public:
    NSViewComponentPeer (Component& comp, const int windowStyleFlags, NSView* viewToAttachTo)
        : ComponentPeer (comp, windowStyleFlags),
          isSharedWindow (viewToAttachTo != nil),
          lastRepaintTime (Time::getMillisecondCounter())
    {
        appFocusChangeCallback = appFocusChanged;
        isEventBlockedByModalComps = checkEventBlockedByModalComps;

        NSRect r = makeNSRect (component.getLocalBounds());

        view = [createViewInstance() initWithFrame: r];
        setOwner (view, this);

        [view registerForDraggedTypes: getSupportedDragTypes()];

        notificationCenter = [NSNotificationCenter defaultCenter];

        [notificationCenter  addObserver: view
                                selector: @selector (frameChanged:)
                                    name: NSViewFrameDidChangeNotification
                                  object: view];

        if (! isSharedWindow)
        {
            [notificationCenter  addObserver: view
                                    selector: @selector (frameChanged:)
                                        name: NSWindowDidMoveNotification
                                      object: window];

            [notificationCenter  addObserver: view
                                    selector: @selector (frameChanged:)
                                        name: NSWindowDidMiniaturizeNotification
                                      object: window];

            [notificationCenter  addObserver: view
                                    selector: @selector (frameChanged:)
                                        name: NSWindowDidDeminiaturizeNotification
                                      object: window];
        }

        [view setPostsFrameChangedNotifications: YES];

        if (isSharedWindow)
        {
            window = [viewToAttachTo window];
            [viewToAttachTo addSubview: view];
        }
        else
        {
            r.origin.x = (CGFloat) component.getX();
            r.origin.y = (CGFloat) component.getY();
            flipScreenRect (r);

            window = [createWindowInstance() initWithContentRect: r
                                                       styleMask: getNSWindowStyleMask (windowStyleFlags)
                                                         backing: NSBackingStoreBuffered
                                                           defer: YES];
            setOwner (window, this);
            [window orderOut: nil];
           #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
            [window setDelegate: (id<NSWindowDelegate>) window];
           #else
            [window setDelegate: window];
           #endif
            [window setOpaque: component.isOpaque()];
            [window setHasShadow: ((windowStyleFlags & windowHasDropShadow) != 0)];

            if (component.isAlwaysOnTop())
                setAlwaysOnTop (true);

            [window setContentView: view];
            [window setAutodisplay: YES];
            [window setAcceptsMouseMovedEvents: YES];

            // We'll both retain and also release this on closing because plugin hosts can unexpectedly
            // close the window for us, and also tend to get cause trouble if setReleasedWhenClosed is NO.
            [window setReleasedWhenClosed: YES];
            [window retain];

            [window setExcludedFromWindowsMenu: (windowStyleFlags & windowIsTemporary) != 0];
            [window setIgnoresMouseEvents: (windowStyleFlags & windowIgnoresMouseClicks) != 0];

           #if defined (MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)
            if ((windowStyleFlags & (windowHasMaximiseButton | windowHasTitleBar)) == (windowHasMaximiseButton | windowHasTitleBar))
                [window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];

            if ([window respondsToSelector: @selector (setRestorable:)])
                [window setRestorable: NO];
           #endif
        }

        const float alpha = component.getAlpha();
        if (alpha < 1.0f)
            setAlpha (alpha);

        setTitle (component.getName());
    }

    ~NSViewComponentPeer()
    {
        [notificationCenter removeObserver: view];
        setOwner (view, nullptr);

        if ([view superview] != nil)
        {
            redirectWillMoveToWindow (nullptr);
            [view removeFromSuperview];
        }

        [view release];

        if (! isSharedWindow)
        {
            setOwner (window, nullptr);
            [window close];
            [window release];
        }
    }

    //==============================================================================
    void* getNativeHandle() const override    { return view; }

    void setVisible (bool shouldBeVisible) override
    {
        if (isSharedWindow)
        {
            [view setHidden: ! shouldBeVisible];
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
            [window setRepresentedFilename: juceStringToNS (file != File()
                                                                ? file.getFullPathName()
                                                                : String())];
    }

    void setBounds (const Rectangle<int>& newBounds, bool isNowFullScreen) override
    {
        fullScreen = isNowFullScreen;

        NSRect r = makeNSRect (newBounds);
        NSSize oldViewSize = [view frame].size;

        if (isSharedWindow)
        {
            r.origin.y = [[view superview] frame].size.height - (r.origin.y + r.size.height);
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
        NSRect r = [view frame];
        NSWindow* viewWindow = [view window];

        if (global && viewWindow != nil)
        {
            r = [[view superview] convertRect: r toView: nil];

           #if defined (MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
            r = [viewWindow convertRectToScreen: r];
           #else
            r.origin = [viewWindow convertBaseToScreen: r.origin];
           #endif

            flipScreenRect (r);
        }
        else
        {
            r.origin.y = [[view superview] frame].size.height - r.origin.y - r.size.height;
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

    Point<float> globalToLocal (Point<float> screenPosition) override
    {
        return screenPosition - getBounds (true).getPosition().toFloat();
    }

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

    void setFullScreen (bool shouldBeFullScreen) override
    {
        if (! isSharedWindow)
        {
            Rectangle<int> r (lastNonFullscreenBounds);

            if (isMinimised())
                setMinimised (false);

            if (fullScreen != shouldBeFullScreen)
            {
                if (shouldBeFullScreen && hasNativeTitleBar())
                {
                    fullScreen = true;
                    [window performZoom: nil];
                }
                else
                {
                    if (shouldBeFullScreen)
                        r = component.getParentMonitorArea();

                    // (can't call the component's setBounds method because that'll reset our fullscreen flag)
                    if (r != component.getBounds() && ! r.isEmpty())
                        setBounds (ScalingHelpers::scaledScreenPosToUnscaled (component, r), shouldBeFullScreen);
                }
            }
        }
    }

    bool isFullScreen() const override
    {
        return fullScreen;
    }

    bool isKioskMode() const override
    {
       #if defined (MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
        if (hasNativeTitleBar() && ([window styleMask] & NSWindowStyleMaskFullScreen) != 0)
            return true;
       #endif

        return ComponentPeer::isKioskMode();
    }

    static bool isWindowAtPoint (NSWindow* w, NSPoint screenPoint)
    {
       #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
        if ([NSWindow respondsToSelector: @selector (windowNumberAtPoint:belowWindowWithWindowNumber:)])
            return [NSWindow windowNumberAtPoint: screenPoint belowWindowWithWindowNumber: 0] == [w windowNumber];
       #endif

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
                const NSRect windowFrame = [viewWindow frame];
                const NSPoint windowPoint = [view convertPoint: NSMakePoint (localPos.x, viewFrame.size.height - localPos.y) toView: nil];
                const NSPoint screenPoint = NSMakePoint (windowFrame.origin.x + windowPoint.x,
                                                         windowFrame.origin.y + windowPoint.y);

                if (! isWindowAtPoint (viewWindow, screenPoint))
                    return false;

            }
        }

        NSView* v = [view hitTest: NSMakePoint (viewFrame.origin.x + localPos.getX(),
                                                viewFrame.origin.y + viewFrame.size.height - localPos.getY())];

        return trueIfInAChildWindow ? (v != nil)
                                    : (v == view);
    }

    BorderSize<int> getFrameSize() const override
    {
        BorderSize<int> b;

        if (! isSharedWindow)
        {
            NSRect v = [view convertRect: [view frame] toView: nil];
            NSRect w = [window frame];

            b.setTop ((int) (w.size.height - (v.origin.y + v.size.height)));
            b.setBottom ((int) v.origin.y);
            b.setLeft ((int) v.origin.x);
            b.setRight ((int) (w.size.width - (v.origin.x + v.size.width)));
        }

        return b;
    }

    void updateFullscreenStatus()
    {
        if (hasNativeTitleBar())
        {
            const Rectangle<int> screen (getFrameSize().subtractedFrom (component.getParentMonitorArea()));

            fullScreen = component.getScreenBounds().expanded (2, 2).contains (screen);
        }
    }

    bool hasNativeTitleBar() const
    {
        return (getStyleFlags() & windowHasTitleBar) != 0;
    }

    bool setAlwaysOnTop (bool alwaysOnTop) override
    {
        if (! isSharedWindow)
            [window setLevel: alwaysOnTop ? ((getStyleFlags() & windowIsTemporary) != 0 ? NSPopUpMenuWindowLevel
                                                                                        : NSFloatingWindowLevel)
                                          : NSNormalWindowLevel];

        return true;
    }

    void toFront (bool makeActiveWindow) override
    {
        if (isSharedWindow)
            [[view superview] addSubview: view
                              positioned: NSWindowAbove
                              relativeTo: nil];

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
                [[view superview] addSubview: view
                                  positioned: NSWindowBelow
                                  relativeTo: otherPeer->view];
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

    void setIcon (const Image&) override
    {
        // to do..
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

        currentModifiers = currentModifiers.withFlags (getModifierForButtonNumber ([ev buttonNumber]));
        sendMouseEvent (ev);
    }

    void redirectMouseUp (NSEvent* ev)
    {
        currentModifiers = currentModifiers.withoutFlags (getModifierForButtonNumber ([ev buttonNumber]));
        sendMouseEvent (ev);
        showArrowCursorIfNeeded();
    }

    void redirectMouseDrag (NSEvent* ev)
    {
        currentModifiers = currentModifiers.withFlags (getModifierForButtonNumber ([ev buttonNumber]));
        sendMouseEvent (ev);
    }

    void redirectMouseMove (NSEvent* ev)
    {
        currentModifiers = currentModifiers.withoutMouseButtons();

        NSPoint windowPos = [ev locationInWindow];

       #if defined (MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7
        NSPoint screenPos = [[ev window] convertRectToScreen: NSMakeRect (windowPos.x, windowPos.y, 1.0f, 1.0f)].origin;
       #else
        NSPoint screenPos = [[ev window] convertBaseToScreen: windowPos];
       #endif

        if (isWindowAtPoint ([ev window], screenPos))
            sendMouseEvent (ev);
        else
            // moved into another window which overlaps this one, so trigger an exit
            handleMouseEvent (MouseInputSource::InputSourceType::mouse, { -1.0f, -1.0f }, currentModifiers,
                              getMousePressure (ev), MouseInputSource::invalidOrientation, getMouseTime (ev));

        showArrowCursorIfNeeded();
    }

    void redirectMouseEnter (NSEvent* ev)
    {
        Desktop::getInstance().getMainMouseSource().forceMouseCursorUpdate();
        currentModifiers = currentModifiers.withoutMouseButtons();
        sendMouseEvent (ev);
    }

    void redirectMouseExit (NSEvent* ev)
    {
        currentModifiers = currentModifiers.withoutMouseButtons();
        sendMouseEvent (ev);
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
           #if defined (MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
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
            else
           #endif
            if ([ev respondsToSelector: @selector (deviceDeltaX)])
            {
                wheel.deltaX = checkDeviceDeltaReturnValue ((float) getMsgSendFPRetFn() (ev, @selector (deviceDeltaX)));
                wheel.deltaY = checkDeviceDeltaReturnValue ((float) getMsgSendFPRetFn() (ev, @selector (deviceDeltaY)));
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
       #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
        const float invScale = 1.0f - (float) [ev magnification];

        if (invScale > 0.0f)
            handleMagnifyGesture (MouseInputSource::InputSourceType::mouse, getMousePos (ev, view), getMouseTime (ev), 1.0f / invScale);
       #endif
        ignoreUnused (ev);
    }

    void redirectCopy  (NSObject*) { handleKeyPress (KeyPress ('c', ModifierKeys (ModifierKeys::commandModifier), 'c')); }
    void redirectPaste (NSObject*) { handleKeyPress (KeyPress ('v', ModifierKeys (ModifierKeys::commandModifier), 'v')); }
    void redirectCut   (NSObject*) { handleKeyPress (KeyPress ('x', ModifierKeys (ModifierKeys::commandModifier), 'x')); }

    void redirectWillMoveToWindow (NSWindow* newWindow)
    {
       #if JUCE_MODULE_AVAILABLE_juce_opengl
        if ([view window] == window)
            componentPeerAboutToChange (getComponent(), newWindow == nullptr);
       #else
        ignoreUnused (newWindow);
       #endif
    }

    void sendMouseEvent (NSEvent* ev)
    {
        updateModifiers (ev);
        handleMouseEvent (MouseInputSource::InputSourceType::mouse, getMousePos (ev, view), currentModifiers,
                          getMousePressure (ev), MouseInputSource::invalidOrientation, getMouseTime (ev));
    }

    bool handleKeyEvent (NSEvent* ev, bool isKeyDown)
    {
        const String unicode (nsStringToJuce ([ev characters]));
        const int keyCode = getKeyCodeFromEvent (ev);

       #if JUCE_DEBUG_KEYCODES
        DBG ("unicode: " + unicode + " " + String::toHexString ((int) unicode[0]));
        String unmodified (nsStringToJuce ([ev charactersIgnoringModifiers]));
        DBG ("unmodified: " + unmodified + " " + String::toHexString ((int) unmodified[0]));
       #endif

        if (keyCode != 0 || unicode.isNotEmpty())
        {
            if (isKeyDown)
            {
                bool used = false;

                for (String::CharPointerType u (unicode.getCharPointer()); ! u.isEmpty();)
                {
                    juce_wchar textCharacter = u.getAndAdvance();

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
        const NSObjectRetainer<NSEvent> r (ev);

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
        const NSObjectRetainer<NSEvent> r (ev);

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

        CGContextRef cg = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];

        if (! component.isOpaque())
            CGContextClearRect (cg, CGContextGetClipBoundingBox (cg));

        float displayScale = 1.0f;

       #if defined (MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)
        NSScreen* screen = [[view window] screen];
        if ([screen respondsToSelector: @selector (backingScaleFactor)])
            displayScale = (float) screen.backingScaleFactor;
       #endif

       #if USE_COREGRAPHICS_RENDERING && JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS
        // This option invokes a separate paint call for each rectangle of the clip region.
        // It's a long story, but this is a basically a workaround for a CGContext not having
        // a way of finding whether a rectangle falls within its clip region
        if (usingCoreGraphics)
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
                    drawRect (cg, rect, displayScale);
                    CGContextRestoreGState (cg);
                }

                return;
            }
        }
       #endif

        drawRect (cg, r, displayScale);
    }

    void drawRect (CGContextRef cg, NSRect r, float displayScale)
    {
       #if USE_COREGRAPHICS_RENDERING
        if (usingCoreGraphics)
        {
            CoreGraphicsContext context (cg, (float) [view frame].size.height, displayScale);
            invokePaint (context);
        }
        else
       #endif
        {
            const Point<int> offset (-roundToInt (r.origin.x),
                                     -roundToInt ([view frame].size.height - (r.origin.y + r.size.height)));
            const int clipW = (int) (r.size.width  + 0.5f);
            const int clipH = (int) (r.size.height + 0.5f);

            RectangleList<int> clip;
            getClipRects (clip, offset, clipW, clipH);

            if (! clip.isEmpty())
            {
                Image temp (component.isOpaque() ? Image::RGB : Image::ARGB,
                            roundToInt (clipW * displayScale),
                            roundToInt (clipH * displayScale),
                            ! component.isOpaque());

                {
                    const int intScale = roundToInt (displayScale);
                    if (intScale != 1)
                        clip.scaleAll (intScale);

                    ScopedPointer<LowLevelGraphicsContext> context (component.getLookAndFeel()
                                                                      .createGraphicsContext (temp, offset * intScale, clip));

                    if (intScale != 1)
                        context->addTransform (AffineTransform::scale (displayScale));

                    invokePaint (*context);
                }

                CGColorSpaceRef colourSpace = CGColorSpaceCreateDeviceRGB();
                CGImageRef image = juce_createCoreGraphicsImage (temp, colourSpace, false);
                CGColorSpaceRelease (colourSpace);
                CGContextDrawImage (cg, CGRectMake (r.origin.x, r.origin.y, clipW, clipH), image);
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
        deferredRepaints.add ((float) area.getX(), (float) ([view frame].size.height - area.getBottom()),
                              (float) area.getWidth(), (float) area.getHeight());

        if (isTimerRunning())
            return;

        const uint32 now = Time::getMillisecondCounter();
        uint32 msSinceLastRepaint = (lastRepaintTime >= now) ? now - lastRepaintTime
                                                             : (std::numeric_limits<uint32>::max() - lastRepaintTime) + now;

        static uint32 minimumRepaintInterval = 1000 / 30; // 30fps

        // When windows are being resized, artificially throttling high-frequency repaints helps
        // to stop the event queue getting clogged, and keeps everything working smoothly.
        // For some reason Logic also needs this throttling to recored parameter events correctly.
        if (msSinceLastRepaint < minimumRepaintInterval && shouldThrottleRepaint())
        {
            startTimer (static_cast<int> (minimumRepaintInterval - msSinceLastRepaint));
            return;
        }

        setNeedsDisplayRectangles();
    }

    static bool shouldThrottleRepaint()
    {
        return areAnyWindowsInLiveResize() || ! JUCEApplication::isStandaloneApp();
    }

    void timerCallback() override
    {
        setNeedsDisplayRectangles();
        stopTimer();
    }

    void setNeedsDisplayRectangles()
    {
        for (auto& i : deferredRepaints)
            [view setNeedsDisplayInRect: makeNSRect (i)];

        lastRepaintTime = Time::getMillisecondCounter();
        deferredRepaints.clear();
    }

    void invokePaint (LowLevelGraphicsContext& context)
    {
        handlePaint (context);
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
    bool sendModalInputAttemptIfBlocked()
    {
        if (auto* modal = Component::getCurrentlyModalComponent())
        {
            if (insideToFrontCall == 0
                 && (! getComponent().isParentOf (modal))
                 && getComponent().isCurrentlyBlockedByAnotherModalComponent())
            {
                modal->inputAttemptWhenModal();
                return true;
            }
        }

        return false;
    }

    bool canBecomeKeyWindow()
    {
        return (getStyleFlags() & juce::ComponentPeer::windowIgnoresKeyPresses) == 0;
    }

    bool canBecomeMainWindow()
    {
        return dynamic_cast<ResizableWindow*> (&component) != nullptr;
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

    bool windowShouldClose()
    {
        if (! isValidPeer (this))
            return YES;

        handleUserClosingWindow();
        return NO;
    }

    void redirectMovedOrResized()
    {
        updateFullscreenStatus();
        handleMovedOrResized();
    }

    void viewMovedToWindow()
    {
        if (isSharedWindow)
            window = [view window];
    }

    void liveResizingStart()
    {
        if (constrainer != nullptr)
        {
            constrainer->resizeStart();
            isFirstLiveResize = true;
        }
    }

    void liveResizingEnd()
    {
        if (constrainer != nullptr)
            constrainer->resizeEnd();
    }

    NSRect constrainRect (NSRect r)
    {
        if (constrainer != nullptr && ! isKioskMode())
        {
            const float scale = getComponent().getDesktopScaleFactor();

            auto pos      = ScalingHelpers::unscaledScreenPosToScaled (scale, convertToRectInt (flippedScreenRect (r)));
            auto original = ScalingHelpers::unscaledScreenPosToScaled (scale, convertToRectInt (flippedScreenRect ([window frame])));

            const Rectangle<int> screenBounds (Desktop::getInstance().getDisplays().getTotalBounds (true));

           #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
            const bool inLiveResize = [window inLiveResize];
           #else
            const bool inLiveResize = [window respondsToSelector: @selector (inLiveResize)]
                                   && [window performSelector: @selector (inLiveResize)];
           #endif

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

            pos = ScalingHelpers::scaledScreenPosToUnscaled (scale, pos);
            r = flippedScreenRect (makeNSRect (pos));
        }

        return r;
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

        currentModifiers = currentModifiers.withOnlyMouseButtons().withFlags (m);
    }

    static void updateKeysDown (NSEvent* ev, bool isKeyDown)
    {
        updateModifiers (ev);
        int keyCode = getKeyCodeFromEvent (ev);

        if (keyCode != 0)
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
        String unmodified;

       #if JUCE_SUPPORT_CARBON
        if (TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource())
        {
            CFDataRef layoutData = (CFDataRef) TISGetInputSourceProperty (currentKeyboard,
                                                                          kTISPropertyUnicodeKeyLayoutData);

            if (layoutData != nullptr)
            {
                if (auto* layoutPtr = (const UCKeyboardLayout*) CFDataGetBytePtr (layoutData))
                {
                    UInt32 keysDown = 0;
                    UniChar buffer[4];
                    UniCharCount actual;

                    if (UCKeyTranslate (layoutPtr, [ev keyCode], kUCKeyActionDown, 0, LMGetKbdType(),
                                        kUCKeyTranslateNoDeadKeysBit, &keysDown, sizeof (buffer) / sizeof (UniChar),
                                        &actual, buffer) == 0)
                        unmodified = String (CharPointer_UTF16 (reinterpret_cast<CharPointer_UTF16::CharType*> (buffer)), 4);
                }
            }

            CFRelease (currentKeyboard);
        }

        // did the above layout conversion fail
        if (unmodified.isEmpty())
       #endif
        {
            unmodified = nsStringToJuce ([ev charactersIgnoringModifiers]);
        }

        int keyCode = unmodified[0];

        if (keyCode == 0x19) // (backwards-tab)
            keyCode = '\t';
        else if (keyCode == 0x03) // (enter)
            keyCode = '\r';
        else
            keyCode = (int) CharacterFunctions::toUpperCase ((juce_wchar) keyCode);

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
        return { (float) p.x, (float) ([view frame].size.height - p.y) };
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
        return [NSArray arrayWithObjects: NSFilenamesPboardType, NSFilesPromisePboardType, NSStringPboardType, nil];
    }

    BOOL sendDragCallback (const int type, id <NSDraggingInfo> sender)
    {
        NSPasteboard* pasteboard = [sender draggingPasteboard];
        NSString* contentType = [pasteboard availableTypeFromArray: getSupportedDragTypes()];

        if (contentType == nil)
            return false;

        NSPoint p = [view convertPoint: [sender draggingLocation] fromView: nil];
        ComponentPeer::DragInfo dragInfo;
        dragInfo.position.setXY ((int) p.x, (int) ([view frame].size.height - p.y));

        if (contentType == NSStringPboardType)
            dragInfo.text = nsStringToJuce ([pasteboard stringForType: NSStringPboardType]);
        else
            dragInfo.files = getDroppedFiles (pasteboard, contentType);

        if (! dragInfo.isEmpty())
        {
            switch (type)
            {
                case 0:   return handleDragMove (dragInfo);
                case 1:   return handleDragExit (dragInfo);
                case 2:   return handleDragDrop (dragInfo);
                default:  jassertfalse; break;
            }
        }

        return false;
    }

    StringArray getDroppedFiles (NSPasteboard* pasteboard, NSString* contentType)
    {
        StringArray files;
        NSString* iTunesPasteboardType = nsStringLiteral ("CorePasteboardFlavorType 0x6974756E"); // 'itun'

        if (contentType == NSFilesPromisePboardType
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
                    NSURL* url = [NSURL URLWithString: [track valueForKey: nsStringLiteral ("Location")]];

                    if ([url isFileURL])
                        files.add (nsStringToJuce ([url path]));
                }
            }
        }
        else
        {
            id list = [pasteboard propertyListForType: NSFilenamesPboardType];

            if ([list isKindOfClass: [NSArray class]])
            {
                NSArray* items = (NSArray*) [pasteboard propertyListForType: NSFilenamesPboardType];

                for (unsigned int i = 0; i < [items count]; ++i)
                    files.add (nsStringToJuce ((NSString*) [items objectAtIndex: i]));
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

    //==============================================================================
    NSWindow* window = nil;
    NSView* view = nil;
    bool isSharedWindow = false, fullScreen = false;
   #if USE_COREGRAPHICS_RENDERING
    bool usingCoreGraphics = true;
   #else
    bool usingCoreGraphics = false;
   #endif
    bool isZooming = false, isFirstLiveResize = false, textWasInserted = false;
    bool isStretchingTop = false, isStretchingLeft = false, isStretchingBottom = false, isStretchingRight = false;
    String stringBeingComposed;
    NSNotificationCenter* notificationCenter = nil;

    RectangleList<float> deferredRepaints;
    uint32 lastRepaintTime;

    static ModifierKeys currentModifiers;
    static ComponentPeer* currentlyFocusedPeer;
    static Array<int> keysCurrentlyDown;
    static int insideToFrontCall;

private:
    static NSView* createViewInstance();
    static NSWindow* createWindowInstance();

    static void setOwner (id viewOrWindow, NSViewComponentPeer* newOwner)
    {
        object_setInstanceVariable (viewOrWindow, "owner", newOwner);
    }

    void getClipRects (RectangleList<int>& clip, const Point<int> offset, const int clipW, const int clipH)
    {
        const NSRect* rects = nullptr;
        NSInteger numRects = 0;
        [view getRectsBeingDrawn: &rects count: &numRects];

        const Rectangle<int> clipBounds (clipW, clipH);
        const CGFloat viewH = [view frame].size.height;

        clip.ensureStorageAllocated ((int) numRects);

        for (int i = 0; i < numRects; ++i)
            clip.addWithoutMerging (clipBounds.getIntersection (Rectangle<int> (roundToInt (rects[i].origin.x) + offset.x,
                                                                                roundToInt (viewH - (rects[i].origin.y + rects[i].size.height)) + offset.y,
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

            if (Component* const modal = Component::getCurrentlyModalComponent())
                modal->inputAttemptWhenModal();
        }

        return true;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NSViewComponentPeer)
};

int NSViewComponentPeer::insideToFrontCall = 0;

//==============================================================================
struct JuceNSViewClass   : public ObjCClass<NSView>
{
    JuceNSViewClass()  : ObjCClass<NSView> ("JUCEView_")
    {
        addIvar<NSViewComponentPeer*> ("owner");

        addMethod (@selector (isOpaque),                      isOpaque,                   "c@:");
        addMethod (@selector (drawRect:),                     drawRect,                   "v@:", @encode (NSRect));
        addMethod (@selector (mouseDown:),                    mouseDown,                  "v@:@");
        addMethod (@selector (asyncMouseDown:),               asyncMouseDown,             "v@:@");
        addMethod (@selector (mouseUp:),                      mouseUp,                    "v@:@");
        addMethod (@selector (asyncMouseUp:),                 asyncMouseUp,               "v@:@");
        addMethod (@selector (mouseDragged:),                 mouseDragged,               "v@:@");
        addMethod (@selector (mouseMoved:),                   mouseMoved,                 "v@:@");
        addMethod (@selector (mouseEntered:),                 mouseEntered,               "v@:@");
        addMethod (@selector (mouseExited:),                  mouseExited,                "v@:@");
        addMethod (@selector (rightMouseDown:),               mouseDown,                  "v@:@");
        addMethod (@selector (rightMouseDragged:),            mouseDragged,               "v@:@");
        addMethod (@selector (rightMouseUp:),                 mouseUp,                    "v@:@");
        addMethod (@selector (otherMouseDown:),               mouseDown,                  "v@:@");
        addMethod (@selector (otherMouseDragged:),            mouseDragged,               "v@:@");
        addMethod (@selector (otherMouseUp:),                 mouseUp,                    "v@:@");
        addMethod (@selector (scrollWheel:),                  scrollWheel,                "v@:@");
        addMethod (@selector (magnifyWithEvent:),             magnify,                    "v@:@");
        addMethod (@selector (acceptsFirstMouse:),            acceptsFirstMouse,          "c@:@");
        addMethod (@selector (frameChanged:),                 frameChanged,               "v@:@");
        addMethod (@selector (wantsDefaultClipping:),         wantsDefaultClipping,       "c@:");
        addMethod (@selector (worksWhenModal),                worksWhenModal,             "c@:");
        addMethod (@selector (viewDidMoveToWindow),           viewDidMoveToWindow,        "v@:");
        addMethod (@selector (keyDown:),                      keyDown,                    "v@:@");
        addMethod (@selector (keyUp:),                        keyUp,                      "v@:@");
        addMethod (@selector (insertText:),                   insertText,                 "v@:@");
        addMethod (@selector (doCommandBySelector:),          doCommandBySelector,        "v@::");
        addMethod (@selector (setMarkedText:selectedRange:),  setMarkedText,              "v@:@", @encode (NSRange));
        addMethod (@selector (unmarkText),                    unmarkText,                 "v@:");
        addMethod (@selector (hasMarkedText),                 hasMarkedText,              "c@:");
        addMethod (@selector (conversationIdentifier),        conversationIdentifier,     "l@:");
        addMethod (@selector (attributedSubstringFromRange:), attributedSubstringFromRange, "@@:", @encode (NSRange));
        addMethod (@selector (markedRange),                   markedRange,                @encode (NSRange), "@:");
        addMethod (@selector (selectedRange),                 selectedRange,              @encode (NSRange), "@:");
        addMethod (@selector (firstRectForCharacterRange:),   firstRectForCharacterRange, @encode (NSRect), "@:", @encode (NSRange));
        addMethod (@selector (validAttributesForMarkedText),  validAttributesForMarkedText, "@@:");
        addMethod (@selector (flagsChanged:),                 flagsChanged,               "v@:@");

        addMethod (@selector (becomeFirstResponder),          becomeFirstResponder,       "c@:");
        addMethod (@selector (resignFirstResponder),          resignFirstResponder,       "c@:");
        addMethod (@selector (acceptsFirstResponder),         acceptsFirstResponder,      "c@:");

        addMethod (@selector (draggingEntered:),              draggingEntered,            @encode (NSDragOperation), "@:@");
        addMethod (@selector (draggingUpdated:),              draggingUpdated,            @encode (NSDragOperation), "@:@");
        addMethod (@selector (draggingEnded:),                draggingEnded,              "v@:@");
        addMethod (@selector (draggingExited:),               draggingExited,             "v@:@");
        addMethod (@selector (prepareForDragOperation:),      prepareForDragOperation,    "c@:@");
        addMethod (@selector (performDragOperation:),         performDragOperation,       "c@:@");
        addMethod (@selector (concludeDragOperation:),        concludeDragOperation,      "v@:@");

        addMethod (@selector (paste:),                        paste,                      "v@:@");
        addMethod (@selector (copy:),                         copy,                       "v@:@");
        addMethod (@selector (cut:),                          cut,                        "v@:@");

        addMethod (@selector (viewWillMoveToWindow:),         willMoveToWindow,           "v@:@");

        addProtocol (@protocol (NSTextInput));

        registerClass();
    }

private:
    static NSViewComponentPeer* getOwner (id self)
    {
        return getIvar<NSViewComponentPeer*> (self, "owner");
    }

    static void mouseDown (id self, SEL s, NSEvent* ev)
    {
        if (JUCEApplicationBase::isStandaloneApp())
            asyncMouseDown (self, s, ev);
        else
            // In some host situations, the host will stop modal loops from working
            // correctly if they're called from a mouse event, so we'll trigger
            // the event asynchronously..
            [self performSelectorOnMainThread: @selector (asyncMouseDown:)
                                   withObject: ev
                                waitUntilDone: NO];
    }

    static void mouseUp (id self, SEL s, NSEvent* ev)
    {
        if (JUCEApplicationBase::isStandaloneApp())
            asyncMouseUp (self, s, ev);
        else
            // In some host situations, the host will stop modal loops from working
            // correctly if they're called from a mouse event, so we'll trigger
            // the event asynchronously..
            [self performSelectorOnMainThread: @selector (asyncMouseUp:)
                                   withObject: ev
                                waitUntilDone: NO];
    }

    static void asyncMouseDown   (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMouseDown  (ev); }
    static void asyncMouseUp     (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMouseUp    (ev); }
    static void mouseDragged     (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMouseDrag  (ev); }
    static void mouseMoved       (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMouseMove  (ev); }
    static void mouseEntered     (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMouseEnter (ev); }
    static void mouseExited      (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMouseExit  (ev); }
    static void scrollWheel      (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMouseWheel (ev); }
    static void magnify          (id self, SEL, NSEvent* ev)   { if (auto* p = getOwner (self)) p->redirectMagnify    (ev); }
    static void copy             (id self, SEL, NSObject* s)   { if (auto* p = getOwner (self)) p->redirectCopy       (s);  }
    static void paste            (id self, SEL, NSObject* s)   { if (auto* p = getOwner (self)) p->redirectPaste      (s);  }
    static void cut              (id self, SEL, NSObject* s)   { if (auto* p = getOwner (self)) p->redirectCut        (s);  }
    static void willMoveToWindow (id self, SEL, NSWindow* w)   { if (auto* p = getOwner (self)) p->redirectWillMoveToWindow (w); }

    static BOOL acceptsFirstMouse (id, SEL, NSEvent*)          { return YES; }
    static BOOL wantsDefaultClipping (id, SEL)                 { return YES; } // (this is the default, but may want to customise it in future)
    static BOOL worksWhenModal (id self, SEL)                  { if (auto* p = getOwner (self)) return p->worksWhenModal(); return NO; }

    static void drawRect (id self, SEL, NSRect r)              { if (auto* p = getOwner (self)) p->drawRect (r); }
    static void frameChanged (id self, SEL, NSNotification*)   { if (auto* p = getOwner (self)) p->redirectMovedOrResized(); }
    static void viewDidMoveToWindow (id self, SEL)             { if (auto* p = getOwner (self)) p->viewMovedToWindow(); }

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
            {
                objc_super s = { self, [NSView class] };
                getMsgSendSuperFn() (&s, @selector (keyDown:), ev);
            }
        }
    }

    static void keyUp (id self, SEL, NSEvent* ev)
    {
        auto* owner = getOwner (self);

        if (owner == nullptr || ! owner->redirectKeyUp (ev))
        {
            objc_super s = { self, [NSView class] };
            getMsgSendSuperFn() (&s, @selector (keyUp:), ev);
        }
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
                const Range<int> r ((int) theRange.location,
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
        if (auto* owner = getOwner (self))
            owner->redirectModKeyChange (ev);
    }

    static BOOL becomeFirstResponder (id self, SEL)
    {
        if (auto* owner = getOwner (self))
            owner->viewFocusGain();

        return YES;
    }

    static BOOL resignFirstResponder (id self, SEL)
    {
        if (auto* owner = getOwner (self))
            owner->viewFocusLoss();

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
            if (owner->sendDragCallback (0, sender))
                return NSDragOperationCopy | NSDragOperationMove | NSDragOperationGeneric;

        return NSDragOperationNone;
    }

    static void draggingEnded (id self, SEL s, id<NSDraggingInfo> sender)
    {
        draggingExited (self, s, sender);
    }

    static void draggingExited (id self, SEL, id<NSDraggingInfo> sender)
    {
        if (auto* owner = getOwner (self))
            owner->sendDragCallback (1, sender);
    }

    static BOOL prepareForDragOperation (id, SEL, id<NSDraggingInfo>)
    {
        return YES;
    }

    static BOOL performDragOperation (id self, SEL, id<NSDraggingInfo> sender)
    {
        auto* owner = getOwner (self);
        return owner != nullptr && owner->sendDragCallback (2, sender);
    }

    static void concludeDragOperation (id, SEL, id<NSDraggingInfo>) {}
};

//==============================================================================
struct JuceNSWindowClass   : public ObjCClass<NSWindow>
{
    JuceNSWindowClass()  : ObjCClass<NSWindow> ("JUCEWindow_")
    {
        addIvar<NSViewComponentPeer*> ("owner");

        addMethod (@selector (canBecomeKeyWindow),            canBecomeKeyWindow,        "c@:");
        addMethod (@selector (canBecomeMainWindow),           canBecomeMainWindow,       "c@:");
        addMethod (@selector (becomeKeyWindow),               becomeKeyWindow,           "v@:");
        addMethod (@selector (windowShouldClose:),            windowShouldClose,         "c@:@");
        addMethod (@selector (constrainFrameRect:toScreen:),  constrainFrameRect,        @encode (NSRect), "@:",  @encode (NSRect), "@");
        addMethod (@selector (windowWillResize:toSize:),      windowWillResize,          @encode (NSSize), "@:@", @encode (NSSize));
        addMethod (@selector (windowDidExitFullScreen:),      windowDidExitFullScreen,   "v@:@");
        addMethod (@selector (zoom:),                         zoom,                      "v@:@");
        addMethod (@selector (windowWillMove:),               windowWillMove,            "v@:@");
        addMethod (@selector (windowWillStartLiveResize:),    windowWillStartLiveResize, "v@:@");
        addMethod (@selector (windowDidEndLiveResize:),       windowDidEndLiveResize,    "v@:@");

       #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
        addProtocol (@protocol (NSWindowDelegate));
       #endif

        registerClass();
    }

private:
    static NSViewComponentPeer* getOwner (id self)
    {
        return getIvar<NSViewComponentPeer*> (self, "owner");
    }

    //==============================================================================
    static BOOL canBecomeKeyWindow (id self, SEL)
    {
        auto* owner = getOwner (self);

        return owner != nullptr
                && owner->canBecomeKeyWindow()
                && ! owner->sendModalInputAttemptIfBlocked();
    }

    static BOOL canBecomeMainWindow (id self, SEL)
    {
        auto* owner = getOwner (self);

        return owner != nullptr
                && owner->canBecomeMainWindow()
                && ! owner->sendModalInputAttemptIfBlocked();
    }

    static void becomeKeyWindow (id self, SEL)
    {
        sendSuperclassMessage (self, @selector (becomeKeyWindow));

        if (auto* owner = getOwner (self))
            owner->becomeKeyWindow();
    }

    static BOOL windowShouldClose (id self, SEL, id /*window*/)
    {
        auto* owner = getOwner (self);
        return owner == nullptr || owner->windowShouldClose();
    }

    static NSRect constrainFrameRect (id self, SEL, NSRect frameRect, NSScreen*)
    {
        if (auto* owner = getOwner (self))
            frameRect = owner->constrainRect (frameRect);

        return frameRect;
    }

    static NSSize windowWillResize (id self, SEL, NSWindow*, NSSize proposedFrameSize)
    {
        auto* owner = getOwner (self);

        if (owner == nullptr || owner->isZooming)
            return proposedFrameSize;

        NSRect frameRect = [(NSWindow*) self frame];
        frameRect.origin.y -= proposedFrameSize.height - frameRect.size.height;
        frameRect.size = proposedFrameSize;

        frameRect = owner->constrainRect (frameRect);

        if (owner->hasNativeTitleBar())
            owner->sendModalInputAttemptIfBlocked();

        return frameRect.size;
    }

    static void windowDidExitFullScreen (id, SEL, NSNotification*)
    {
       #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
        [NSApp setPresentationOptions: NSApplicationPresentationDefault];
       #endif
    }

    static void zoom (id self, SEL, id sender)
    {
        if (auto* owner = getOwner (self))
        {
            owner->isZooming = true;
            objc_super s = { self, [NSWindow class] };
            getMsgSendSuperFn() (&s, @selector (zoom:), sender);
            owner->isZooming = false;

            owner->redirectMovedOrResized();
        }
    }

    static void windowWillMove (id self, SEL, NSNotification*)
    {
        if (auto* owner = getOwner (self))
            if (owner->hasNativeTitleBar())
                owner->sendModalInputAttemptIfBlocked();
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
ModifierKeys NSViewComponentPeer::currentModifiers;
ComponentPeer* NSViewComponentPeer::currentlyFocusedPeer = nullptr;
Array<int> NSViewComponentPeer::keysCurrentlyDown;

//==============================================================================
bool KeyPress::isKeyCurrentlyDown (const int keyCode)
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


ModifierKeys ModifierKeys::getCurrentModifiersRealtime() noexcept
{
   #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
    if ([NSEvent respondsToSelector: @selector (modifierFlags)])
        NSViewComponentPeer::updateModifiers ((NSUInteger) [NSEvent modifierFlags]);
   #endif

    return NSViewComponentPeer::currentModifiers;
}

void ModifierKeys::updateCurrentModifiers() noexcept
{
    currentModifiers = NSViewComponentPeer::currentModifiers;
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
   #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6

    auto* peer = dynamic_cast<NSViewComponentPeer*> (kioskComp->getPeer());
    jassert (peer != nullptr); // (this should have been checked by the caller)

   #if defined (MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (peer->hasNativeTitleBar()
          && [peer->window respondsToSelector: @selector (toggleFullScreen:)])
    {
        if (shouldBeEnabled && ! allowMenusAndBars)
            [NSApp setPresentationOptions: NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar];

        [peer->window performSelector: @selector (toggleFullScreen:) withObject: nil];
    }
    else
   #endif
    {
        if (shouldBeEnabled)
        {
            if (peer->hasNativeTitleBar())
                [peer->window setStyleMask: NSWindowStyleMaskBorderless];

            [NSApp setPresentationOptions: (allowMenusAndBars ? (NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar)
                                                              : (NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar))];
            kioskComp->setBounds (Desktop::getInstance().getDisplays().getMainDisplay().totalArea);
            peer->becomeKeyWindow();
        }
        else
        {
            if (peer->hasNativeTitleBar())
            {
                [peer->window setStyleMask: (NSViewComponentPeer::getNSWindowStyleMask (peer->getStyleFlags()))];
                peer->setTitle (peer->getComponent().getName()); // required to force the OS to update the title
            }

            [NSApp setPresentationOptions: NSApplicationPresentationDefault];
        }
    }
   #elif JUCE_SUPPORT_CARBON
    if (shouldBeEnabled)
    {
        SetSystemUIMode (kUIModeAllSuppressed, allowMenusAndBars ? kUIOptionAutoShowMenuBar : 0);
        kioskComp->setBounds (Desktop::getInstance().getDisplays().getMainDisplay().totalArea);
    }
    else
    {
        SetSystemUIMode (kUIModeNormal, 0);
    }
   #else
    ignoreUnused (kioskComp, shouldBeEnabled, allowMenusAndBars);

    // If you're targeting OSes earlier than 10.6 and want to use this feature,
    // you'll need to enable JUCE_SUPPORT_CARBON.
    jassertfalse;
   #endif
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


const int KeyPress::numberPad0      = 0x30020;
const int KeyPress::numberPad1      = 0x30021;
const int KeyPress::numberPad2      = 0x30022;
const int KeyPress::numberPad3      = 0x30023;
const int KeyPress::numberPad4      = 0x30024;
const int KeyPress::numberPad5      = 0x30025;
const int KeyPress::numberPad6      = 0x30026;
const int KeyPress::numberPad7      = 0x30027;
const int KeyPress::numberPad8      = 0x30028;
const int KeyPress::numberPad9      = 0x30029;
const int KeyPress::numberPadAdd            = 0x3002a;
const int KeyPress::numberPadSubtract       = 0x3002b;
const int KeyPress::numberPadMultiply       = 0x3002c;
const int KeyPress::numberPadDivide         = 0x3002d;
const int KeyPress::numberPadSeparator      = 0x3002e;
const int KeyPress::numberPadDecimalPoint   = 0x3002f;
const int KeyPress::numberPadEquals         = 0x30030;
const int KeyPress::numberPadDelete         = 0x30031;
const int KeyPress::playKey         = 0x30000;
const int KeyPress::stopKey         = 0x30001;
const int KeyPress::fastForwardKey  = 0x30002;
const int KeyPress::rewindKey       = 0x30003;

} // namespace juce
