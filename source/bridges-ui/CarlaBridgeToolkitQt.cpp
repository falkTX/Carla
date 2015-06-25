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

#include "CarlaBridgeUI.hpp"
#include "CarlaBridgeToolkit.hpp"
#include "CarlaStyle.hpp"

#include <QtCore/QThread>
#include <QtCore/QTimerEvent>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
# include <QtWidgets/QApplication>
# include <QtWidgets/QMainWindow>
#else
# include <QtGui/QApplication>
# include <QtGui/QMainWindow>
#endif

#if defined(CARLA_OS_LINUX) && defined(HAVE_X11) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
# define USE_CUSTOM_X11_METHODS
# include <QtGui/QX11Info>
# include <X11/Xlib.h>
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
    CarlaBridgeToolkitQt(CarlaBridgeUI* const u)
        : QObject(nullptr),
          CarlaBridgeToolkit(u),
          fApp(nullptr),
          fWindow(nullptr),
          fMsgTimer(0),
          fNeedsShow(false)
    {
        carla_debug("CarlaBridgeToolkitQt::CarlaBridgeToolkitQt(%p)", u);
    }

    ~CarlaBridgeToolkitQt() override
    {
        CARLA_SAFE_ASSERT(fApp == nullptr);
        CARLA_SAFE_ASSERT(fWindow == nullptr);
        CARLA_SAFE_ASSERT(fMsgTimer == 0);
        carla_debug("CarlaBridgeToolkitQt::~CarlaBridgeToolkitQt()");
    }

    bool init(const int /*argc*/, const char** /*argv[]*/) override
    {
        CARLA_SAFE_ASSERT_RETURN(fApp == nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fWindow == nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fMsgTimer == 0, false);
        carla_debug("CarlaBridgeToolkitQt::init()");

        fApp = new QApplication(qargc, qargv);

        fWindow = new QMainWindow(nullptr);
        fWindow->resize(30, 30);
        fWindow->hide();

        return true;
    }

    void exec(const bool showUI) override
    {
        CARLA_SAFE_ASSERT_RETURN(ui != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fApp != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        carla_debug("CarlaBridgeToolkitQt::exec(%s)", bool2str(showUI));

        const CarlaBridgeUI::Options& options(ui->getOptions());

        QWidget* const widget((QWidget*)ui->getWidget());

        fWindow->setCentralWidget(widget);
        fWindow->adjustSize();

        widget->setParent(fWindow);
        widget->show();

        if (! options.isResizable)
        {
            fWindow->setFixedSize(fWindow->width(), fWindow->height());
#ifdef CARLA_OS_WIN
            fWindow->setWindowFlags(fWindow->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
        }

        fWindow->setWindowIcon(QIcon::fromTheme("carla", QIcon(":/scalable/carla.svg")));
        fWindow->setWindowTitle(options.windowTitle.buffer());

        if (options.transientWindowId != 0)
        {
#ifdef USE_CUSTOM_X11_METHODS
            XSetTransientForHint(QX11Info::display(), static_cast< ::Window>(fWindow->winId()), static_cast< ::Window>(options.transientWindowId));
#endif
        }

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
        CARLA_SAFE_ASSERT_RETURN(ui != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fApp != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
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

    void focus() override
    {
        carla_debug("CarlaBridgeToolkitQt::focus()");
    }

    void hide() override
    {
        carla_debug("CarlaBridgeToolkitQt::hide()");

        fNeedsShow = false;

        if (fWindow != nullptr)
            fWindow->hide();
    }

    void setSize(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(ui != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(width > 0,);
        CARLA_SAFE_ASSERT_RETURN(height > 0,);
        carla_debug("CarlaBridgeToolkitQt::resize(%i, %i)", width, height);

        if (ui->getOptions().isResizable)
            fWindow->resize(width, height);
        else
            fWindow->setFixedSize(width, height);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(title != nullptr && title[0] != '\0',);
        carla_debug("CarlaBridgeToolkitQt::setTitle(\"%s\")", title);

        fWindow->setWindowTitle(title);
    }

protected:
    QApplication* fApp;
    QMainWindow* fWindow;

    int  fMsgTimer;
    bool fNeedsShow;

    void handleTimeout()
    {
        CARLA_SAFE_ASSERT_RETURN(ui != nullptr,);

        if (ui->isPipeRunning())
            ui->idlePipe();

        ui->idleUI();
    }

private:
    void timerEvent(QTimerEvent* const ev) override
    {
        if (ev->timerId() == fMsgTimer)
            handleTimeout();

        QObject::timerEvent(ev);
    }

#ifndef MOC_PARSING
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeToolkitQt)
#endif
};

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include "CarlaBridgeToolkitQt5.moc"
#else
# include "CarlaBridgeToolkitQt4.moc"
#endif

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeUI* const ui)
{
    return new CarlaBridgeToolkitQt(ui);
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

// -------------------------------------------------------------------------

// missing declaration
int qInitResources();
int qCleanupResources();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
# include "resources.qt5.cpp"
#else
# include "resources.qt4.cpp"
#endif

// -------------------------------------------------------------------------
