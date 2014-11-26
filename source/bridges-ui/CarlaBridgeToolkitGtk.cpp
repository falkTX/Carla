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

#include "CarlaBridgeClient.hpp"
#include "CarlaBridgeToolkit.hpp"

#include <gtk/gtk.h>

#ifdef CARLA_OS_LINUX
# include <gdk/gdkx.h>
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

static int    gargc = 0;
static char** gargv = nullptr;

// -------------------------------------------------------------------------

class CarlaBridgeToolkitGtk : public CarlaBridgeToolkit
{
public:
    CarlaBridgeToolkitGtk(CarlaBridgeClient* const client, const char* const windowTitle)
        : CarlaBridgeToolkit(client, windowTitle),
          fNeedsShow(false),
          fWindow(nullptr),
          fLastX(0),
          fLastY(0),
          fLastWidth(0),
          fLastHeight(0),
          leakDetector_CarlaBridgeToolkitGtk()
    {
        carla_debug("CarlaBridgeToolkitGtk::CarlaBridgeToolkitGtk(%p, \"%s\")", client, windowTitle);
    }

    ~CarlaBridgeToolkitGtk() override
    {
        CARLA_SAFE_ASSERT(fWindow == nullptr);
        carla_debug("CarlaBridgeToolkitGtk::~CarlaBridgeToolkitGtk()");
    }

    void init() override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow == nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::init()");

        gtk_init(&gargc, &gargv);

        fWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_resize(GTK_WINDOW(fWindow), 30, 30);
        gtk_widget_hide(fWindow);
    }

    void exec(const bool showUI) override
    {
        CARLA_SAFE_ASSERT_RETURN(kClient != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::exec(%s)", bool2str(showUI));

        GtkWidget* const widget((GtkWidget*)kClient->getWidget());

        gtk_container_add(GTK_CONTAINER(fWindow), widget);

        gtk_window_set_resizable(GTK_WINDOW(fWindow), kClient->isResizable());
        gtk_window_set_title(GTK_WINDOW(fWindow), kWindowTitle);

        if (const char* const winIdStr = std::getenv("ENGINE_OPTION_FRONTEND_WIN_ID"))
        {
            if (const long long winId = std::strtoll(winIdStr, nullptr, 16))
            {
#ifdef CARLA_OS_LINUX
                if (GdkWindow* const gdkWindow = GDK_WINDOW(fWindow))
                {
                    if (GdkDisplay* const gdkDisplay = gdk_window_get_display(gdkWindow))
                    {
                        ::Display* const display(gdk_x11_display_get_xdisplay(gdkDisplay));
# ifdef BRIDGE_GTK3
                        ::XID const xid(gdk_x11_window_get_xid(gdkWindow));
# else
                        ::XID xid = 0;
                        if (GdkDrawable* const gdkDrawable = GDK_DRAWABLE(fWindow))
                            xid = gdk_x11_drawable_get_xid(gdkDrawable);
# endif
                        if (display != nullptr && xid != 0)
                            XSetTransientForHint(display, xid, static_cast<::Window>(winId));
                    }
                }
#else
                (void)winId;
#endif
            }
        }

        if (showUI || fNeedsShow)
        {
            show();
            fNeedsShow = false;
        }

        g_timeout_add(30, gtk_ui_timeout, this);
        g_signal_connect(fWindow, "destroy", G_CALLBACK(gtk_ui_destroy), this);

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

    void hide() override
    {
        carla_debug("CarlaBridgeToolkitGtk::hide()");

        fNeedsShow = false;

        if (fWindow != nullptr)
            gtk_widget_hide(fWindow);
    }

    void resize(int width, int height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitGtk::resize(%i, %i)", width, height);

        gtk_window_resize(GTK_WINDOW(fWindow), width, height);
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

    gboolean handleTimeout()
    {
        if (fWindow != nullptr)
        {
            gtk_window_get_position(GTK_WINDOW(fWindow), &fLastX, &fLastY);
            gtk_window_get_size(GTK_WINDOW(fWindow), &fLastWidth, &fLastHeight);
        }

        kClient->uiIdle();

        if (! kClient->oscIdle())
        {
            gtk_main_quit_if_needed();
            return false;
        }

        return true;
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

    static gboolean gtk_ui_timeout(gpointer data)
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        return ((CarlaBridgeToolkitGtk*)data)->handleTimeout();
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeToolkitGtk)
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const windowTitle)
{
    return new CarlaBridgeToolkitGtk(client, windowTitle);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE
