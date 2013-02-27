/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DISTRHO_UI_INTERNAL_HPP__
#define __DISTRHO_UI_INTERNAL_HPP__

#include "DistrhoDefines.h"

#ifdef DISTRHO_UI_OPENGL
# include "../DistrhoUIOpenGL.hpp"
# include "../dgl/App.hpp"
# include "../dgl/Window.hpp"
#else
# include "../DistrhoUIQt4.hpp"
# include <QtGui/QMouseEvent>
# include <QtGui/QResizeEvent>
# include <QtGui/QSizeGrip>
# include <QtGui/QVBoxLayout>
# ifdef Q_WS_X11
#  include <QtGui/QX11EmbedWidget>
# endif
#endif

#include <cassert>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

typedef void (*editParamFunc) (void* ptr, uint32_t index, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t index, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*uiResizeFunc)  (void* ptr, unsigned int width, unsigned int height);

extern double d_lastUiSampleRate;

// -------------------------------------------------

#ifdef DISTRHO_UI_QT4
# ifdef Q_WS_X11
class QEmbedWidget : public QX11EmbedWidget
# else
class QEmbedWidget : public QWidget
# endif
{
public:
    QEmbedWidget();
    ~QEmbedWidget();

    void embedInto(WId id);
    WId containerWinId() const;
};
#endif

// -------------------------------------------------

struct UIPrivateData {
    // DSP
    double   sampleRate;
    uint32_t parameterOffset;

    // Callbacks
    editParamFunc editParamCallbackFunc;
    setParamFunc  setParamCallbackFunc;
    setStateFunc  setStateCallbackFunc;
    sendNoteFunc  sendNoteCallbackFunc;
    uiResizeFunc  uiResizeCallbackFunc;
    void*         ptr;

    UIPrivateData()
        : sampleRate(d_lastUiSampleRate),
          parameterOffset(0),
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          uiResizeCallbackFunc(nullptr),
          ptr(nullptr)
    {
        assert(d_lastUiSampleRate != 0.0);
    }

    ~UIPrivateData()
    {
    }

    void editParamCallback(uint32_t index, bool started)
    {
        if (editParamCallbackFunc != nullptr)
            editParamCallbackFunc(ptr, index, started);
    }

    void setParamCallback(uint32_t rindex, float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(ptr, rindex, value);
    }

    void setStateCallback(const char* key, const char* value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(ptr, key, value);
    }

    void sendNoteCallback(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(ptr, onOff, channel, note, velocity);
    }

    void uiResizeCallback(unsigned int width, unsigned int height)
    {
        if (uiResizeCallbackFunc != nullptr)
            uiResizeCallbackFunc(ptr, width, height);
    }
};

// -------------------------------------------------

