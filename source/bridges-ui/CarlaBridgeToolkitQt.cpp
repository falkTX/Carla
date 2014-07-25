/*
 * Carla Bridge Toolkit, Qt version
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
#include "CarlaStyle.hpp"

#include <QtCore/QThread>
#include <QtCore/QTimerEvent>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QApplication>
# include <QtWidgets/QMainWindow>
#else
# include <QtGui/QApplication>
# include <QtGui/QMainWindow>
#endif

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

static int   qargc = 0;
static char* qargv[0] = {};

// -------------------------------------------------------------------------

class CarlaBridgeToolkitQt: public QObject,
                            public CarlaBridgeToolkit
{
      Q_OBJECT

public:
    CarlaBridgeToolkitQt(CarlaBridgeClient* const client, const char* const windowTitle)
        : QObject(nullptr),
          CarlaBridgeToolkit(client, windowTitle),
          fApp(nullptr),
          fWindow(nullptr),
          fMsgTimer(0),
          fNeedsShow(false)
    {
        carla_debug("CarlaBridgeToolkitQt::CarlaBridgeToolkitQt(%p, \"%s\")", client, windowTitle);

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

        fWindow = new QMainWindow(nullptr);
        fWindow->resize(30, 30);
        fWindow->hide();
    }

    void exec(const bool showUI) override
    {
        CARLA_ASSERT(kClient != nullptr);
        CARLA_ASSERT(fApp != nullptr);
        CARLA_ASSERT(fWindow != nullptr);
        carla_debug("CarlaBridgeToolkitQt::exec(%s)", bool2str(showUI));

        QWidget* const widget((QWidget*)kClient->getWidget());

        fWindow->setCentralWidget(widget);
        fWindow->adjustSize();

        widget->setParent(fWindow);
        widget->show();

        if (! kClient->isResizable())
        {
            fWindow->setFixedSize(fWindow->width(), fWindow->height());
#ifdef Q_OS_WIN
            fWindow->setWindowFlags(fWindow->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
        }

        fWindow->setWindowIcon(QIcon::fromTheme("carla", QIcon(":/scalable/carla.svg")));
        fWindow->setWindowTitle(kWindowTitle.buffer());

        if (showUI || fNeedsShow)
        {
            show();
            fNeedsShow = false;
        }

        fMsgTimer = startTimer(30);

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
            fWindow->close();

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
        carla_debug("CarlaBridgeToolkitQt::show()");

        fNeedsShow = true;

        if (fWindow != nullptr)
            fWindow->show();
    }

    void hide() override
    {
        carla_debug("CarlaBridgeToolkitQt::hide()");

        fNeedsShow = false;

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

protected:
    QApplication* fApp;
    QMainWindow* fWindow;

    int  fMsgTimer;
    bool fNeedsShow;

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
    }
};

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include "CarlaBridgeToolkitQt5.moc"
#else
# include "CarlaBridgeToolkitQt4.moc"
#endif

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const windowTitle)
{
    return new CarlaBridgeToolkitQt(client, windowTitle);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

// -------------------------------------------------------------------------

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include "resources.qt5.cpp"
#else
# include "resources.qt4.cpp"
#endif

// -------------------------------------------------------------------------
