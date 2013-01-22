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

#ifndef __DISTRHO_UI_OPENGL_EXT_H__
#define __DISTRHO_UI_OPENGL_EXT_H__

#include "src/DistrhoDefines.h"

#ifdef DISTRHO_UI_OPENGL

#include "DistrhoUIOpenGL.h"
#include <GL/gl.h>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class Point
{
public:
    Point(int x, int y);
    Point(const Point& pos);

    int getX() const;
    int getY() const;

    void setX(int x);
    void setY(int y);

    Point& operator=(const Point& pos);
    Point& operator+=(const Point& pos);
    Point& operator-=(const Point& pos);
    bool operator==(const Point& pos) const;
    bool operator!=(const Point& pos) const;

private:
    int _x, _y;
    friend class Rectangle;
};

class Size
{
public:
    Size(int width, int height);
    Size(const Size& size);

    int getWidth() const;
    int getHeight() const;

    void setWidth(int width);
    void setHeight(int height);

    Size& operator=(const Size& size);
    Size& operator+=(const Size& size);
    Size& operator-=(const Size& size);
    Size& operator*=(int m);
    Size& operator/=(int d);
    Size& operator*=(float m);
    Size& operator/=(float d);

private:
    int _width, _height;
    friend class Rectangle;
};

class Rectangle
{
public:
    Rectangle(int x, int y, int width, int height);
    Rectangle(int x, int y, const Size& size);
    Rectangle(const Point& pos, int width, int height);
    Rectangle(const Point& pos, const Size& size);
    Rectangle(const Rectangle& rect);

    int getX() const;
    int getY() const;
    int getWidth() const;
    int getHeight() const;

    const Point& getPos() const;
    const Size&  getSize() const;

    bool contains(int x, int y) const;
    bool contains(const Point& pos) const;
    bool containsX(int x) const;
    bool containsY(int y) const;

    void setX(int x);
    void setY(int y);
    void setPos(int x, int y);
    void setPos(const Point& pos);

    void move(int x, int y);
    void move(const Point& pos);

    void setWidth(int width);
    void setHeight(int height);
    void setSize(int width, int height);
    void setSize(const Size& size);

    void grow(int m);
    void grow(float m);
    void grow(int width, int height);
    void grow(const Size& size);

    void shrink(int m);
    void shrink(float m);
    void shrink(int width, int height);
    void shrink(const Size& size);

    Rectangle& operator=(const Rectangle& rect);
    Rectangle& operator+=(const Point& pos);
    Rectangle& operator-=(const Point& pos);
    Rectangle& operator+=(const Size& size);
    Rectangle& operator-=(const Size& size);

private:
    Point _pos;
    Size  _size;
};

// -------------------------------------------------

class Image
{
public:
    Image(const char* data, int width, int height, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE);
    Image(const char* data, const Size& size, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE);
    Image(const Image& image);

    bool isValid() const;

    int getWidth() const;
    int getHeight() const;
    const Size& getSize() const;

    const char* getData() const;
    GLenum getFormat() const;
    GLenum getType() const;

    Image& operator=(const Image& image);

private:
    const char* _data;
    Size   _size;
    GLenum _format;
    GLenum _type;
    friend class OpenGLExtUI;
};

class ImageButton
{
public:
    ImageButton(const Image& imageNormal, const Image& imageHover, const Image& imageDown, const Point& pos);
    ImageButton(const ImageButton& imageButton);

    int getWidth() const;
    int getHeight() const;
    const Size& getSize() const;

    ImageButton& operator=(const ImageButton& imageButton);

private:
    Image _imageNormal;
    Image _imageHover;
    Image _imageDown;
    Image* _curImage;
    Point _pos;
    Rectangle _area;
    friend class OpenGLExtUI;
};

class ImageKnob
{
public:
    enum Orientation {
        Horizontal,
        Vertical
    };

    ImageKnob(const Image& image, const Point& pos, Orientation orientation = Vertical);
    ImageKnob(const ImageKnob& imageKnob);

    void setOrientation(Orientation orientation);
    void setRange(float min, float max);
    void setValue(float value);

    ImageKnob& operator=(const ImageKnob& slider);

private:
    Image _image;
    Point _pos;
    Orientation _orientation;
    bool _isVertical;
    int _layerSize;
    int _layerCount;
    Rectangle _area;
    float _min, _max, _value;
    friend class OpenGLExtUI;
};

class ImageSlider
{
public:
    ImageSlider(const Image& image, const Point& startPos, const Point& endPos);
    ImageSlider(const ImageSlider& imageSlider);

    int getWidth() const;
    int getHeight() const;

    void setRange(float min, float max);
    void setValue(float value);

    ImageSlider& operator=(const ImageSlider& slider);

private:
    Image _image;
    Point _startPos;
    Point _endPos;
    Rectangle _area;
    float _min, _max, _value;
    friend class OpenGLExtUI;
};

// -------------------------------------------------

struct OpenGLExtUIPrivateData;

class OpenGLExtUI : public OpenGLUI
{
public:
    OpenGLExtUI();
    virtual ~OpenGLExtUI();

    // ---------------------------------------------

protected:
    // Information
    virtual unsigned int d_width() = 0;
    virtual unsigned int d_height() = 0;

    // DSP Callbacks
    virtual void d_parameterChanged(uint32_t index, float value) = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_programChanged(uint32_t index) = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_stateChanged(const char* key, const char* value) = 0;
#endif

    // UI Callbacks
    virtual void d_uiIdle();

    // Extended Calls
    void setBackgroundImage(const Image& image);

    void addImageButton(ImageButton* button);
    void addImageKnob(ImageKnob* knob);
    void addImageSlider(ImageSlider* slider);

    void showImageModalDialog(const Image& image, const char* title);

    // Extended Callbacks
    virtual void imageButtonClicked(ImageButton* button);
    virtual void imageKnobDragStarted(ImageKnob* knob);
    virtual void imageKnobDragFinished(ImageKnob* knob);
    virtual void imageKnobValueChanged(ImageKnob* knob, float value);
    virtual void imageSliderDragStarted(ImageSlider* slider);
    virtual void imageSliderDragFinished(ImageSlider* slider);
    virtual void imageSliderValueChanged(ImageSlider* slider, float value);

private:
    OpenGLExtUIPrivateData* data;

    // Implemented internally
    void d_onInit();
    void d_onDisplay();
    void d_onKeyboard(bool press, uint32_t key);
    void d_onMotion(int x, int y);
    void d_onMouse(int button, bool press, int x, int y);
    void d_onReshape(int width, int height);
    void d_onScroll(float dx, float dy);
    void d_onSpecial(bool press, Key key);
    void d_onClose();
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_OPENGL

#endif // __DISTRHO_UI_OPENGL_EXT_H__
