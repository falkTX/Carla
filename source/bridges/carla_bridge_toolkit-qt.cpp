/*
 * Carla Bridge Toolkit, Qt version
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_bridge_client.hpp"
#include "carla_bridge_toolkit.hpp"

#include <QtCore/QSettings>
#include <QtCore/QThread>
#include <QtCore/QTimerEvent>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QApplication>
# include <QtWidgets/QMainWindow>
# ifdef Q_WS_X11
#  undef Q_WS_X11
# endif
#else
# include <QtGui/QApplication>
# include <QtGui/QMainWindow>
# ifdef Q_WS_X11
#  include <QtGui/QX11EmbedContainer>
# endif
#endif

#if defined(BRIDGE_COCOA) || defined(BRIDGE_HWND) || defined(BRIDGE_X11)
# define BRIDGE_CONTAINER
# ifdef Q_WS_X11
typedef QX11EmbedContainer QEmbedContainer;
# else
typedef QWidget QEmbedContainer;
# endif
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

#if defined(BRIDGE_QT4)
static const char* const appName = "Carla-Qt4UIs";
#elif defined(BRIDGE_QT5)
static const char* const appName = "Carla-Qt5UIs";
#elif defined(BRIDGE_COCOA)
static const char* const appName = "Carla-CocoaUIs";
#elif defined(BRIDGE_HWND)
static const char* const appName = "Carla-HWNDUIs";
#elif defined(BRIDGE_X11)
static const char* const appName = "Carla-X11UIs";
#else
static const char* const appName = "Carla-UIs";
#endif

static int   qargc = 0;
static char* qargv[0] = {};

// -------------------------------------------------------------------------

class CarlaBridgeToolkitQt: public CarlaBridgeToolkit,
                            public QObject
{
public:
    CarlaBridgeToolkitQt(CarlaBridgeClient* const client, const char* const uiTitle)
        : CarlaBridgeToolkit(client, uiTitle),
          QObject(nullptr),
          settings("Cadence", appName)
    {
        qDebug("CarlaBridgeToolkitQt::CarlaBridgeToolkitQt(%p, \"%s\")", client, uiTitle);

        app = nullptr;
        window = nullptr;

        msgTimer = 0;

        needsResize = false;
        nextWidth   = 0;
        nextHeight  = 0;

#ifdef BRIDGE_CONTAINER
        embedContainer = nullptr;
#endif
    }

    ~CarlaBridgeToolkitQt()
    {
        qDebug("CarlaBridgeToolkitQt::~CarlaBridgeToolkitQt()");
        CARLA_ASSERT(! app);
        CARLA_ASSERT(! window);
        CARLA_ASSERT(! msgTimer);
    }

    void init()
    {
        qDebug("CarlaBridgeToolkitQt::init()");
        CARLA_ASSERT(! app);
        CARLA_ASSERT(! window);
        CARLA_ASSERT(! msgTimer);

        app = new QApplication(qargc, qargv);

        window = new QMainWindow(nullptr);
        window->resize(30, 30);
        window->hide();
    }

    void exec(const bool showGui)
    {
        qDebug("CarlaBridgeToolkitQt::exec(%s)", bool2str(showGui));
        CARLA_ASSERT(app);
        CARLA_ASSERT(window);
        CARLA_ASSERT(client);

#if defined(BRIDGE_QT4) || defined(BRIDGE_QT5)
        QWidget* const widget = (QWidget*)client->getWidget();

        window->setCentralWidget(widget);
        window->adjustSize();

        widget->setParent(window);
        widget->show();
#endif

        if (! client->isResizable())
        {
            window->setFixedSize(window->width(), window->height());
#ifdef Q_OS_WIN
            window->setWindowFlags(window->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
        }

        window->setWindowTitle(uiTitle);

        if (settings.contains(QString("%1/pos_x").arg(uiTitle)))
        {
            bool hasX, hasY;
            int posX = settings.value(QString("%1/pos_x").arg(uiTitle), window->x()).toInt(&hasX);
            int posY = settings.value(QString("%1/pos_y").arg(uiTitle), window->y()).toInt(&hasY);

            if (hasX && hasY)
                window->move(posX, posY);

            if (client->isResizable())
            {
                bool hasWidth, hasHeight;
                int width  = settings.value(QString("%1/width").arg(uiTitle), window->width()).toInt(&hasWidth);
                int height = settings.value(QString("%1/height").arg(uiTitle), window->height()).toInt(&hasHeight);

                if (hasWidth && hasHeight)
                    window->resize(width, height);
            }
        }

        if (showGui)
            show();
        else
            client->sendOscUpdate();

        // Timer
        msgTimer = startTimer(50);

        // First idle
        handleTimeout();

        // Main loop
        app->exec();
    }

    void quit()
    {
        qDebug("CarlaBridgeToolkitQt::quit()");
        CARLA_ASSERT(app);

        if (msgTimer != 0)
        {
            killTimer(msgTimer);
            msgTimer = 0;
        }

        if (window)
        {
            settings.setValue(QString("%1/pos_x").arg(uiTitle), window->x());
            settings.setValue(QString("%1/pos_y").arg(uiTitle), window->y());
            settings.setValue(QString("%1/width").arg(uiTitle), window->width());
            settings.setValue(QString("%1/height").arg(uiTitle), window->height());
            settings.sync();

            window->close();

            delete window;
            window = nullptr;
        }

#ifdef BRIDGE_CONTAINER
        if (embedContainer)
        {
            embedContainer->close();

            delete embedContainer;
            embedContainer = nullptr;
        }
#endif

        if (app)
        {
            if (! app->closingDown())
                app->quit();

            delete app;
            app = nullptr;
        }
    }

    void show()
    {
        qDebug("CarlaBridgeToolkitQt::show()");
        CARLA_ASSERT(window);

        if (window)
            window->show();
    }

    void hide()
    {
        qDebug("CarlaBridgeToolkitQt::hide()");
        CARLA_ASSERT(window);

        if (window)
            window->hide();
    }

    void resize(const int width, const int height)
    {
        qDebug("CarlaBridgeToolkitQt::resize(%i, %i)", width, height);
        CARLA_ASSERT(window);

        if (app->thread() != QThread::currentThread())
        {
            nextWidth  = width;
            nextHeight  = height;
            needsResize = true;
            return;
        }

        if (window)
            window->setFixedSize(width, height);

#ifdef BRIDGE_CONTAINER
        if (embedContainer)
            embedContainer->setFixedSize(width, height);
#endif
    }

#ifdef BRIDGE_CONTAINER
    void* getContainerId()
    {
        qDebug("CarlaBridgeToolkitQt::getContainerId()");
        CARLA_ASSERT(window);

        if (! embedContainer)
        {
            embedContainer = new QEmbedContainer(window);

            window->setCentralWidget(embedContainer);
            window->adjustSize();

            embedContainer->setParent(window);
            embedContainer->show();
        }

        return (void*)embedContainer->winId();
    }
#endif

protected:
    QApplication* app;
    QMainWindow* window;
    QSettings settings;
    int msgTimer;

    bool needsResize;
    int nextWidth, nextHeight;

#ifdef BRIDGE_CONTAINER
    QEmbedContainer* embedContainer;
#endif

    void handleTimeout()
    {
        if (! client)
            return;

        if (needsResize)
        {
            client->toolkitResize(nextWidth, nextHeight);
            needsResize = false;
        }

        if (client->isOscControlRegistered() && ! client->oscIdle())
        {
            killTimer(msgTimer);
            msgTimer = 0;
        }
    }

private:
    void timerEvent(QTimerEvent* const event)
    {
        if (event->timerId() == msgTimer)
            handleTimeout();

        QObject::timerEvent(event);
    }
};

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const uiTitle)
{
    return new CarlaBridgeToolkitQt(client, uiTitle);
}

CARLA_BRIDGE_END_NAMESPACE