#ifdef DISTRHO_UI_QT4
class UIInternal : public QObject // needed for eventFilter()
#else
class UIInternal
#endif
{
public:
    UIInternal(void* ptr, intptr_t winId, editParamFunc editParamCall, setParamFunc setParamCall, setStateFunc setStateCall, sendNoteFunc sendNoteCall, uiResizeFunc uiResizeCall)
#ifdef DISTRHO_UI_QT4
        : qtGrip(nullptr),
          qtWidget(nullptr),
#else
        : glApp(),
          glWindow(&glApp, winId),
#endif
          kUi(createUI()),
          kData((kUi != nullptr) ? kUi->pData : nullptr)
    {
        assert(kUi != nullptr);
        assert(winId != 0);

        if (kUi == nullptr || winId == 0)
            return;

        kData->ptr = ptr;
        kData->editParamCallbackFunc = editParamCall;
        kData->setParamCallbackFunc  = setParamCall;
        kData->setStateCallbackFunc  = setStateCall;
        kData->sendNoteCallbackFunc  = sendNoteCall;
        kData->uiResizeCallbackFunc  = uiResizeCall;

#ifdef DISTRHO_UI_QT4
        createWindow(winId);
#endif
    }

    ~UIInternal()
    {
        if (kUi != nullptr)
        {
#ifdef DISTRHO_UI_QT4
            destroyWindow();
#endif
            delete kUi;
        }
    }

    // ---------------------------------------------

    const char* name() const
    {
        assert(kUi != nullptr);
        return (kUi != nullptr) ? kUi->d_name() : "";
    }

    unsigned int width()
    {
        assert(kUi != nullptr);
        return (kUi != nullptr) ? kUi->d_width() : 0;
    }

    unsigned int height()
    {
        assert(kUi != nullptr);
        return (kUi != nullptr) ? kUi->d_height() : 0;
    }

    // ---------------------------------------------

    void parameterChanged(uint32_t index, float value)
    {
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programChanged(uint32_t index)
    {
      assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_programChanged(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value)
    {
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_stateChanged(key, value);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_noteReceived(onOff, channel, note, velocity);
    }
#endif

    // ---------------------------------------------

    void idle()
    {
#ifdef DISTRHO_UI_QT4
        kUi->d_uiIdle();
#else
        glApp.idle();
#endif
    }

    intptr_t getWinId()
    {
#ifdef DISTRHO_UI_QT4
        assert(qtWidget != nullptr);
        return (qtWidget != nullptr) ? qtWidget->winId() : 0;
#else
        return glWindow.getWindowId();
#endif
    }

    // ---------------------------------------------

#ifdef DISTRHO_UI_QT4
    void createWindow(intptr_t parent)
    {
        assert(kUi != nullptr);
        assert(kData != nullptr);
        assert(qtGrip == nullptr);
        assert(qtWidget == nullptr);

        if (kUi == nullptr)
            return;
        if (kData == nullptr)
            return;
        if (qtGrip != nullptr)
            return;
        if (qtWidget != nullptr)
            return;

        Qt4UI* qt4Ui = (Qt4UI*)kUi;

        // create embedable widget
        qtWidget = new QEmbedWidget;

        // set layout
        qtWidget->setLayout(new QVBoxLayout(qtWidget));
        qtWidget->layout()->addWidget(qt4Ui);
        qtWidget->layout()->setContentsMargins(0, 0, 0, 0);
        qtWidget->setFixedSize(kUi->d_width(), kUi->d_height());

        // set resize grip
        if (qt4Ui->d_resizable())
        {
            // listen for resize on the plugin widget
            qt4Ui->installEventFilter(this);

            // create resize grip on bottom-right
            qtGrip = new QSizeGrip(qtWidget);
            qtGrip->resize(qtGrip->sizeHint());
            qtGrip->setCursor(Qt::SizeFDiagCursor);
            qtGrip->move(kUi->d_width() - qtGrip->width(), kUi->d_height() - qtGrip->height());
            qtGrip->show();
            qtGrip->raise();
            qtGrip->installEventFilter(this);
        }

        // reparent widget
        qtWidget->embedInto(parent);

        // show it
        qtWidget->show();
    }

    void destroyWindow()
    {
        assert(kData != nullptr);
        assert(qtWidget != nullptr);

        if (kData == nullptr)
            return;
        if (qtWidget == nullptr)
            return;

        Qt4UI* qt4Ui = (Qt4UI*)kUi;

        // remove main widget, to prevent it from being auto-deleted
        qt4Ui->hide();
        qtWidget->layout()->removeWidget(qt4Ui);
        qt4Ui->setParent(nullptr);
        qt4Ui->close();

        qtWidget->close();
        qtWidget->removeEventFilter(this);

        if (qtGrip != nullptr)
        {
            qtGrip->removeEventFilter(this);
            delete qtGrip;
            qtGrip = nullptr;
        }

        delete qtWidget;
        qtWidget = nullptr;
    }
#endif

    // ---------------------------------------------

private:
#ifdef DISTRHO_UI_QT4
    QSizeGrip*    qtGrip;
    QEmbedWidget* qtWidget;
#else
    App    glApp;
    Window glWindow;
#endif

protected:
    UI* const kUi;
    UIPrivateData* const kData;

#ifdef DISTRHO_UI_QT4
    bool eventFilter(QObject* obj, QEvent* event)
    {
        assert(kUi != nullptr);
        assert(kData != nullptr);
        //assert(qtGrip != nullptr);
        assert(qtWidget != nullptr);

        if (kUi == nullptr)
            return false;
        if (kData == nullptr)
            return false;
        if (qtGrip == nullptr)
            return false;
        if (qtWidget == nullptr)
            return false;

        Qt4UI* qt4Ui = (Qt4UI*)kUi;

        if (obj == nullptr)
        {
            // nothing
        }
        else if (obj == qtGrip)
        {
            if (event->type() == QEvent::MouseMove)
            {
                QMouseEvent* mEvent = (QMouseEvent*)event;

                if (mEvent->button() == Qt::LeftButton)
                {
                    unsigned int width  = qt4Ui->d_width()  + mEvent->x() - qtGrip->width();
                    unsigned int height = qt4Ui->d_height() + mEvent->y() - qtGrip->height();

                    if (width < qt4Ui->d_minimumWidth())
                        width = qt4Ui->d_minimumWidth();
                    if (height < qt4Ui->d_minimumHeight())
                        height = qt4Ui->d_minimumHeight();

                    qt4Ui->setFixedSize(width, height);

                    return true;
                }
            }
        }
        else if (obj == qt4Ui && event->type() == QEvent::Resize)
        {
            QResizeEvent* rEvent = (QResizeEvent*)event;
            const QSize&  size   = rEvent->size();

            qtWidget->setFixedSize(size.width(), size.height());
            qtGrip->move(size.width() - qtGrip->width(), size.height() - qtGrip->height());

            kUi->d_uiResize(size.width(), size.height());
        }

        return QObject::eventFilter(obj, event);
    }
#endif
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_INTERNAL_HPP__
