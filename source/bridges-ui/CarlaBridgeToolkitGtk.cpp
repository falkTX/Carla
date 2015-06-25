/*
 * Carla Bridge Toolkit, Gtk version
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBridgeUI.hpp"

#include <gtk/gtk.h>

#if defined(CARLA_OS_LINUX) && defined(HAVE_X11)
# define USE_CUSTOM_X11_METHODS
# include <gdk/gdkx.h>
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

static int    gargc = 0;
static char** gargv = nullptr;

static const bool gHideShowTesting = std::getenv("CARLA_UI_TESTING") != nullptr;

// -------------------------------------------------------------------------

class CarlaBridgeToolkitGtk : public CarlaBridgeToolkit
{
public:
    CarlaBridgeToolkitGtk(CarlaBridgeUI* const u)
        : CarlaBridgeToolkit(u),
          fNeedsShow(false),
          fWindow(nullptr),
          fLastX(0),
          fLastY(0),
          fLastWidth(0),
          fLastHeight(0)
    {
        carla_debug("CarlaBridgeToolkitGtk::CarlaBridgeToolkitGtk(%p)", u);
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

        gtk_init(&gargc, &gargv);

        fWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr, false);

        gtk_window_resize(GTK_WINDOW(fWindow), 30, 30);
        gtk_widget_hide(fWindow);

        return true;
    }

    void exec(const bool showUI) override
    {
        CARLA_SAFE_ASSERT_RETURN(ui != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::exec(%s)", bool2str(showUI));

        const CarlaBridgeUI::Options& options(ui->getOptions());

        GtkWindow* const gtkWindow(GTK_WINDOW(fWindow));
        CARLA_SAFE_ASSERT_RETURN(gtkWindow != nullptr,);

        GtkWidget* const widget((GtkWidget*)ui->getWidget());
        gtk_container_add(GTK_CONTAINER(fWindow), widget);

        gtk_window_set_resizable(gtkWindow, options.isResizable);
        gtk_window_set_title(gtkWindow, options.windowTitle.buffer());

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
        gtk_main();
    }

    void quit() override
    {
        carla_debug("CarlaBridgeToolkitGtk::quit()");

        if (fWindow != nullptr)
        {
            gtk_widget_destroy(fWindow);
            fWindow = nullptr;

            gtk_main_quit_if_needed();
        }
    }

    void show() override
    {
        carla_debug("CarlaBridgeToolkitGtk::show()");

        fNeedsShow = true;

        if (fWindow != nullptr)
            gtk_widget_show_all(fWindow);
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
            gtk_widget_hide(fWindow);
    }

    void setSize(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::resize(%i, %i)", width, height);

        gtk_window_resize(GTK_WINDOW(fWindow), width, height);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::setTitle(\"%s\")", title);

        gtk_window_set_title(GTK_WINDOW(fWindow), title);
    }

    // ---------------------------------------------------------------------

protected:
    bool fNeedsShow;
    GtkWidget* fWindow;

    gint fLastX;
    gint fLastY;
    gint fLastWidth;
    gint fLastHeight;

    void handleDestroy()
    {
        carla_debug("CarlaBridgeToolkitGtk::handleDestroy()");

        fWindow = nullptr;
    }

    void handleRealize()
    {
        carla_debug("CarlaBridgeToolkitGtk::handleRealize()");

        const CarlaBridgeUI::Options& options(ui->getOptions());

        if (options.transientWindowId != 0)
            setTransient(options.transientWindowId);
    }

    gboolean handleTimeout()
    {
        if (fWindow != nullptr)
        {
            gtk_window_get_position(GTK_WINDOW(fWindow), &fLastX, &fLastY);
            gtk_window_get_size(GTK_WINDOW(fWindow), &fLastWidth, &fLastHeight);
        }

        if (ui->isPipeRunning())
            ui->idlePipe();

        ui->idleUI();

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

        return true;
    }

    void setTransient(const uintptr_t winId)
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::setTransient(0x" P_UINTPTR ")", winId);

#ifdef USE_CUSTOM_X11_METHODS
        GdkWindow* const gdkWindow(gtk_widget_get_window(fWindow));
        CARLA_SAFE_ASSERT_RETURN(gdkWindow != nullptr,);

# ifdef BRIDGE_GTK3
        GdkDisplay* const gdkDisplay(gdk_window_get_display(gdkWindow));
        CARLA_SAFE_ASSERT_RETURN(gdkDisplay != nullptr,);

        ::Display* const display(gdk_x11_display_get_xdisplay(gdkDisplay));
        CARLA_SAFE_ASSERT_RETURN(display != nullptr,);

        const ::XID xid(gdk_x11_window_get_xid(gdkWindow));
        CARLA_SAFE_ASSERT_RETURN(xid != 0,);
# else
        ::Display* const display(gdk_x11_drawable_get_xdisplay(gdkWindow));
        CARLA_SAFE_ASSERT_RETURN(display != nullptr,);

        const ::XID xid(gdk_x11_drawable_get_xid(gdkWindow));
        CARLA_SAFE_ASSERT_RETURN(xid != 0,);
# endif

        XSetTransientForHint(display, xid, static_cast< ::Window>(winId));
#endif
    }

    // ---------------------------------------------------------------------

private:
    static void gtk_main_quit_if_needed()
    {
        if (gtk_main_level() != 0)
            gtk_main_quit();
    }

    static void gtk_ui_destroy(GtkWidget*, gpointer data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        ((CarlaBridgeToolkitGtk*)data)->handleDestroy();
        gtk_main_quit_if_needed();
    }

    static void gtk_ui_realize(GtkWidget*, gpointer data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        ((CarlaBridgeToolkitGtk*)data)->handleRealize();
    }

    static gboolean gtk_ui_timeout(gpointer data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        return ((CarlaBridgeToolkitGtk*)data)->handleTimeout();
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeToolkitGtk)
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeUI* const ui)
{
    return new CarlaBridgeToolkitGtk(ui);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE
