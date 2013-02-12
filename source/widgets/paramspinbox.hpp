/*
 * Parameter Progress-Bar, a custom Qt4 widget
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __PARAMPROGRESSBAR_HPP__
#define __PARAMPROGRESSBAR_HPP__

#include <QtGui/QProgressBar>

class TextCallback
{
public:
    virtual ~TextCallback() {}
    virtual const char* textCallBack() = 0;
};

class ParamProgressBar : public QProgressBar
{
    Q_OBJECT

public:
    ParamProgressBar(QWidget* parent);

    void set_minimum(float value);
    void set_maximum(float value);
    void set_value(float value);
    void set_label(QString label);
    void set_text_call(TextCallback* textCall);

signals:
    void valueChangedFromBar(float value);

protected:
    void handleMouseEventPos(const QPoint& pos);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent* event);

private:
    bool  m_leftClickDown;
    float m_minimum;
    float m_maximum;
    float m_rvalue;
    QString m_label;
    QString m_preLabel;

    TextCallback* m_textCall;
};

#endif // __PARAMPROGRESSBAR_HPP__
