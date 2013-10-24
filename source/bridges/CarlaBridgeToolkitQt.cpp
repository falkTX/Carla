/*
 * Carla Bridge Toolkit, Qt version
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaBridgeClient.hpp"
#include "CarlaBridgeToolkit.hpp"
#include "CarlaStyle.hpp"

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
# ifndef BRIDGE_X11
typedef QWidget QEmbedContainer;
# else
#  ifdef Q_WS_X11
typedef QX11EmbedContainer QEmbedContainer;
#  else
#   warning Using X11 UI bridge without QX11EmbedContainer
typedef QWidget QEmbedContainer;
#  endif
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

class CarlaBridgeToolkitQt: public QObject,
                            public CarlaBridgeToolkit
{
      Q_OBJECT

public:
    CarlaBridgeToolkitQt(CarlaBridgeClient* const client, const char* const uiTitle)
        : QObject(nullptr),
          CarlaBridgeToolkit(client, uiTitle),
          fApp(nullptr),
          fWindow(nullptr),
#ifdef BRIDGE_CONTAINER
          fEmbedContainer(nullptr),
#endif
          fMsgTimer(0)
    {
        carla_debug("CarlaBridgeToolkitQt::CarlaBridgeToolkitQt(%p, \"%s\")", client, uiTitle);

        connect(this, SIGNAL(setSizeSafeSignal(int,int)), SLOT(setSizeSafeSlot(int,int)));
    }

    ~CarlaBridgeToolkitQt() override
    {
        CARLA_ASSERT(fApp == nullptr);
        CARLA_ASSERT(fWindow == nullptr);
        CARLA_ASSERT(fMsgTimer == 0);
        carla_debug("CarlaBridgeToolkitQt::~CarlaBridgeToolkitQt()");
    }

    void init() override
    {
        CARLA_ASSERT(fApp == nullptr);
        CARLA_ASSERT(fWindow == nullptr);
        CARLA_ASSERT(fMsgTimer == 0);
        carla_debug("CarlaBridgeToolkitQt::init()");

        fApp = new QApplication(qargc, qargv);

        {
            QSettings settings("falkTX", "Carla");

            if (settings.value("Main/UseProTheme", true).toBool())
            {
                CarlaStyle* const style(new CarlaStyle());
                fApp->setStyle(style);
                //style->ready(fApp);

//                 QString color(settings.value("Main/ProThemeColor", "Black").toString());
//
//                 if (color == "System")
//                     pass(); //style->setColorScheme(CarlaStyle::COLOR_SYSTEM);
//                 else
//                     style->setColorScheme(CarlaStyle::COLOR_BLACK);
            }
        }

        fWindow = new QMainWindow(nullptr);
        fWindow->resize(30, 30);
        fWindow->hide();
    }

    void exec(const bool showGui) override
    {
        CARLA_ASSERT(kClient != nullptr);
        CARLA_ASSERT(fApp != nullptr);
        CARLA_ASSERT(fWindow != nullptr);
        carla_debug("CarlaBridgeToolkitQt::exec(%s)", bool2str(showGui));

#if defined(BRIDGE_QT4) || defined(BRIDGE_QT5)
        QWidget* const widget((QWidget*)kClient->getWidget());

        fWindow->setCentralWidget(widget);
        fWindow->adjustSize();

        widget->setParent(fWindow);
        widget->show();
#endif

        if (! kClient->isResizable())
        {
            fWindow->setFixedSize(fWindow->width(), fWindow->height());
#ifdef Q_OS_WIN
            fWindow->setWindowFlags(fWindow->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
        }

        fWindow->setWindowIcon(QIcon::fromTheme("carla", QIcon(":/scalable/carla.svg")));
        fWindow->setWindowTitle(kUiTitle);

        {
            QSettings settings("falkTX", appName);

            if (settings.contains(QString("%1/pos_x").arg(kUiTitle)))
            {
                bool hasX, hasY;
                const int posX(settings.value(QString("%1/pos_x").arg(kUiTitle), fWindow->x()).toInt(&hasX));
                const int posY(settings.value(QString("%1/pos_y").arg(kUiTitle), fWindow->y()).toInt(&hasY));

                if (hasX && hasY)
                    fWindow->move(posX, posY);

                if (kClient->isResizable())
                {
                    bool hasWidth, hasHeight;
                    const int width(settings.value(QString("%1/width").arg(kUiTitle), fWindow->width()).toInt(&hasWidth));
                    const int height(settings.value(QString("%1/height").arg(kUiTitle), fWindow->height()).toInt(&hasHeight));

                    if (hasWidth && hasHeight)
                        fWindow->resize(width, height);
                }
            }

            if (settings.value("Engine/UIsAlwaysOnTop", true).toBool())
                fWindow->setWindowFlags(fWindow->windowFlags() | Qt::WindowStaysOnTopHint);
        }

        if (showGui)
            show();
        else
            kClient->sendOscUpdate();

        fMsgTimer = startTimer(50);

        // First idle
        handleTimeout();

        // Main loop
        fApp->exec();
    }

    void quit() override
    {
        CARLA_ASSERT(kClient != nullptr);
        CARLA_ASSERT(fApp != nullptr);
        CARLA_ASSERT(fWindow != nullptr);
        carla_debug("CarlaBridgeToolkitQt::quit()");

        if (fMsgTimer != 0)
        {
            killTimer(fMsgTimer);
            fMsgTimer = 0;
        }

        if (fWindow != nullptr)
        {
            QSettings settings("falkTX", appName);
            settings.setValue(QString("%1/pos_x").arg(kUiTitle), fWindow->x());
            settings.setValue(QString("%1/pos_y").arg(kUiTitle), fWindow->y());
            settings.setValue(QString("%1/width").arg(kUiTitle), fWindow->width());
            settings.setValue(QString("%1/height").arg(kUiTitle), fWindow->height());
            settings.sync();

            fWindow->close();

#ifdef BRIDGE_CONTAINER
            if (fEmbedContainer != nullptr)
            {
                fEmbedContainer->close();

                delete fEmbedContainer;
                fEmbedContainer = nullptr;
            }
#endif

            delete fWindow;
            fWindow = nullptr;
        }

        if (fApp != nullptr)
        {
            if (! fApp->closingDown())
                fApp->quit();

            delete fApp;
            fApp = nullptr;
        }
    }

    void show() override
    {
        CARLA_ASSERT(fWindow != nullptr);
        carla_debug("CarlaBridgeToolkitQt::show()");

        if (fWindow != nullptr)
            fWindow->show();
    }

    void hide() override
    {
        CARLA_ASSERT(fWindow != nullptr);
        carla_debug("CarlaBridgeToolkitQt::hide()");

        if (fWindow != nullptr)
            fWindow->hide();
    }

    void resize(const int width, const int height) override
    {
        CARLA_ASSERT_INT(width > 0, width);
        CARLA_ASSERT_INT(height > 0, height);
        carla_debug("CarlaBridgeToolkitQt::resize(%i, %i)", width, height);

        if (width <= 0)
            return;
        if (height <= 0)
            return;

        emit setSizeSafeSignal(width, height);
    }

#ifdef BRIDGE_CONTAINER
    void* getContainerId()
    {
        CARLA_ASSERT(fWindow != nullptr);
        carla_debug("CarlaBridgeToolkitQt::getContainerId()");

        if (fEmbedContainer == nullptr)
        {
            fEmbedContainer = new QEmbedContainer(fWindow);

            fWindow->setCentralWidget(fEmbedContainer);
            fWindow->adjustSize();

            fEmbedContainer->setParent(fWindow);
            fEmbedContainer->show();
        }

        return (void*)fEmbedContainer->winId();
    }
#endif

protected:
    QApplication* fApp;
    QMainWindow* fWindow;

#ifdef BRIDGE_CONTAINER
    QEmbedContainer* fEmbedContainer;
#endif

    int fMsgTimer;

    void handleTimeout()
    {
        if (kClient == nullptr)
            return;

        kClient->uiIdle();

        if (! kClient->oscIdle())
        {
            killTimer(fMsgTimer);
            fMsgTimer = 0;
        }
    }

private:
    void timerEvent(QTimerEvent* const event)
    {
        if (event->timerId() == fMsgTimer)
            handleTimeout();

        QObject::timerEvent(event);
    }

signals:
    void setSizeSafeSignal(int, int);

private slots:
    void setSizeSafeSlot(int width, int height)
    {
        CARLA_ASSERT(kClient != nullptr && ! kClient->isResizable());
        CARLA_ASSERT(fWindow != nullptr);

        if (kClient == nullptr || fWindow == nullptr)
            return;

        if (kClient->isResizable())
            fWindow->resize(width, height);
        else
            fWindow->setFixedSize(width, height);

#ifdef BRIDGE_CONTAINER
        if (fEmbedContainer != nullptr)
            fEmbedContainer->setFixedSize(width, height);
#endif
    }
};

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include "CarlaBridgeToolkitQt.moc"
#else
# include "CarlaBridgeToolkitQt4.moc"
#endif

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const uiTitle)
{
    return new CarlaBridgeToolkitQt(client, uiTitle);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include "resources.cpp"
#else
# include "resources.qt4.cpp"
#endif

// -------------------------------------------------------------------------
