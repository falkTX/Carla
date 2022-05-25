/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DGL_WINDOW_PRIVATE_DATA_HPP_INCLUDED
#define DGL_WINDOW_PRIVATE_DATA_HPP_INCLUDED

#include "../Window.hpp"
#include "../Widget.hpp"
#include "ApplicationPrivateData.hpp"

#include "pugl.hpp"

#include <list>

START_NAMESPACE_DGL

class TopLevelWidget;

// -----------------------------------------------------------------------

struct Window::PrivateData : IdleCallback {
    /** Reference to the DGL Application class this (private data) window associates with. */
    Application& app;

    /** Direct access to the DGL Application private data where we registers ourselves in. */
    Application::PrivateData* const appData;

    /** Pointer to the the DGL Window class that this private data belongs to. */
    Window* const self;

    /** Pugl view instance. */
    PuglView* view;

    /** Pugl view instance of the transient parent window. */
    PuglView* const transientParentView;

    /** Reserved space for graphics context. */
    mutable uint8_t graphicsContext[sizeof(void*)];

    /** The top-level widgets associated with this Window. */
    std::list<TopLevelWidget*> topLevelWidgets;

    /** Whether this Window is closed (not visible or counted in the Application it is tied to).
        Defaults to true unless embed (embed windows are never closed). */
    bool isClosed;

    /** Whether this Window is currently visible/mapped. Defaults to false. */
    bool isVisible;

    /** Whether this Window is embed into another (usually not DGL-controlled) Window. */
    const bool isEmbed;

    /** Whether to ignore resize requests and feed them into the host instead. used for VST3 */
    const bool usesSizeRequest;

    /** Scale factor to report to widgets on request, purely informational. */
    double scaleFactor;

    /** Automatic scaling to apply on widgets, implemented internally. */
    bool autoScaling;
    double autoScaleFactor;

    /** Pugl geometry constraints access. */
    uint minWidth, minHeight;
    bool keepAspectRatio;

    /** Whether to ignore idle callback requests, useful for temporary windows. */
    bool ignoreIdleCallbacks;

    /** Whether to ignore pugl events (except create and destroy), used for puglGetClipboard. */
    bool ignoreEvents;

    /** Render to a picture file when non-null, automatically free+unset after saving. */
    char* filenameToRenderInto;

#ifndef DGL_FILE_BROWSER_DISABLED
    /** Handle for file browser dialog operations. */
    FileBrowserHandle fileBrowserHandle;
#endif

    /** Modal window setup. */
    struct Modal {
        PrivateData* parent; // parent of this window (so we can become modal)
        PrivateData* child;  // child window to give focus to when modal mode is enabled
        bool enabled;        // wherever modal mode is enabled (only possible if parent != null)

        /** Constructor for a non-modal window. */
        Modal() noexcept
            : parent(nullptr),
              child(nullptr),
              enabled(false) {}

        /** Constructor for a modal window (with a parent). */
        Modal(PrivateData* const p) noexcept
            : parent(p),
              child(nullptr),
              enabled(false) {}

        /** Destructor. */
        ~Modal() noexcept
        {
            DISTRHO_SAFE_ASSERT(! enabled);
        }

        DISTRHO_DECLARE_NON_COPYABLE(Modal)
        DISTRHO_PREVENT_HEAP_ALLOCATION
    } modal;

    /** Constructor for a regular, standalone window. */
    explicit PrivateData(Application& app, Window* self);

    /** Constructor for a modal window. */
    explicit PrivateData(Application& app, Window* self, PrivateData* ppData);

    /** Constructor for an embed Window, with a few extra hints from the host side. */
    explicit PrivateData(Application& app, Window* self, uintptr_t parentWindowHandle, double scaling, bool resizable);

    /** Constructor for an embed Window, with a few extra hints from the host side. */
    explicit PrivateData(Application& app, Window* self, uintptr_t parentWindowHandle,
                         uint width, uint height, double scaling, bool resizable, bool isVST3);

    /** Destructor. */
    ~PrivateData() override;

    /** Helper initialization function called at the end of all this class constructors. */
    void initPre(uint width, uint height, bool resizable);
    /** Helper initialization function called on the Window constructor after we are done. */
    bool initPost();

    /** Hide window and notify application of a window close event.
      * Does nothing if window is embed (that is, not standalone).
      * The application event-loop will stop when all windows have been closed.
      *
      * @note It is possible to hide the window while not stopping the event-loop.
      *       A closed window is always hidden, but the reverse is not always true.
      */
    void close();

    void show();
    void hide();

    void focus();

    void setResizable(bool resizable);

    const GraphicsContext& getGraphicsContext() const noexcept;

    // idle callback stuff
    void idleCallback() override;
    bool addIdleCallback(IdleCallback* callback, uint timerFrequencyInMs);
    bool removeIdleCallback(IdleCallback* callback);

#ifndef DGL_FILE_BROWSER_DISABLED
    // file handling
    bool openFileBrowser(const FileBrowserOptions& options);
#endif

    static void renderToPicture(const char* filename, const GraphicsContext& context, uint width, uint height);

    // modal handling
    void startModal();
    void stopModal();
    void runAsModal(bool blockWait);

    // pugl events
    void onPuglConfigure(double width, double height);
    void onPuglExpose();
    void onPuglClose();
    void onPuglFocus(bool focus, CrossingMode mode);
    void onPuglKey(const Widget::KeyboardEvent& ev);
    void onPuglText(const Widget::CharacterInputEvent& ev);
    void onPuglMouse(const Widget::MouseEvent& ev);
    void onPuglMotion(const Widget::MotionEvent& ev);
    void onPuglScroll(const Widget::ScrollEvent& ev);

    // Pugl event handling entry point
    static PuglStatus puglEventCallback(PuglView* view, const PuglEvent* event);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WINDOW_PRIVATE_DATA_HPP_INCLUDED
