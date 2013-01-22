/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the license see the GPL.txt file
 */

#ifndef __DISTRHO_UI_INTERNAL_H__
#define __DISTRHO_UI_INTERNAL_H__

#include "DistrhoUI.h"

#ifdef DISTRHO_UI_OPENGL
# include "DistrhoUIOpenGL.h"
START_NAMESPACE_DISTRHO
# include "pugl/pugl.h"
END_NAMESPACE_DISTRHO
#else
# include "DistrhoUIQt4.h"
# include <QtGui/QApplication>
# include <QtGui/QMouseEvent>
# include <QtGui/QSizeGrip>
# include <QtGui/QVBoxLayout>
# ifdef Q_WS_X11
#  include <QtGui/QX11EmbedWidget>
# endif
#endif

#include <cassert>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

#ifdef DISTRHO_UI_OPENGL
typedef PuglView NativeWidget;
#else
typedef QWidget  NativeWidget;
#endif

typedef void (*setParamFunc)    (void* ptr, uint32_t index, float value);
typedef void (*setStateFunc)    (void* ptr, const char* key, const char* value);
typedef void (*uiEditParamFunc) (void* ptr, uint32_t index, bool started);
typedef void (*uiSendNoteFunc)  (void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*uiResizeFunc)    (void* ptr, unsigned int width, unsigned int height);

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

    // UI
    void*         ptr;
    NativeWidget* widget;
    /* ^this:
     * Under Qt4 it points to the UI itself (in this case Qt4UI),
     *  It's set right on the Qt4UI constructor.
     *
     * Under OpenGL it starts NULL and its created in createWindow().
     *  It points to a puglView.
     */

    // Callbacks
    setParamFunc    setParamCallbackFunc;
    setStateFunc    setStateCallbackFunc;
    uiEditParamFunc uiEditParamCallbackFunc;
    uiSendNoteFunc  uiSendNoteCallbackFunc;
    uiResizeFunc    uiResizeCallbackFunc;

    UIPrivateData()
        : sampleRate(d_lastUiSampleRate),
          parameterOffset(0),
          ptr(nullptr),
          widget(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          uiEditParamCallbackFunc(nullptr),
          uiSendNoteCallbackFunc(nullptr),
          uiResizeCallbackFunc(nullptr)
    {
        assert(d_lastUiSampleRate != 0.0);
    }

    ~UIPrivateData()
    {
    }

    void setParamCallback(uint32_t rindex, float value)
    {
        if (setParamCallbackFunc)
            setParamCallbackFunc(ptr, rindex, value);
    }

    void setStateCallback(const char* key, const char* value)
    {
        if (setStateCallbackFunc)
            setStateCallbackFunc(ptr, key, value);
    }

    void uiEditParamCallback(uint32_t index, bool started)
    {
        if (uiEditParamCallbackFunc)
            uiEditParamCallbackFunc(ptr, index, started);
    }

    void uiSendNoteCallback(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        if (uiSendNoteCallbackFunc)
            uiSendNoteCallbackFunc(ptr, onOff, channel, note, velocity);
    }

    void uiResizeCallback(unsigned int width, unsigned int height)
    {
        if (uiResizeCallbackFunc)
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
    UIInternal(void* ptr, intptr_t winId, setParamFunc setParamCall, setStateFunc setStateCall, uiEditParamFunc uiEditParamCall, uiSendNoteFunc uiSendNoteCall, uiResizeFunc uiResizeCall)
        : ui(createUI()),
          data(ui ? ui->data : nullptr)
    {
        assert(ui);

#ifdef DISTRHO_UI_QT4
        qt_grip   = nullptr;
        qt_widget = nullptr;
#else
        gl_initiated = false;
#endif

        if (winId == 0 || ! ui)
            return;

        data->ptr = ptr;
        data->setParamCallbackFunc    = setParamCall;
        data->setStateCallbackFunc    = setStateCall;
        data->uiEditParamCallbackFunc = uiEditParamCall;
        data->uiSendNoteCallbackFunc  = uiSendNoteCall;
        data->uiResizeCallbackFunc    = uiResizeCall;

        createWindow(winId);
    }

    ~UIInternal()
    {
        if (ui)
        {
            destroyWindow();
            delete ui;
        }
    }

    // ---------------------------------------------

    void idle()
    {
        assert(ui);

        if (ui)
            ui->d_uiIdle();
    }

    unsigned int getWidth()
    {
        assert(ui);
        return ui ? ui->d_width() : 0;
    }

    unsigned int getHeight()
    {
        assert(ui);
        return ui ? ui->d_height() : 0;
    }

    intptr_t getWindowId()
    {
#ifdef DISTRHO_UI_QT4
        assert(qt_widget);
        return qt_widget ? qt_widget->winId() : 0;
#else
        assert(data && data->widget);
        return (data && data->widget) ? puglGetNativeWindow(data->widget) : 0;
#endif
    }

    // ---------------------------------------------

    void parameterChanged(uint32_t index, float value)
    {
        assert(ui);

        if (ui)
            ui->d_parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programChanged(uint32_t index)
    {
      assert(ui);

        if (ui)
            ui->d_programChanged(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* key, const char* value)
    {
        assert(ui);

        if (ui)
            ui->d_stateChanged(key, value);
    }
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
    void noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        assert(ui);

        if (ui)
            ui->d_uiNoteReceived(onOff, channel, note, velocity);
    }
#endif

    // ---------------------------------------------

    void createWindow(intptr_t parent)
    {
#ifdef DISTRHO_UI_QT4
        assert(ui && data && ! qt_widget);

        if (qt_widget || ! (ui && data))
            return;

        qt_mouseDown = false;

        // create embedable widget
        qt_widget = new QEmbedWidget;

        // set layout
        qt_widget->setLayout(new QVBoxLayout(qt_widget));
        qt_widget->layout()->addWidget(data->widget);
        qt_widget->layout()->setContentsMargins(0, 0, 0, 0);
        qt_widget->setFixedSize(ui->d_width(), ui->d_height());

        // listen for resize on the plugin widget
        data->widget->installEventFilter(this);

        // set resize grip
        if (((Qt4UI*)ui)->d_resizable())
        {
            qt_grip = new QSizeGrip(qt_widget);
            qt_grip->resize(qt_grip->sizeHint());
            qt_grip->setCursor(Qt::SizeFDiagCursor);
            qt_grip->move(ui->d_width() - qt_grip->width(), ui->d_height() - qt_grip->height());
            qt_grip->show();
            qt_grip->raise();
            qt_grip->installEventFilter(this);
        }

        // reparent widget
        qt_widget->embedInto(parent);

        // show it
        qt_widget->show();
#else
        assert(ui && data && ! data->widget);

        if ((! data) || data->widget || ! ui)
            return;

        data->widget = puglCreate(parent, DISTRHO_PLUGIN_NAME, ui->d_width(), ui->d_height(), false);

        assert(data->widget);

        if (! data->widget)
            return;

        puglSetHandle(data->widget, this);
        puglSetDisplayFunc(data->widget, gl_onDisplayCallback);
        puglSetKeyboardFunc(data->widget, gl_onKeyboardCallback);
        puglSetMotionFunc(data->widget, gl_onMotionCallback);
        puglSetMouseFunc(data->widget, gl_onMouseCallback);
        puglSetScrollFunc(data->widget, gl_onScrollCallback);
        puglSetSpecialFunc(data->widget, gl_onSpecialCallback);
        puglSetReshapeFunc(data->widget, gl_onReshapeCallback);
        puglSetCloseFunc(data->widget, gl_onCloseCallback);
#endif
    }

    void destroyWindow()
    {
#ifdef DISTRHO_UI_QT4
        if (qt_widget)
        {
            // remove main widget, to prevent it from being auto-deleted
            data->widget->hide();
            qt_widget->layout()->removeWidget(data->widget);
            data->widget->setParent(nullptr);
            data->widget->close();

            qt_widget->close();

            if (qt_grip)
            {
                delete qt_grip;
                qt_grip = nullptr;
            }

            delete qt_widget;
            qt_widget = nullptr;
        }
#else
        ((OpenGLUI*)ui)->d_onClose();

        if (data && data->widget)
        {
            puglDestroy(data->widget);
            data->widget = nullptr;
        }
#endif
    }

    // ---------------------------------------------

#ifdef DISTRHO_UI_OPENGL
    void gl_onDisplay()
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
            uiGL->d_onDisplay();
    }

    void gl_onKeyboard(bool press, uint32_t key)
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
            uiGL->d_onKeyboard(press, key);
    }

    void gl_onMotion(int x, int y)
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
            uiGL->d_onMotion(x, y);
    }

    void gl_onMouse(int button, bool press, int x, int y)
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
            uiGL->d_onMouse(button, press, x, y);
    }

    void gl_onReshape(int width, int height)
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
        {
            if (! gl_initiated)
            {
                uiGL->d_onInit();
                gl_initiated = true;
            }
            else
                uiGL->d_onReshape(width, height);
        }
    }

    void gl_onScroll(float dx, float dy)
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
            uiGL->d_onScroll(dx, dy);
    }

    void gl_onSpecial(bool press, Key key)
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
            uiGL->d_onSpecial(press, key);
    }

    void gl_onClose()
    {
        if (OpenGLUI* uiGL = (OpenGLUI*)ui)
            uiGL->d_onClose();
    }

    // ---------------------------------------------
