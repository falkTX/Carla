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

#include "paramspinbox.h"

#include <QtGui/QMouseEvent>

ParamProgressBar::ParamProgressBar(QWidget* parent)
    : QProgressBar(parent)
{
    m_leftClickDown = false;

    m_minimum = 0.0f;
    m_maximum = 1.0f;
    m_rvalue  = 0.0f;

    m_textCall = nullptr;

    setMinimum(0);
    setMaximum(1000);
    setValue(0);
    setFormat("(none)");
}

void ParamProgressBar::set_minimum(float value)
{
    m_minimum = value;
}

void ParamProgressBar::set_maximum(float value)
{
    m_maximum = value;
}

void ParamProgressBar::set_value(float value)
{
    m_rvalue = value;
    float vper = (value - m_minimum) / (m_maximum - m_minimum);
    setValue(vper * 1000);
}

void ParamProgressBar::set_label(QString label)
{
    m_label = label;

    if (m_label == "(coef)")
    {
        m_label = "";
        m_preLabel = "*";
    }
}

void ParamProgressBar::set_text_call(TextCallback* textCall)
{
    m_textCall = textCall;
}

void ParamProgressBar::handleMouseEventPos(const QPoint& pos)
{
    float xper  = float(pos.x()) / width();
    float value = xper * (m_maximum - m_minimum) + m_minimum;

    if (value < m_minimum)
        value = m_minimum;
    else if (value > m_maximum)
        value = m_maximum;

    emit valueChangedFromBar(value);
}

void ParamProgressBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        handleMouseEventPos(event->pos());
        m_leftClickDown = true;
    }
    else
        m_leftClickDown = false;
    QProgressBar::mousePressEvent(event);
}

void ParamProgressBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_leftClickDown)
        handleMouseEventPos(event->pos());
    QProgressBar::mouseMoveEvent(event);
}

void ParamProgressBar::mouseReleaseEvent(QMouseEvent* event)
{
    m_leftClickDown = false;
    QProgressBar::mouseReleaseEvent(event);
}

void ParamProgressBar::paintEvent(QPaintEvent* event)
{
    if (m_textCall)
        setFormat(QString("%1 %2 %3").arg(m_preLabel).arg(m_textCall->textCallBack()).arg(m_label));
    else
        setFormat(QString("%1 %2 %3").arg(m_preLabel).arg(m_rvalue).arg(m_label));

    QProgressBar::paintEvent(event);
}
