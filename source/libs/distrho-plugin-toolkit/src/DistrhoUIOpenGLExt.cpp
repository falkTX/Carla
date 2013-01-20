/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoDefines.h"

#ifdef DISTRHO_UI_OPENGL

#include "DistrhoUIOpenGLExt.h"

#include <cassert>
#include <cmath>
#include <vector>

#if DISTRHO_OS_LINUX
# include "pugl/pugl_x11.h"
#endif

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Point

Point::Point(int x, int y)
    : _x(x),
      _y(y)
{
}

Point::Point(const Point& pos)
    : _x(pos._x),
      _y(pos._y)
{
}

int Point::getX() const
{
    return _x;
}

int Point::getY() const
{
    return _y;
}

void Point::setX(int x)
{
    _x = x;
}

void Point::setY(int y)
{
    _y = y;
}

Point& Point::operator=(const Point& pos)
{
    _x = pos._x;
    _y = pos._y;
    return *this;
}

Point& Point::operator+=(const Point& pos)
{
    _x += pos._x;
    _y += pos._y;
    return *this;
}

Point& Point::operator-=(const Point& pos)
{
    _x -= pos._x;
    _y -= pos._y;
    return *this;
}

bool Point::operator==(const Point& pos) const
{
    return (_x == pos._x && _y == pos._y);
}

bool Point::operator!=(const Point& pos) const
{
    return !operator==(pos);
}

// -------------------------------------------------
// Point

Size::Size(int width, int height)
    : _width(width),
      _height(height)
{
}

Size::Size(const Size& size)
    : _width(size._width),
      _height(size._height)
{
}

int Size::getWidth() const
{
    return _width;
}

int Size::getHeight() const
{
    return _height;
}

void Size::setWidth(int width)
{
    _width = width;
}

void Size::setHeight(int height)
{
    _height = height;
}

Size& Size::operator=(const Size& size)
{
    _width  = size._width;
    _height = size._height;
    return *this;
}

Size& Size::operator+=(const Size& size)
{
    _width  += size._width;
    _height += size._height;
    return *this;
}

Size& Size::operator-=(const Size& size)
{
    _width  -= size._width;
    _height -= size._height;
    return *this;
}

Size& Size::operator*=(int m)
{
    _width  *= m;
    _height *= m;
    return *this;
}

Size& Size::operator/=(int d)
{
    _width  /= d;
    _height /= d;
    return *this;
}

Size& Size::operator*=(float m)
{
    _width  *= m;
    _height *= m;
    return *this;
}

Size& Size::operator/=(float d)
{
    _width  /= d;
    _height /= d;
    return *this;
}

// -------------------------------------------------
// Rectangle

Rectangle::Rectangle(int x, int y, int width, int height)
    : _pos(x, y),
      _size(width, height)
{
}

Rectangle::Rectangle(int x, int y, const Size& size)
    : _pos(x, y),
      _size(size)
{
}

Rectangle::Rectangle(const Point& pos, int width, int height)
    : _pos(pos),
      _size(width, height)
{
}

Rectangle::Rectangle(const Point& pos, const Size& size)
    : _pos(pos),
      _size(size)
{
}

Rectangle::Rectangle(const Rectangle& rect)
    : _pos(rect._pos),
      _size(rect._size)
{
}

int Rectangle::getX() const
{
    return _pos._x;
}

int Rectangle::getY() const
{
    return _pos._y;
}

int Rectangle::getWidth() const
{
    return _size._width;
}

int Rectangle::getHeight() const
{
    return _size._height;
}

const Point& Rectangle::getPos() const
{
    return _pos;
}

const Size& Rectangle::getSize() const
{
    return _size;
}

bool Rectangle::contains(int x, int y) const
{
    return (x >= _pos._x && y >= _pos._y && x <= _pos._x+_size._width && y <= _pos._y+_size._height);
}

bool Rectangle::contains(const Point& pos) const
{
    return contains(pos._x, pos._y);
}

bool Rectangle::containsX(int x) const
{
    return (x >= _pos._x && x <= _pos._x+_size._width);
}

bool Rectangle::containsY(int y) const
{
    return (y >= _pos._y && y <= _pos._y+_size._height);
}

void Rectangle::setX(int x)
{
    _pos._x = x;
}

void Rectangle::setY(int y)
{
    _pos._y = y;
}

void Rectangle::setPos(int x, int y)
{
    _pos._x = x;
    _pos._y = y;
}

void Rectangle::setPos(const Point& pos)
{
    _pos = pos;
}

void Rectangle::move(int x, int y)
{
    _pos._x += x;
    _pos._y +=  y;
}

void Rectangle::move(const Point& pos)
{
    _pos += pos;
}

void Rectangle::setWidth(int width)
{
    _size._width = width;
}

void Rectangle::setHeight(int height)
{
    _size._height = height;
}

void Rectangle::setSize(int width, int height)
{
    _size._width  = width;
    _size._height = height;
}

void Rectangle::setSize(const Size& size)
{
    _size = size;
}

void Rectangle::grow(int m)
{
    _size *= m;
}

void Rectangle::grow(float m)
{
    _size *= m;
}

void Rectangle::grow(int width, int height)
{
    _size._width  += width;
    _size._height += height;
}

void Rectangle::grow(const Size& size)
{
    _size += size;
}

void Rectangle::shrink(int m)
{
    _size /= m;
}

void Rectangle::shrink(float m)
{
    _size /= m;
}

void Rectangle::shrink(int width, int height)
{
    _size._width  -= width;
    _size._height -= height;
}

void Rectangle::shrink(const Size& size)
{
    _size -= size;
}

Rectangle& Rectangle::operator=(const Rectangle& rect)
{
    _pos  = rect._pos;
    _size = rect._size;
    return *this;
}

Rectangle& Rectangle::operator+=(const Point& pos)
{
    _pos  += pos;
    return *this;
}

Rectangle& Rectangle::operator-=(const Point& pos)
{
    _pos  -= pos;
    return *this;
}

Rectangle& Rectangle::operator+=(const Size& size)
{
    _size += size;
    return *this;
}

Rectangle& Rectangle::operator-=(const Size& size)
{
    _size -= size;
    return *this;
}

// -------------------------------------------------
// Image

Image::Image(const char* data, int width, int height, GLenum format, GLenum type)
    : _data(data),
      _size(width, height),
      _format(format),
      _type(type)
{
}

Image::Image(const char* data, const Size& size, GLenum format, GLenum type)
    : _data(data),
      _size(size),
      _format(format),
      _type(type)
{
}

Image::Image(const Image& image)
    : _data(image._data),
      _size(image._size),
      _format(image._format),
      _type(image._type)
{
}

bool Image::isValid() const
{
    return (_data && getWidth() > 0 && getHeight() > 0);
}

int Image::getWidth() const
{
    return _size.getWidth();
}

int Image::getHeight() const
{
    return _size.getHeight();
}

const Size& Image::getSize() const
{
    return _size;
}

const char* Image::getData() const
{
    return _data;
}

GLenum Image::getFormat() const
{
    return _format;
}

GLenum Image::getType() const
{
    return _type;
}

Image& Image::operator=(const Image& image)
{
    _data   = image._data;
    _size   = image._size;
    _format = image._format;
    _type   = image._type;
    return *this;
}

// -------------------------------------------------
// ImageButton

ImageButton::ImageButton(const Image& imageNormal, const Image& imageHover, const Image& imageDown, const Point& pos)
    : _imageNormal(imageNormal),
      _imageHover(imageHover),
      _imageDown(imageDown),
      _curImage(&_imageNormal),
      _pos(pos),
      _area(pos, imageNormal.getSize())
{
}

ImageButton::ImageButton(const ImageButton& imageButton)
    : _imageNormal(imageButton._imageNormal),
      _imageHover(imageButton._imageHover),
      _imageDown(imageButton._imageDown),
      _curImage(&_imageNormal),
      _pos(imageButton._pos),
      _area(imageButton._area)
{
}

int ImageButton::getWidth() const
{
    return _area.getWidth();
}

int ImageButton::getHeight() const
{
    return _area.getHeight();
}

const Size& ImageButton::getSize() const
{
    return _area.getSize();
}