private:

    static void gl_onDisplayCallback(PuglView* view)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onDisplay();
    }

    static void gl_onKeyboardCallback(PuglView* view, bool press, uint32_t key)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onKeyboard(press, key);
    }

    static void gl_onMotionCallback(PuglView* view, int x, int y)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onMotion(x, y);
    }

    static void gl_onMouseCallback(PuglView* view, int button, bool press, int x, int y)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onMouse(button, press, x, y);
    }

    static void gl_onReshapeCallback(PuglView* view, int width, int height)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onReshape(width, height);
    }

    static void gl_onScrollCallback(PuglView* view, float dx, float dy)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onScroll(dx, dy);
    }

    static void gl_onSpecialCallback(PuglView* view, bool press, PuglKey key)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onSpecial(press, (Key)key);
    }

    static void gl_onCloseCallback(PuglView* view)
    {
        if (UIInternal* _this_ = (UIInternal*)puglGetHandle(view))
            _this_->gl_onClose();
    }
#endif

    // ---------------------------------------------

protected:
    UI* const ui;
    UIPrivateData* const data;

#ifdef DISTRHO_UI_QT4
    bool eventFilter(QObject* obj, QEvent* event)
    {
        if (obj == qt_grip)
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                QMouseEvent* mEvent = (QMouseEvent*)event;
                if (mEvent->button() == Qt::LeftButton)
                    qt_mouseDown = true;
                return true;
            }

            else if (event->type() == QEvent::MouseMove)
            {
                if (qt_mouseDown)
                {
                    Qt4UI* qt_ui = (Qt4UI*)ui;
                    QMouseEvent* mEvent = (QMouseEvent*)event;
                    int width  = ui->d_width()  + mEvent->x() - qt_grip->width();
                    int height = ui->d_height() + mEvent->y() - qt_grip->height();

                    if (width < qt_ui->d_minimumWidth())
                        width = qt_ui->d_minimumWidth();
                    if (height < qt_ui->d_minimumHeight())
                        height = qt_ui->d_minimumHeight();

                    if (data && data->widget)
                        data->widget->setFixedSize(width, height);

                    return true;
                }

                // FIXME?
                //return true;
            }

            else if (event->type() == QEvent::MouseButtonRelease)
            {
                QMouseEvent* mEvent = (QMouseEvent*)event;
                if (mEvent->button() == Qt::LeftButton)
                    qt_mouseDown = false;
                return true;
            }
        }
        else if (data && obj == data->widget)
        {
            if (event->type() == QEvent::Resize)
            {
                QResizeEvent* rEvent = (QResizeEvent*)event;
                const QSize&  size   = rEvent->size();

                qt_widget->setFixedSize(size.width(), size.height());
                qt_grip->move(size.width() - qt_grip->width(), size.height() - qt_grip->height());

                ui->d_uiResize(size.width(), size.height());
            }
        }

        return QObject::eventFilter(obj, event);
    }
#endif

private:
#ifdef DISTRHO_UI_QT4
    bool qt_mouseDown;
    QSizeGrip* qt_grip;
    QEmbedWidget* qt_widget;
#else
    bool gl_initiated;
#endif
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_INTERNAL_H__
