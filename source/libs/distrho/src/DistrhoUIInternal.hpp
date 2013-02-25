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

//#ifdef DISTRHO_UI_OPENGL
//# include "DistrhoUIOpenGL.h"
// START_NAMESPACE_DISTRHO
//# include "pugl/pugl.h"
// END_NAMESPACE_DISTRHO
//#else

# include "../DistrhoUIQt4.hpp"
# include <QtGui/QMouseEvent>
# include <QtGui/QResizeEvent>
# include <QtGui/QSizeGrip>
# include <QtGui/QVBoxLayout>
# ifdef Q_WS_X11
#  include <QtGui/QX11EmbedWidget>
# endif

//#endif

START_NAMESPACE_DISTRHO

// -------------------------------------------------

//#ifdef DISTRHO_UI_OPENGL
//typedef PuglView NativeWidget;
//#else
typedef QWidget  NativeWidget;
//#endif

typedef void (*editParamFunc) (void* ptr, uint32_t index, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t index, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*uiResizeFunc)  (void* ptr, unsigned int width, unsigned int height);

extern double d_lastUiSampleRate;

// -------------------------------------------------

//#ifdef DISTRHO_UI_QT4
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
//#endif

// -------------------------------------------------

struct UIPrivateData {
    // DSP
    double   sampleRate;
    uint32_t parameterOffset;

    // UI
    void*         ptr;
    NativeWidget* widget;

    // Callbacks
    editParamFunc editParamCallbackFunc;
    setParamFunc  setParamCallbackFunc;
    setStateFunc  setStateCallbackFunc;
    sendNoteFunc  sendNoteCallbackFunc;
    uiResizeFunc  uiResizeCallbackFunc;

    UIPrivateData()
        : sampleRate(d_lastUiSampleRate),
          parameterOffset(0),
          ptr(nullptr),
          widget(nullptr),
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          uiResizeCallbackFunc(nullptr)
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
        : kUi(createUI()),
          kData((kUi != nullptr) ? kUi->pData : nullptr),
#ifdef DISTRHO_UI_QT4
          qtMouseDown(false),
          qtGrip(nullptr),
          qtWidget(nullptr)
#else
          glInitiated(false)
#endif
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

        createWindow(winId);
    }

    ~UIInternal()
    {
        if (kUi != nullptr)
        {
            destroyWindow();
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
        assert(kUi != nullptr);

        if (kUi != nullptr)
            kUi->d_uiIdle();
    }

    intptr_t getWindowId()
    {
#ifdef DISTRHO_UI_QT4
        assert(qtWidget != nullptr);
        return (qtWidget != nullptr) ? qtWidget->winId() : 0;
#else
#endif
    }

    // ---------------------------------------------

    void createWindow(intptr_t parent)
    {
#ifdef DISTRHO_UI_QT4
        assert(kUi != nullptr);
        assert(kData != nullptr);
        assert(kData->widget != nullptr);
        assert(qtGrip == nullptr);
        assert(qtWidget == nullptr);

        if (kUi == nullptr)
            return;
        if (kData == nullptr)
            return;
        if (kData->widget == nullptr)
            return;
        if (qtGrip != nullptr)
            return;
        if (qtWidget != nullptr)
            return;

        qtMouseDown = false;

        // create embedable widget
        qtWidget = new QEmbedWidget;

        // set layout
        qtWidget->setLayout(new QVBoxLayout(qtWidget));
        qtWidget->layout()->addWidget(kData->widget);
        qtWidget->layout()->setContentsMargins(0, 0, 0, 0);
        qtWidget->setFixedSize(kUi->d_width(), kUi->d_height());

        // set resize grip
        if (((Qt4UI*)kUi)->d_resizable())
        {
            // listen for resize on the plugin widget
            kData->widget->installEventFilter(this);

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
#else
#endif
    }

    void destroyWindow()
    {
#ifdef DISTRHO_UI_QT4
        assert(kData != nullptr);
        assert(kData->widget != nullptr);
        assert(qtWidget != nullptr);

        if (kData == nullptr)
            return;
        if (kData->widget == nullptr)
            return;
        if (qtWidget == nullptr)
            return;

        // remove main widget, to prevent it from being auto-deleted
        kData->widget->hide();
        qtWidget->layout()->removeWidget(kData->widget);
        kData->widget->setParent(nullptr);
        kData->widget->close();

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
#else
#endif
    }

    // ---------------------------------------------

protected:
    UI* const kUi;
    UIPrivateData* const kData;

#ifdef DISTRHO_UI_QT4
    // FIXME - remove qtMouseDown usage
    bool eventFilter(QObject* obj, QEvent* event)
    {
        assert(kUi != nullptr);
        assert(kData != nullptr);
        assert(kData->widget != nullptr);
        assert(qtGrip != nullptr);
        assert(qtWidget != nullptr);

        if (kUi == nullptr)
            return false;
        if (kData == nullptr)
            return false;
        if (kData->widget == nullptr)
            return false;
        if (qtGrip == nullptr)
            return false;
        if (qtWidget == nullptr)
            return false;

        if (obj == nullptr)
        {
            // nothing
        }
        else if (obj == qtGrip)
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                QMouseEvent* mEvent = (QMouseEvent*)event;
                if (mEvent->button() == Qt::LeftButton)
                    qtMouseDown = true;
            }
            else if (event->type() == QEvent::MouseMove)
            {
                if (qtMouseDown)
                {
                    Qt4UI* qtUi = (Qt4UI*)kUi;
                    QMouseEvent* mEvent = (QMouseEvent*)event;
                    unsigned int width  = qtUi->d_width()  + mEvent->x() - qtGrip->width();
                    unsigned int height = qtUi->d_height() + mEvent->y() - qtGrip->height();

                    if (width < qtUi->d_minimumWidth())
                        width = qtUi->d_minimumWidth();
                    if (height < qtUi->d_minimumHeight())
                        height = qtUi->d_minimumHeight();

                    kData->widget->setFixedSize(width, height);

                    return true;
                }
            }
            else if (event->type() == QEvent::MouseButtonRelease)
            {
                QMouseEvent* mEvent = (QMouseEvent*)event;
                if (mEvent->button() == Qt::LeftButton)
                    qtMouseDown = false;
            }
        }
        else if (obj == kData->widget && event->type() == QEvent::Resize)
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

private:
#ifdef DISTRHO_UI_QT4
    bool qtMouseDown;
    QSizeGrip* qtGrip;
    QEmbedWidget* qtWidget;
#else
    bool glInitiated;
#endif
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_INTERNAL_HPP__