ImageButton& ImageButton::operator=(const ImageButton& imageButton)
{
    _imageNormal = imageButton._imageNormal;
    _imageHover  = imageButton._imageHover;
    _imageDown   = imageButton._imageDown;
    _curImage    = &_imageNormal;
    _pos  = imageButton._pos;
    _area = imageButton._area;
    return *this;
}

// -------------------------------------------------
// ImageKnob

ImageKnob::ImageKnob(const Image& image, const Point& pos, Orientation orientation)
    : _image(image),
      _pos(pos),
      _orientation(orientation),
      _isVertical(image.getHeight() > image.getWidth()),
      _layerSize(_isVertical ? image.getWidth() : image.getHeight()),
      _layerCount(_isVertical ? image.getHeight()/_layerSize : image.getWidth()/_layerSize),
      _area(_pos, _layerSize, _layerSize)
{
    _min   = 0.0f;
    _max   = 1.0f;
    _value = _min;
}

ImageKnob::ImageKnob(const ImageKnob& imageKnob)
    : _image(imageKnob._image),
      _pos(imageKnob._pos),
      _orientation(imageKnob._orientation),
      _isVertical(imageKnob._isVertical),
      _layerSize(imageKnob._layerSize),
      _layerCount(imageKnob._layerCount),
      _area(imageKnob._area)
{
    _min   = imageKnob._min;
    _max   = imageKnob._max;
    _value = imageKnob._value;
}

void ImageKnob::setOrientation(Orientation orientation)
{
    _orientation = orientation;
}

void ImageKnob::setRange(float min, float max)
{
    _min = min;
    _max = max;

    if (_value < _min)
        _value = _min;
    else if (_value > _max)
        _value = _max;
}

void ImageKnob::setValue(float value)
{
    if (value < _min)
        value = _min;
    else if (value > _max)
        value = _max;

    _value = value;
}

ImageKnob& ImageKnob::operator=(const ImageKnob& imageKnob)
{
    _image = imageKnob._image;
    _pos   = imageKnob._pos;
    _orientation = imageKnob._orientation;
    _isVertical  = imageKnob._isVertical;
    _layerSize   = imageKnob._layerSize;
    _layerCount  = imageKnob._layerCount;
    _area  = imageKnob._area;
    _min   = imageKnob._min;
    _max   = imageKnob._max;
    _value = imageKnob._value;
    return *this;
}

// -------------------------------------------------
// ImageSlider

ImageSlider::ImageSlider(const Image& image, const Point& startPos, const Point& endPos)
    : _image(image),
      _startPos(startPos),
      _endPos(endPos),
      _area(startPos.getX(), startPos.getY(),
            endPos.getX() > startPos.getX() ? endPos.getX() + image.getWidth() - startPos.getX() : image.getWidth(),
            endPos.getY() > startPos.getY() ? endPos.getY() + image.getHeight() - startPos.getY() : image.getHeight())
{
    _min   = 0.0f;
    _max   = 1.0f;
    _value = _min;
}

ImageSlider::ImageSlider(const ImageSlider& imageSlider)
    : _image(imageSlider._image),
      _startPos(imageSlider._startPos),
      _endPos(imageSlider._endPos),
      _area(imageSlider._area)
{
    _min   = imageSlider._min;
    _max   = imageSlider._max;
    _value = imageSlider._value;
}

int ImageSlider::getWidth() const
{
    return _image.getWidth();
}

int ImageSlider::getHeight() const
{
    return _image.getHeight();
}

void ImageSlider::setRange(float min, float max)
{
    _min = min;
    _max = max;

    if (_value < _min)
        _value = _min;
    else if (_value > _max)
        _value = _max;
}

void ImageSlider::setValue(float value)
{
    if (value < _min)
        value = _min;
    else if (value > _max)
        value = _max;

    _value = value;
}

ImageSlider& ImageSlider::operator=(const ImageSlider& imageSlider)
{
    _image    = imageSlider._image;
    _startPos = imageSlider._startPos;
    _endPos   = imageSlider._endPos;
    _area     = imageSlider._area;
    _min      = imageSlider._min;
    _max      = imageSlider._max;
    _value    = imageSlider._value;
    return *this;
}

// -------------------------------------------------

class OpenGLDialog
{
public:
    OpenGLDialog(PuglView* parentView, const Size& parentSize, const Image& image_, const char* title)
        : image(image_)
    {
#if DISTRHO_OS_LINUX
        bool addToDesktop = false;
#else
        bool addToDesktop = true;
#endif

        view   = puglCreate(0, title, image.getWidth(), image.getHeight(), false, addToDesktop);
        closed = bool(!view);

        if (closed)
            return;

        puglSetHandle(view, this);
        puglSetDisplayFunc(view, onDisplayCallback);
        puglSetKeyboardFunc(view, onKeyboardCallback);
        puglSetMotionFunc(view, onMotionCallback);
        puglSetMouseFunc(view, onMouseCallback);
        puglSetScrollFunc(view, onScrollCallback);
        puglSetSpecialFunc(view, onSpecialCallback);
        puglSetReshapeFunc(view, onReshapeCallback);
        puglSetCloseFunc(view, onCloseCallback);

#if DISTRHO_OS_LINUX
        PuglInternals* impl = puglGetInternalsImpl(view);
        Display* display    = impl->display;
        Window thisWindow   = impl->win;
        Window parentWindow = puglGetInternalsImpl(parentView)->win;

        int x = (parentSize.getWidth()-image.getWidth())/2;
        int y = (parentSize.getHeight()-image.getHeight())/2;
        Window childRet;

        if (XTranslateCoordinates(display, parentWindow, thisWindow, x, y, &x, &y, &childRet))
            XMoveWindow(display, thisWindow, x, y);

        XSetTransientForHint(display, thisWindow, parentWindow);
        XMapRaised(display, thisWindow);
#else
        return;

        // unused
        (void)parentView;
        (void)parentSize;
#endif
    }

    ~OpenGLDialog()
    {
        if (view)
            puglDestroy(view);
    }

    bool idle()
    {
        if (view)
            puglProcessEvents(view);

        return !closed;
    }

    void close()
    {
        closed = true;
    }

    void raise()
    {
        if (view)
        {
#if DISTRHO_OS_LINUX
	    PuglInternals* impl = puglGetInternalsImpl(view);
            Display* display    = impl->display;
            Window   window     = impl->win;
            XRaiseWindow(display, window);
            XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
#endif
        }
    }

protected:
    void onDisplay()
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glRasterPos2i(0, image.getHeight());
        glDrawPixels(image.getWidth(), image.getHeight(), image.getFormat(), image.getType(), image.getData());
    }

    void onKeyboard(bool press, uint32_t key)
    {
        if (press && key == CHAR_ESCAPE)
            closed = true;
    }

    void onMotion(int, int)
    {
    }

    void onMouse(int, bool, int, int)
    {
    }

    void onReshape(int width, int height)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, 0, 1);
        glViewport(0, 0, width, height);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void onScroll(float, float)
    {
    }

    void onSpecial(bool, Key)
    {
    }

    void onClose()
    {
        closed = true;
    }

private:
    PuglView* view;
    bool closed;

    const Image image;

    // Callbacks
    static void onDisplayCallback(PuglView* view)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onDisplay();
    }

    static void onKeyboardCallback(PuglView* view, bool press, uint32_t key)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onKeyboard(press, key);
    }

    static void onMotionCallback(PuglView* view, int x, int y)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onMotion(x, y);
    }

    static void onMouseCallback(PuglView* view, int button, bool press, int x, int y)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onMouse(button, press, x, y);
    }

    static void onReshapeCallback(PuglView* view, int width, int height)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onReshape(width, height);
    }

    static void onScrollCallback(PuglView* view, float dx, float dy)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onScroll(dx, dy);
    }

    static void onSpecialCallback(PuglView* view, bool press, PuglKey key)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onSpecial(press, (Key)key);
    }

    static void onCloseCallback(PuglView* view)
    {
        OpenGLDialog* _this_ = (OpenGLDialog*)puglGetHandle(view);
        _this_->onClose();
    }
};

enum ObjectType {
    OBJECT_NULL   = 0,
    OBJECT_BUTTON = 1,
    OBJECT_KNOB   = 2,
    OBJECT_SLIDER = 3
};

#if DISTRHO_OS_LINUX
struct LinuxData {
    Display* display;
    Window   window;

    char   colorData[8];
    XColor colorBlack;
    Pixmap pixmapBlack;
    Cursor cursorBlack;

    LinuxData()
        : display(nullptr),
          window(0),
          colorData{0},
          colorBlack{0, 0, 0, 0, 0, 0}
    {
    }

    ~LinuxData()
    {
        XFreeCursor(display, cursorBlack);
    }
};
#endif

struct OpenGLExtUIPrivateData {
    int initialPosX;
    int initialPosY;
    void*      lastObj;
    Point      lastCursorPos;
    ObjectType lastObjType;

    Image background;
    std::vector<ImageButton*> buttons;
    std::vector<ImageKnob*>   knobs;
    std::vector<ImageSlider*> sliders;
    OpenGLDialog* dialog;

#if DISTRHO_OS_LINUX
    LinuxData linuxData;
#endif

    OpenGLExtUIPrivateData()
        : initialPosX(0),
          initialPosY(0),
          lastObj(nullptr),
          lastCursorPos(0, 0),
          lastObjType(OBJECT_NULL),
          background(nullptr, 0, 0),
          dialog(nullptr)
    {
    }

    void showCursor()
    {
#if DISTRHO_OS_LINUX
        if (lastCursorPos != Point(-1, -2))
            XWarpPointer(linuxData.display, None, DefaultRootWindow(linuxData.display), 0, 0, 0, 0, lastCursorPos.getX(), lastCursorPos.getY());

        XUndefineCursor(linuxData.display, linuxData.window);
#endif
    }

    void hideCursor()
    {
#if DISTRHO_OS_LINUX
        Window root, child;
        int rootX, rootY, winX, winY;
        unsigned int mask;

        if (XQueryPointer(linuxData.display, DefaultRootWindow(linuxData.display), &root, &child, &rootX, &rootY, &winX, &winY, &mask))
            lastCursorPos = Point(rootX, rootY);
        else
            lastCursorPos = Point(-1, -2);

        XDefineCursor(linuxData.display, linuxData.window, linuxData.cursorBlack);
#endif
    }
};

// -------------------------------------------------

OpenGLExtUI::OpenGLExtUI()
    : OpenGLUI()
{
    data = new OpenGLExtUIPrivateData;
}

OpenGLExtUI::~OpenGLExtUI()
{
    assert(! data->dialog);
    delete data;
}

// -------------------------------------------------
// UI Callbacks

void OpenGLExtUI::d_uiIdle()
{
    if (data->dialog && ! data->dialog->idle())
    {
        delete data->dialog;
        data->dialog = nullptr;
    }

    OpenGLUI::d_uiIdle();
}

// -------------------------------------------------
// Extended Calls

void OpenGLExtUI::setBackgroundImage(const Image& image)
{
    data->background = image;
    d_uiRepaint();
}

void OpenGLExtUI::addImageButton(ImageButton* button)
{
    data->buttons.push_back(button);
}

void OpenGLExtUI::addImageKnob(ImageKnob* knob)
{
    data->knobs.push_back(knob);
}

void OpenGLExtUI::addImageSlider(ImageSlider* slider)
{
    data->sliders.push_back(slider);
}

void OpenGLExtUI::showImageModalDialog(const Image& image, const char* title)
{
    data->dialog = new OpenGLDialog(OpenGLUI::data->widget, Size(d_width(), d_height()), image, title);
}

// -------------------------------------------------
// Extended Callbacks

void OpenGLExtUI::imageButtonClicked(ImageButton*)
{
}

void OpenGLExtUI::imageKnobDragStarted(ImageKnob*)
{
}

void OpenGLExtUI::imageKnobDragFinished(ImageKnob*)
{
}

void OpenGLExtUI::imageKnobValueChanged(ImageKnob*, float)
{
}

void OpenGLExtUI::imageSliderValueChanged(ImageSlider*, float)
{
}

void OpenGLExtUI::imageSliderDragStarted(ImageSlider*)
{
}

void OpenGLExtUI::imageSliderDragFinished(ImageSlider*)
{
}

void OpenGLExtUI::d_onInit()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, d_width(), d_height(), 0, 0, 1);
    glViewport(0, 0, d_width(), d_height());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

#if DISTRHO_OS_LINUX
    PuglInternals* dataImpl = puglGetInternalsImpl(OpenGLUI::data->widget);
    data->linuxData.display = dataImpl->display;
    data->linuxData.window  = dataImpl->win;
    data->linuxData.pixmapBlack = XCreateBitmapFromData(data->linuxData.display, data->linuxData.window, data->linuxData.colorData, 8, 8);
    data->linuxData.cursorBlack = XCreatePixmapCursor(data->linuxData.display, data->linuxData.pixmapBlack, data->linuxData.pixmapBlack,
                                                      &data->linuxData.colorBlack, &data->linuxData.colorBlack, 0, 0);
#endif
}

void OpenGLExtUI::d_onDisplay()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Background
    if (data->background.isValid())
    {
        glRasterPos2i(0, d_height());
        glDrawPixels(d_width(), d_height(), data->background._format, data->background._type, data->background._data);
    }

    // Buttons
    if (data->buttons.size() > 0)
    {
        for (auto it = data->buttons.begin(); it != data->buttons.end(); it++)
        {
            ImageButton* button(*it);

            glRasterPos2i(button->_pos.getX(), button->_pos.getY() + button->_area.getHeight());
            glDrawPixels(button->getWidth(), button->getHeight(), button->_curImage->getFormat(), button->_curImage->getType(), button->_curImage->getData());
        }
    }

    // Knobs
    if (data->knobs.size() > 0)
    {
        for (auto it = data->knobs.begin(); it != data->knobs.end(); it++)
        {
            ImageKnob* knob(*it);

            float vper = (knob->_value - knob->_min) / (knob->_max - knob->_min);

            int layerDataSize   = knob->_layerSize * knob->_layerSize * 4;
            int imageDataSize   = layerDataSize * knob->_layerCount;
            int imageDataOffset = imageDataSize - layerDataSize - layerDataSize * rint(vper*(knob->_layerCount-1));

            glRasterPos2i(knob->_pos.getX(), knob->_pos.getY()+knob->_area.getHeight());
            glDrawPixels(knob->_layerSize, knob->_layerSize, knob->_image.getFormat(), knob->_image.getType(), knob->_image.getData() + imageDataOffset);
        }
    }

    // Sliders
    if (data->sliders.size() > 0)
    {
        for (auto it = data->sliders.begin(); it != data->sliders.end(); it++)
        {
            ImageSlider* slider(*it);

            float vper = (slider->_value - slider->_min) / (slider->_max - slider->_min);
            int x = slider->_area.getX();
            int y = slider->_area.getY();

            if (slider->_endPos.getX() > slider->_startPos.getX())
                // horizontal
                x +=  rint(vper * (slider->_area.getWidth()-slider->getWidth()));
            else
                // vertical
                y += slider->_area.getHeight() - rint(vper * (slider->_area.getHeight()-slider->getHeight()));

#if 0 // DEBUG
            glColor3i(160, 90, 161);
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 0.0f); glVertex3f(slider->_area.getX(),                          slider->_area.getY(), 0);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(slider->_area.getX()+slider->_area.getWidth(), slider->_area.getY(), 0);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(slider->_area.getX()+slider->_area.getWidth(), slider->_area.getY()+slider->_area.getHeight(), 0);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(slider->_area.getX(),                          slider->_area.getY()+slider->_area.getHeight(), 0);
            glEnd();
            glColor3i(160, 90, 161);
#endif

            glRasterPos2i(x, y);
            glDrawPixels(slider->getWidth(), slider->getHeight(), slider->_image._format, slider->_image._type, slider->_image._data);
        }
    }
}

void OpenGLExtUI::d_onKeyboard(bool press, uint32_t key)
{
    if (data->dialog)
        return;

    (void)press;
    (void)key;
}

