/*
 * Carla Bridge UI
 * Copyright (C) 2011-2021 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaBridgeFormat.hpp"
#include "CarlaBridgeToolkit.hpp"
#include "CarlaLibUtils.hpp"

#include <glib.h>
#include <glib-object.h>

#ifdef HAVE_X11
# include <X11/Xlib.h>
#endif

// #include <gtk/gtk.h>

// #ifdef HAVE_X11
// # include <gdk/gdkx.h>
// #endif

// #undef GTK_TYPE_CONTAINER
// #define GTK_TYPE_CONTAINER (gtk.container_get_type())
//
// #undef GTK_TYPE_WINDOW
// #define GTK_TYPE_WINDOW (gtk.window_get_type())

#define GTK_CONTAINER(G) ((GtkContainer*)G)
#define GTK_WINDOW(G)    ((GtkWindow*)G)

struct GdkDisplay;
struct GdkDrawable;
struct GdkWindow;

struct GtkContainer;
struct GtkWidget;
struct GtkWindow;

enum GtkWindowType {
    GTK_WINDOW_TOPLEVEL
};

typedef void (*gtksym_init)(int* argc, char*** argv);
typedef void (*gtksym_main)(void);
typedef uint (*gtksym_main_level)(void);
typedef void (*gtksym_main_quit)(void);
typedef GType (*gtksym_container_get_type)(void) G_GNUC_CONST;
typedef void (*gtksym_container_add)(GtkContainer* container, GtkWidget* widget);
typedef void (*gtksym_widget_destroy)(GtkWidget* widget);
typedef void (*gtksym_widget_hide)(GtkWidget* widget);
typedef void (*gtksym_widget_show_all)(GtkWidget* widget);
typedef GType (*gtksym_window_get_type)(void) G_GNUC_CONST;
typedef GtkWidget* (*gtksym_window_new)(GtkWindowType type);
typedef void (*gtksym_window_get_position)(GtkWindow* window, int* root_x, int* root_y);
typedef void (*gtksym_window_get_size)(GtkWindow* window, int* width, int* height);
typedef void (*gtksym_window_resize)(GtkWindow* window, int width, int height);
typedef void (*gtksym_window_set_resizable)(GtkWindow* window, int resizable);
typedef void (*gtksym_window_set_title)(GtkWindow* window, const char* title);
#ifdef HAVE_X11
typedef GdkWindow* (*gtksym_widget_get_window)(GtkWidget* widget);
# ifdef BRIDGE_GTK3
typedef GdkDisplay* (*gdksym_window_get_display)(GdkWindow* window);
typedef Display* (*gdksym_x11_display_get_xdisplay)(GdkDisplay* display);
typedef Window (*gdksym_x11_window_get_xid)(GdkWindow* window);
# else
typedef Display* (*gdksym_x11_drawable_get_xdisplay)(GdkDrawable* drawable);
typedef XID      (*gdksym_x11_drawable_get_xid)(GdkDrawable* drawable);
# endif
#endif

CARLA_BRIDGE_UI_START_NAMESPACE

// -------------------------------------------------------------------------

struct GtkLoader {
    void* lib;
    gtksym_init init;
    gtksym_main main;
    gtksym_main_level main_level;
    gtksym_main_quit main_quit;
    gtksym_container_get_type container_get_type;
    gtksym_container_add container_add;
    gtksym_widget_destroy widget_destroy;
    gtksym_widget_hide widget_hide;
    gtksym_widget_show_all widget_show_all;
    gtksym_window_get_type window_get_type;
    gtksym_window_new window_new;
    gtksym_window_get_position window_get_position;
    gtksym_window_get_size window_get_size;
    gtksym_window_resize window_resize;
    gtksym_window_set_resizable window_set_resizable;
    gtksym_window_set_title window_set_title;
    bool ok;

#ifdef HAVE_X11
    gtksym_widget_get_window widget_get_window;
# ifdef BRIDGE_GTK3
    gdksym_window_get_display window_get_display;
    gdksym_x11_display_get_xdisplay x11_display_get_xdisplay;
    gdksym_x11_window_get_xid x11_window_get_xid;
# else
    gdksym_x11_drawable_get_xdisplay x11_drawable_get_xdisplay;
    gdksym_x11_drawable_get_xid x11_drawable_get_xid;
# endif
#endif

    GtkLoader()
        : lib(nullptr),
          init(nullptr),
          main(nullptr),
          main_level(nullptr),
          main_quit(nullptr),
          container_get_type(nullptr),
          container_add(nullptr),
          widget_destroy(nullptr),
          widget_hide(nullptr),
          widget_show_all(nullptr),
          window_get_type(nullptr),
          window_new(nullptr),
          window_get_position(nullptr),
          window_get_size(nullptr),
          window_resize(nullptr),
          window_set_resizable(nullptr),
          window_set_title(nullptr),
          ok(false)
#ifdef HAVE_X11
        , widget_get_window(nullptr),
# ifdef BRIDGE_GTK3
          window_get_display(nullptr),
          x11_display_get_xdisplay(nullptr),
          x11_window_get_xid(nullptr)
# else
          x11_drawable_get_xdisplay(nullptr),
          x11_drawable_get_xid(nullptr)
# endif
#endif
    {
        const char* filename;
        const char* const filenames[] = {
#ifdef BRIDGE_GTK3
# if defined(CARLA_OS_MAC)
            "libgtk-3.0.dylib",
# else
            "libgtk-3.so.0",
# endif
#else
# if defined(CARLA_OS_MAC)
            "libgtk-quartz-2.0.dylib",
            "libgtk-x11-2.0.dylib",
            "/opt/local/lib/libgtk-quartz-2.0.dylib",
            "/opt/local/lib/libgtk-x11-2.0.dylib",
# else
            "libgtk-x11-2.0.so.0",
# endif
#endif
        };

        for (size_t i=0; i<sizeof(filenames)/sizeof(filenames[0]); ++i)
        {
            filename = filenames[i];
            if ((lib = lib_open(filename)) != nullptr)
                break;
        }

        if (lib == nullptr)
        {
            fprintf(stderr, "Failed to load Gtk, reason:\n%s\n", lib_error(filename));
            return;
        }
        else
        {
            fprintf(stdout, "%s loaded successfully!\n", filename);
        }

        #define GTK_LIB_SYMBOL(NAME) \
            NAME = lib_symbol<gtksym_##NAME>(lib, "gtk_" #NAME); \
            CARLA_SAFE_ASSERT_RETURN(NAME != nullptr,);

        #define GDK_LIB_SYMBOL(NAME) \
            NAME = lib_symbol<gdksym_##NAME>(lib, "gdk_" #NAME);

        GTK_LIB_SYMBOL(init)
        GTK_LIB_SYMBOL(main)
        GTK_LIB_SYMBOL(main_level)
        GTK_LIB_SYMBOL(main_quit)
        GTK_LIB_SYMBOL(container_get_type)
        GTK_LIB_SYMBOL(container_add)
        GTK_LIB_SYMBOL(widget_destroy)
        GTK_LIB_SYMBOL(widget_hide)
        GTK_LIB_SYMBOL(widget_show_all)
        GTK_LIB_SYMBOL(window_get_type)
        GTK_LIB_SYMBOL(window_new)
        GTK_LIB_SYMBOL(window_get_position)
        GTK_LIB_SYMBOL(window_get_size)
        GTK_LIB_SYMBOL(window_resize)
        GTK_LIB_SYMBOL(window_set_resizable)
        GTK_LIB_SYMBOL(window_set_title)

        ok = true;

#ifdef HAVE_X11
        GTK_LIB_SYMBOL(widget_get_window)
# ifdef BRIDGE_GTK3
        GDK_LIB_SYMBOL(window_get_display)
        GDK_LIB_SYMBOL(x11_display_get_xdisplay)
        GDK_LIB_SYMBOL(x11_window_get_xid)
# else
        GDK_LIB_SYMBOL(x11_drawable_get_xdisplay)
        GDK_LIB_SYMBOL(x11_drawable_get_xid)
# endif
#endif

        #undef GDK_LIB_SYMBOL
        #undef GTK_LIB_SYMBOL
    }

    ~GtkLoader()
    {
        if (lib != nullptr)
            lib_close(lib);
    }

    void main_quit_if_needed()
    {
        if (main_level() != 0)
            main_quit();
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GtkLoader)
};

// -------------------------------------------------------------------------

static const bool gHideShowTesting = std::getenv("CARLA_UI_TESTING") != nullptr;

// -------------------------------------------------------------------------

class CarlaBridgeToolkitGtk : public CarlaBridgeToolkit
{
public:
    CarlaBridgeToolkitGtk(CarlaBridgeFormat* const format)
        : CarlaBridgeToolkit(format),
          gtk(),
          fNeedsShow(false),
          fWindow(nullptr),
          fLastX(0),
          fLastY(0),
          fLastWidth(0),
          fLastHeight(0)
    {
        carla_debug("CarlaBridgeToolkitGtk::CarlaBridgeToolkitGtk(%p)", format);
    }

    ~CarlaBridgeToolkitGtk() override
    {
        CARLA_SAFE_ASSERT(fWindow == nullptr);
        carla_debug("CarlaBridgeToolkitGtk::~CarlaBridgeToolkitGtk()");
    }

    bool init(const int /*argc*/, const char** /*argv[]*/) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow == nullptr, false);
        carla_debug("CarlaBridgeToolkitGtk::init()");

        if (! gtk.ok)
            return false;

        static int    gargc = 0;
        static char** gargv = nullptr;
        gtk.init(&gargc, &gargv);

        fWindow = gtk.window_new(GTK_WINDOW_TOPLEVEL);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr, false);

        gtk.window_resize(GTK_WINDOW(fWindow), 30, 30);
        gtk.widget_hide(fWindow);

        return true;
    }

    void exec(const bool showUI) override
    {
        CARLA_SAFE_ASSERT_RETURN(fPlugin != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::exec(%s)", bool2str(showUI));

        const CarlaBridgeFormat::Options& options(fPlugin->getOptions());

        GtkWindow* const gtkWindow(GTK_WINDOW(fWindow));
        CARLA_SAFE_ASSERT_RETURN(gtkWindow != nullptr,);

        GtkWidget* const widget((GtkWidget*)fPlugin->getWidget());
        gtk.container_add(GTK_CONTAINER(fWindow), widget);

        gtk.window_set_resizable(gtkWindow, options.isResizable);
        gtk.window_set_title(gtkWindow, options.windowTitle.buffer());

        if (showUI || fNeedsShow)
        {
            show();
            fNeedsShow = false;
        }

        g_timeout_add(30, gtk_ui_timeout, this);
        g_signal_connect(fWindow, "destroy", G_CALLBACK(gtk_ui_destroy), this);
        g_signal_connect(fWindow, "realize", G_CALLBACK(gtk_ui_realize), this);

        // First idle
        handleTimeout();

        // Main loop
        gtk.main();
    }

    void quit() override
    {
        carla_debug("CarlaBridgeToolkitGtk::quit()");

        if (fWindow != nullptr)
        {
            gtk.widget_destroy(fWindow);
            fWindow = nullptr;

            gtk.main_quit_if_needed();
        }
    }

    void show() override
    {
        carla_debug("CarlaBridgeToolkitGtk::show()");

        fNeedsShow = true;

        if (fWindow != nullptr)
            gtk.widget_show_all(fWindow);
    }

    void focus() override
    {
        carla_debug("CarlaBridgeToolkitGtk::focus()");
    }

    void hide() override
    {
        carla_debug("CarlaBridgeToolkitGtk::hide()");

        fNeedsShow = false;

        if (fWindow != nullptr)
            gtk.widget_hide(fWindow);
    }

    void setChildWindow(void* const) override {}

    void setSize(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::resize(%i, %i)", width, height);

        gtk.window_resize(GTK_WINDOW(fWindow), static_cast<int>(width), static_cast<int>(height));
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::setTitle(\"%s\")", title);

        gtk.window_set_title(GTK_WINDOW(fWindow), title);
    }

    // ---------------------------------------------------------------------

protected:
    GtkLoader gtk;
    bool fNeedsShow;
    GtkWidget* fWindow;

    int fLastX;
    int fLastY;
    int fLastWidth;
    int fLastHeight;

    void handleDestroy()
    {
        carla_debug("CarlaBridgeToolkitGtk::handleDestroy()");

        fWindow = nullptr;
        gtk.main_quit_if_needed();
    }

    void handleRealize()
    {
        carla_debug("CarlaBridgeToolkitGtk::handleRealize()");

#ifdef HAVE_X11
        const CarlaBridgeFormat::Options& options(fPlugin->getOptions());

        if (options.transientWindowId != 0)
            setTransient(options.transientWindowId);
#endif
    }

    int handleTimeout()
    {
        if (fWindow != nullptr)
        {
            gtk.window_get_position(GTK_WINDOW(fWindow), &fLastX, &fLastY);
            gtk.window_get_size(GTK_WINDOW(fWindow), &fLastWidth, &fLastHeight);
        }

        if (fPlugin->isPipeRunning())
            fPlugin->idlePipe();

        fPlugin->idleUI();

        if (gHideShowTesting)
        {
            static int counter = 0;
            ++counter;

            if (counter == 100)
            {
                hide();
            }
            else if (counter == 200)
            {
                show();
                counter = 0;
            }
        }

        return 1;
    }

#ifdef HAVE_X11
    void setTransient(const uintptr_t winId)
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::setTransient(0x" P_UINTPTR ")", winId);

        if (gtk.widget_get_window == nullptr)
            return;
# ifdef BRIDGE_GTK3
        if (gtk.window_get_display == nullptr)
            return;
        if (gtk.x11_display_get_xdisplay == nullptr)
            return;
        if (gtk.x11_window_get_xid == nullptr)
            return;
# else
        if (gtk.x11_drawable_get_xdisplay == nullptr)
            return;
        if (gtk.x11_drawable_get_xid == nullptr)
            return;
# endif

        GdkWindow* const gdkWindow = gtk.widget_get_window(fWindow);
        CARLA_SAFE_ASSERT_RETURN(gdkWindow != nullptr,);

# ifdef BRIDGE_GTK3
        GdkDisplay* const gdkDisplay = gtk.window_get_display(gdkWindow);
        CARLA_SAFE_ASSERT_RETURN(gdkDisplay != nullptr,);

        ::Display* const display = gtk.x11_display_get_xdisplay(gdkDisplay);
        CARLA_SAFE_ASSERT_RETURN(display != nullptr,);

        const ::XID xid = gtk.x11_window_get_xid(gdkWindow);
        CARLA_SAFE_ASSERT_RETURN(xid != 0,);
# else
        ::Display* const display = gtk.x11_drawable_get_xdisplay((GdkDrawable*)gdkWindow);
        CARLA_SAFE_ASSERT_RETURN(display != nullptr,);

        const ::XID xid = gtk.x11_drawable_get_xid((GdkDrawable*)gdkWindow);
        CARLA_SAFE_ASSERT_RETURN(xid != 0,);
# endif

        XSetTransientForHint(display, xid, static_cast< ::Window>(winId));
    }
#endif

    // ---------------------------------------------------------------------

private:
    static void gtk_ui_destroy(GtkWidget*, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        ((CarlaBridgeToolkitGtk*)data)->handleDestroy();
    }

    static void gtk_ui_realize(GtkWidget*, void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        ((CarlaBridgeToolkitGtk*)data)->handleRealize();
    }

    static int gtk_ui_timeout(void* data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        return ((CarlaBridgeToolkitGtk*)data)->handleTimeout();
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeToolkitGtk)
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeFormat* const format)
{
    return new CarlaBridgeToolkitGtk(format);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_UI_END_NAMESPACE