void OpenGLExtUI::d_onMotion(int x, int y)
{
    if (data->dialog)
        return;

    // Buttons
    if (data->buttons.size() > 0)
    {
        for (auto it = data->buttons.begin(); it != data->buttons.end(); it++)
        {
            ImageButton* button(*it);

            if (button == data->lastObj)
                continue;

            if (button->_area.contains(x, y))
            {
                if (button->_curImage != &button->_imageHover)
                    d_uiRepaint();

                button->_curImage = &button->_imageHover;
            }
            else
            {
                if (button->_curImage != &button->_imageNormal)
                    d_uiRepaint();

                button->_curImage = &button->_imageNormal;
            }
        }
    }

    if (data->lastObjType == OBJECT_NULL || ! data->lastObj)
        return;

    // Knobs
    if (data->lastObjType == OBJECT_KNOB && data->knobs.size() > 0)
    {
        for (auto it = data->knobs.begin(); it != data->knobs.end(); it++)
        {
            ImageKnob* knob(*it);

            if (knob != data->lastObj)
                continue;

            if (knob->_orientation == ImageKnob::Horizontal)
            {
                int movX = x - data->initialPosX;

                if (movX != 0)
                {
                    int d = (d_uiGetModifiers() & MODIFIER_SHIFT) ? 2000 : 200;
                    float value = knob->_value + (knob->_max - knob->_min) / d * movX;

                    if (value < knob->_min)
                        value = knob->_min;
                    else if (value > knob->_max)
                        value = knob->_max;

                    if (knob->_value != value)
                    {
                        knob->_value = value;
                        imageKnobValueChanged(knob, value);

                        d_uiRepaint();
                    }
                }
            }
            else if (knob->_orientation == ImageKnob::Vertical)
            {
                int movY = data->initialPosY - y;

                if (movY != 0)
                {
                    int d = (d_uiGetModifiers() & MODIFIER_SHIFT) ? 2000 : 200;
                    float value = knob->_value + (knob->_max - knob->_min) / d * movY;

                    if (value < knob->_min)
                        value = knob->_min;
                    else if (value > knob->_max)
                        value = knob->_max;

                    if (knob->_value != value)
                    {
                        knob->_value = value;
                        imageKnobValueChanged(knob, value);

                        d_uiRepaint();
                    }
                }
            }

            data->initialPosX = x;
            data->initialPosY = y;

            return;
        }
    }

    // Sliders
    if (data->lastObjType == OBJECT_SLIDER && data->sliders.size() > 0)
    {
        for (auto it = data->sliders.begin(); it != data->sliders.end(); it++)
        {
            ImageSlider* slider(*it);

            if (slider != data->lastObj)
                continue;

            bool horizontal = slider->_endPos.getX() > slider->_startPos.getX();

            if ((horizontal && slider->_area.containsX(x)) || (slider->_area.containsY(y) && ! horizontal))
            {
                float vper;

                if (horizontal)
                    // horizontal
                    vper  = float(x - slider->_area.getX()) / slider->_area.getWidth();
                else
                    // vertical
                    vper  = float(y - slider->_area.getY()) / slider->_area.getHeight();

                float value = slider->_max - vper * (slider->_max - slider->_min);

                if (value < slider->_min)
                    value = slider->_min;
                else if (value > slider->_max)
                    value = slider->_max;

                if (slider->_value != value)
                {
                    slider->_value = value;
                    imageSliderValueChanged(slider, value);

                    d_uiRepaint();
                }
            }
            else if (y < slider->_area.getY())
            {
                if (slider->_value != slider->_max)
                {
                    slider->_value = slider->_max;
                    imageSliderValueChanged(slider, slider->_max);

                    d_uiRepaint();
                }
            }
            else
            {
                if (slider->_value != slider->_min)
                {
                    slider->_value = slider->_min;
                    imageSliderValueChanged(slider, slider->_min);

                    d_uiRepaint();
                }
            }

            return;
        }
    }
}

void OpenGLExtUI::d_onMouse(int button, bool press, int x, int y)
{
    if (data->dialog)
    {
        data->dialog->raise();
        return;
    }

    if ((!press) && data->lastObjType == OBJECT_BUTTON && data->lastObj && data->buttons.size() > 0)
    {
        for (auto it = data->buttons.begin(); it != data->buttons.end(); it++)
        {
            ImageButton* button(*it);

            if (button == data->lastObj)
            {
                if (button->_area.contains(x, y))
                    imageButtonClicked(button);

                button->_curImage = &button->_imageNormal;
                d_uiRepaint();

                break;
            }
        }
    }

    if (button != 1)
        return;

    if (data->lastObjType != OBJECT_NULL && data->lastObj)
    {
        if (data->lastObjType == OBJECT_KNOB)
        {
            data->showCursor();
            imageKnobDragFinished((ImageKnob*)data->lastObj);
        }
        else if (data->lastObjType == OBJECT_SLIDER)
            imageSliderDragFinished((ImageSlider*)data->lastObj);
    }

    data->initialPosX = 0;
    data->initialPosY = 0;
    data->lastObj     = nullptr;
    data->lastObjType = OBJECT_NULL;

    if (! press)
        return;

    // Buttons
    if (data->buttons.size() > 0)
    {
        for (auto it = data->buttons.begin(); it != data->buttons.end(); it++)
        {
            ImageButton* button(*it);

            if (button->_area.contains(x, y))
            {
                data->initialPosX = x;
                data->initialPosY = y;
                data->lastObj     = button;
                data->lastObjType = OBJECT_BUTTON;

                button->_curImage = &button->_imageDown;
                d_uiRepaint();

                return;
            }
        }
    }

    // Knobs
    if (data->knobs.size() > 0)
    {
        for (auto it = data->knobs.begin(); it != data->knobs.end(); it++)
        {
            ImageKnob* knob(*it);

            if (knob->_area.contains(x, y))
            {
                data->initialPosX = x;
                data->initialPosY = y;
                data->lastObj     = knob;
                data->lastObjType = OBJECT_KNOB;
                data->hideCursor();
                imageKnobDragStarted(knob);
                return;
            }
        }
    }

    // Sliders
    if (data->sliders.size() > 0)
    {
        for (auto it = data->sliders.begin(); it != data->sliders.end(); it++)
        {
            ImageSlider* slider(*it);

            if (slider->_area.contains(x, y))
            {
                data->initialPosX = x;
                data->initialPosY = y;
                data->lastObj     = slider;
                data->lastObjType = OBJECT_SLIDER;
                imageSliderDragStarted(slider);

                float vper;

                if (slider->_endPos.getX() > slider->_startPos.getX())
                    // horizontal
                    vper  = float(x - slider->_area.getX()) / slider->_area.getWidth();
                else
                    // vertical
                    vper  = float(y - slider->_area.getY()) / slider->_area.getHeight();

                float value = slider->_max - vper * (slider->_max - slider->_min);

                if (value < slider->_min)
                    value = slider->_min;
                else if (value > slider->_max)
                    value = slider->_max;

                if (slider->_value != value)
                {
                    slider->_value = value;
                    imageSliderValueChanged(slider, value);

                    d_uiRepaint();
                }

                return;
            }
        }
    }
}

void OpenGLExtUI::d_onReshape(int width, int height)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0, 1);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void OpenGLExtUI::d_onScroll(float dx, float dy)
{
    if (data->dialog)
        return;

    // unused
    (void)dx;
    (void)dy;
}

void OpenGLExtUI::d_onSpecial(bool press, Key key)
{
    if (data->dialog)
        return;

    // unused
    (void)press;
    (void)key;
}

void OpenGLExtUI::d_onClose()
{
    if (data->dialog)
    {
        data->dialog->close();
        delete data->dialog;
        data->dialog = nullptr;
    }

    if (data->lastObjType != OBJECT_NULL && data->lastObj)
    {
        if (data->lastObjType == OBJECT_KNOB)
        {
            data->showCursor();
            imageKnobDragFinished((ImageKnob*)data->lastObj);
        }
        else if (data->lastObjType == OBJECT_SLIDER)
            imageSliderDragFinished((ImageSlider*)data->lastObj);
    }

    data->initialPosX = 0;
    data->initialPosY = 0;
    data->lastObj     = nullptr;
    data->lastObjType = OBJECT_NULL;

    data->buttons.clear();
    data->knobs.clear();
    data->sliders.clear();
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_OPENGL
