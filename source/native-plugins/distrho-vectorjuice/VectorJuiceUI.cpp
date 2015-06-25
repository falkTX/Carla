/*
 * Vector Juice Plugin
 * Copyright (C) 2014 Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz
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

#include "VectorJuicePlugin.hpp"
#include "VectorJuiceUI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

VectorJuiceUI::VectorJuiceUI()
    : UI(),
      fAboutWindow(this)
{
    setSize(VectorJuiceArtwork::backgroundWidth, VectorJuiceArtwork::backgroundHeight);

    // xy params
    paramX = paramY = 0.5f;

    // orbit params
    orbitX = orbitY = subOrbitX = subOrbitY = 0.5f;

    // set the XY canvas area
    fDragging = false;
    fDragValid = false;
    fLastX = fLastY = 0;
    fCanvasArea.setPos(22+12, 49+12);
    fCanvasArea.setSize(368-24, 368-24);

    // background
    fImgBackground = Image(VectorJuiceArtwork::backgroundData, VectorJuiceArtwork::backgroundWidth, VectorJuiceArtwork::backgroundHeight, GL_BGR);

    //roundlet
    fImgRoundlet = Image(VectorJuiceArtwork::roundletData, VectorJuiceArtwork::roundletWidth, VectorJuiceArtwork::roundletHeight);

    //orbit
    fImgOrbit = Image(VectorJuiceArtwork::orbitData, VectorJuiceArtwork::orbitWidth, VectorJuiceArtwork::orbitHeight);

    //subOrbit
    fImgSubOrbit = Image(VectorJuiceArtwork::subOrbitData, VectorJuiceArtwork::subOrbitWidth, VectorJuiceArtwork::subOrbitHeight);

    // about
    Image aboutImage(VectorJuiceArtwork::aboutData, VectorJuiceArtwork::aboutWidth, VectorJuiceArtwork::aboutHeight, GL_BGR);
    fAboutWindow.setImage(aboutImage);

    // about button
    Image aboutImageNormal(VectorJuiceArtwork::aboutButtonNormalData, VectorJuiceArtwork::aboutButtonNormalWidth, VectorJuiceArtwork::aboutButtonNormalHeight);
    Image aboutImageHover(VectorJuiceArtwork::aboutButtonHoverData, VectorJuiceArtwork::aboutButtonHoverWidth, VectorJuiceArtwork::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setAbsolutePos(599, 17);
    fButtonAbout->setCallback(this);

    // knobs
    Image knobImage(VectorJuiceArtwork::knobData, VectorJuiceArtwork::knobWidth, VectorJuiceArtwork::knobHeight);

    // knob KnobOrbitSizeX
    fKnobOrbitSizeX = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobOrbitSizeX->setId(VectorJuicePlugin::paramOrbitSizeX);
    fKnobOrbitSizeX->setAbsolutePos(423, 73);
    fKnobOrbitSizeX->setRotationAngle(270);
    fKnobOrbitSizeX->setRange(0.0f, 1.0f);
    fKnobOrbitSizeX->setDefault(0.5f);
    fKnobOrbitSizeX->setCallback(this);

    // knob KnobOrbitSizeY
    fKnobOrbitSizeY = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobOrbitSizeY->setId(VectorJuicePlugin::paramOrbitSizeY);
    fKnobOrbitSizeY->setAbsolutePos(516, 73);
    fKnobOrbitSizeY->setRotationAngle(270);
    fKnobOrbitSizeY->setRange(0.0f, 1.0f);
    fKnobOrbitSizeY->setDefault(0.5f);
    fKnobOrbitSizeY->setCallback(this);

    // knob KnobOrbitSpeedX
    fKnobOrbitSpeedX = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobOrbitSpeedX->setId(VectorJuicePlugin::paramOrbitSpeedX);
    fKnobOrbitSpeedX->setAbsolutePos(423, 185);
    fKnobOrbitSpeedX->setRotationAngle(270);
    fKnobOrbitSpeedX->setStep(1.0f);
    fKnobOrbitSpeedX->setRange(1.0f, 128.0f);
    fKnobOrbitSpeedX->setDefault(4.0f);
    fKnobOrbitSpeedX->setCallback(this);

    // knob KnobOrbitSpeedY
    fKnobOrbitSpeedY = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobOrbitSpeedY->setId(VectorJuicePlugin::paramOrbitSpeedY);
    fKnobOrbitSpeedY->setAbsolutePos(516, 185);
    fKnobOrbitSpeedY->setRotationAngle(270);
    fKnobOrbitSpeedY->setStep(1.0f);
    fKnobOrbitSpeedY->setRange(1.0f, 128.0f);
    fKnobOrbitSpeedY->setDefault(4.0f);
    fKnobOrbitSpeedY->setCallback(this);

    // knob KnobSubOrbitSize
    fKnobSubOrbitSize = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobSubOrbitSize->setId(VectorJuicePlugin::paramSubOrbitSize);
    fKnobSubOrbitSize->setAbsolutePos(620, 73);
    fKnobSubOrbitSize->setRange(0.0f, 1.0f);
    fKnobSubOrbitSize->setRotationAngle(270);
    fKnobSubOrbitSize->setDefault(0.5f);
    fKnobSubOrbitSize->setCallback(this);

    // knob KnobSubOrbitSpeed
    fKnobSubOrbitSpeed = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobSubOrbitSpeed->setId(VectorJuicePlugin::paramSubOrbitSpeed);
    fKnobSubOrbitSpeed->setAbsolutePos(620, 185);
    fKnobSubOrbitSpeed->setRotationAngle(270);
    fKnobSubOrbitSpeed->setStep(1.0f);
    fKnobSubOrbitSpeed->setRange(1.0f, 128.0f);
    fKnobSubOrbitSpeed->setDefault(32.0f);
    fKnobSubOrbitSpeed->setCallback(this);

    // knob KnobSubOrbitSmooth
    fKnobSubOrbitSmooth = new ImageKnob(this, knobImage, ImageKnob::Vertical);
    fKnobSubOrbitSmooth->setId(VectorJuicePlugin::paramSubOrbitSmooth);
    fKnobSubOrbitSmooth->setAbsolutePos(620, 297);
    fKnobSubOrbitSmooth->setRotationAngle(270);
    fKnobSubOrbitSmooth->setRange(0.0f, 1.0f);
    fKnobSubOrbitSmooth->setDefault(0.5f);
    fKnobSubOrbitSmooth->setCallback(this);

    // sliders
    Image sliderImage(VectorJuiceArtwork::sliderData, VectorJuiceArtwork::sliderWidth, VectorJuiceArtwork::sliderHeight);
    Point<int> sliderPosStart(410, 284);
    Point<int> sliderPosEnd(410+48, 284);

    // slider OrbitWaveX
    fSliderOrbitWaveX = new ImageSlider(this, sliderImage);
    fSliderOrbitWaveX->setId(VectorJuicePlugin::paramOrbitWaveX);
    fSliderOrbitWaveX->setStartPos(sliderPosStart);
    fSliderOrbitWaveX->setEndPos(sliderPosEnd);
    fSliderOrbitWaveX->setRange(1.0f, 4.0f);
    fSliderOrbitWaveX->setStep(1.0f);
    fSliderOrbitWaveX->setCallback(this);

    // slider OrbitWaveY
    sliderPosStart.setX(503);
    sliderPosEnd.setX(503+48);
    fSliderOrbitWaveY = new ImageSlider(this, sliderImage);
    fSliderOrbitWaveY->setId(VectorJuicePlugin::paramOrbitWaveY);
    fSliderOrbitWaveY->setStartPos(sliderPosStart);
    fSliderOrbitWaveY->setEndPos(sliderPosEnd);
    fSliderOrbitWaveY->setRange(1.0f, 4.0f);
    fSliderOrbitWaveY->setStep(1.0f);
    fSliderOrbitWaveY->setCallback(this);

    // slider OrbitPhaseX
    sliderPosStart.setX(410);
    sliderPosStart.setY(345);
    sliderPosEnd.setX(410+48);
    sliderPosEnd.setY(345);
    fSliderOrbitPhaseX = new ImageSlider(this, sliderImage);
    fSliderOrbitPhaseX->setId(VectorJuicePlugin::paramOrbitPhaseX);
    fSliderOrbitPhaseX->setStartPos(sliderPosStart);
    fSliderOrbitPhaseX->setEndPos(sliderPosEnd);
    fSliderOrbitPhaseX->setRange(1.0f, 4.0f);
    fSliderOrbitPhaseX->setStep(1.0f);
    fSliderOrbitPhaseX->setCallback(this);

    // slider OrbitPhaseY
    sliderPosStart.setX(503);
    sliderPosEnd.setX(503+48);
    fSliderOrbitPhaseY = new ImageSlider(this, sliderImage);
    fSliderOrbitPhaseY->setId(VectorJuicePlugin::paramOrbitPhaseY);
    fSliderOrbitPhaseY->setStartPos(sliderPosStart);
    fSliderOrbitPhaseY->setEndPos(sliderPosEnd);
    fSliderOrbitPhaseY->setRange(1.0f, 4.0f);
    fSliderOrbitPhaseY->setStep(1.0f);
    fSliderOrbitPhaseY->setCallback(this);

    // set default values
    programLoaded(0);
}

// -----------------------------------------------------------------------
// DSP Callbacks

void VectorJuiceUI::parameterChanged(uint32_t index, float value)
{
    switch (index)
    {
    case VectorJuicePlugin::paramX:
        if (paramX != value)
        {
            paramX = value;
            fDragValid = false;
            repaint();
        }
        break;
    case VectorJuicePlugin::paramY:
        if (paramY != value)
        {
            paramY = value;
            fDragValid = false;
            repaint();
        }
        break;
    case VectorJuicePlugin::paramOrbitSizeX:
        fKnobOrbitSizeX->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitSizeY:
        fKnobOrbitSizeY->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitSpeedX:
        fKnobOrbitSpeedX->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitSpeedY:
        fKnobOrbitSpeedY->setValue(value);
        break;
    case VectorJuicePlugin::paramSubOrbitSize:
        fKnobSubOrbitSize->setValue(value);
        break;
    case VectorJuicePlugin::paramSubOrbitSpeed:
        fKnobSubOrbitSpeed->setValue(value);
        break;
    case VectorJuicePlugin::paramSubOrbitSmooth:
        fKnobSubOrbitSmooth->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitWaveX:
        fSliderOrbitWaveX->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitWaveY:
        fSliderOrbitWaveY->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitPhaseX:
        fSliderOrbitPhaseX->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitPhaseY:
        fSliderOrbitPhaseY->setValue(value);
        break;
    case VectorJuicePlugin::paramOrbitOutX:
        if (orbitX != value)
        {
            orbitX = value;
            repaint();
        }
        break;
    case VectorJuicePlugin::paramOrbitOutY:
        if (orbitY != value)
        {
            orbitY = value;
            repaint();
        }
        break;
    case VectorJuicePlugin::paramSubOrbitOutX:
        if (subOrbitX != value)
        {
            subOrbitX = value;
            repaint();
        }
        break;
    case VectorJuicePlugin::paramSubOrbitOutY:
        if (subOrbitY != value)
        {
            subOrbitY = value;
            repaint();
        }
        break;
    }
}

void VectorJuiceUI::programLoaded(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    paramX = paramY = 0.5f;
    fKnobOrbitSizeX->setValue(0.5f);
    fKnobOrbitSizeY->setValue(0.5f);
    fKnobOrbitSpeedX->setValue(4.0f);
    fKnobOrbitSpeedY->setValue(4.0f);
    fKnobSubOrbitSize->setValue(0.5f);
    fKnobSubOrbitSpeed->setValue(32.0f);
    fKnobSubOrbitSmooth->setValue(0.5f);
    fSliderOrbitWaveX->setValue(3.0f);
    fSliderOrbitWaveY->setValue(3.0f);
    fSliderOrbitPhaseX->setValue(1.0f);
    fSliderOrbitPhaseY->setValue(1.0f);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void VectorJuiceUI::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void VectorJuiceUI::imageKnobDragStarted(ImageKnob* knob)
{
    editParameter(knob->getId(), true);
}

void VectorJuiceUI::imageKnobDragFinished(ImageKnob* knob)
{
    editParameter(knob->getId(), false);
}

void VectorJuiceUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    setParameterValue(knob->getId(), value);
}

void VectorJuiceUI::imageSliderDragStarted(ImageSlider* slider)
{
    editParameter(slider->getId(), true);
}

void VectorJuiceUI::imageSliderDragFinished(ImageSlider* slider)
{
    editParameter(slider->getId(), false);
}

void VectorJuiceUI::imageSliderValueChanged(ImageSlider* slider, float value)
{
    setParameterValue(slider->getId(), value);
}

void VectorJuiceUI::onDisplay()
{
    fImgBackground.draw();

    // get x, y mapped to XY area
    int x = fCanvasArea.getX() + paramX*fCanvasArea.getWidth() - fImgRoundlet.getWidth()/2;
    int y = fCanvasArea.getY() + paramY*fCanvasArea.getHeight() - fImgRoundlet.getHeight()/2;
    int nOrbitX = fCanvasArea.getX()+((orbitX)*fCanvasArea.getWidth())-15;
    int nOrbitY = fCanvasArea.getY()+((orbitY)*fCanvasArea.getWidth())-15;
    int nSubOrbitX = fCanvasArea.getX()+(subOrbitX*fCanvasArea.getWidth())-15;
    int nSubOrbitY = fCanvasArea.getY()+(subOrbitY*fCanvasArea.getWidth())-14;

    //draw lines, just for fun
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 1.0f, 0.0f, 0.05f);
    glLineWidth(4);
    glBegin(GL_LINES);
        glVertex2i(x+ fImgRoundlet.getWidth()/2, y+ fImgRoundlet.getHeight()/2);
        glVertex2i(nOrbitX+15, nOrbitY+15);
    glEnd();
    glBegin(GL_LINES);
        glVertex2i(nOrbitX+15, nOrbitY+15);
        glVertex2i(nSubOrbitX+15, nSubOrbitY+14);
    glEnd();

    // reset color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // draw roundlet and orbits
    fImgRoundlet.drawAt(x, y);
    fImgOrbit.drawAt(nOrbitX, nOrbitY);
    fImgSubOrbit.drawAt(nSubOrbitX, nSubOrbitY);
}

bool VectorJuiceUI::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! fCanvasArea.contains(ev.pos))
            return false;

        fDragging = true;
        fDragValid = true;
        fLastX = ev.pos.getX();
        fLastY = ev.pos.getY();
        return true;
    }
    else if (fDragging)
    {
        fDragging = false;
        return true;
    }

    return false;
}

bool VectorJuiceUI::onMotion(const MotionEvent& ev)
{
    if (! fDragging)
        return false;

    const int x = ev.pos.getX();
    const int y = ev.pos.getY();

    if (! fDragValid)
    {
        fDragValid = true;
        fLastX = x;
        fLastY = y;
    }

    const int movedX = fLastX - x;
    const int movedY = fLastY - y;
    fLastX = x;
    fLastY = y;

    float newX = paramX;
    float newY = paramY;

    newX -= float(movedX)/fCanvasArea.getWidth();
    newY -= float(movedY)/fCanvasArea.getHeight();

    if (newX < 0.0f)
        newX = 0.0f;
    else if (newX > 1.0f)
        newX = 1.0f;

    if (newY < 0.0f)
        newY = 0.0f;
    else if (newY > 1.0f)
        newY = 1.0f;

    if (newX != paramX)
    {
        paramX = newX;
        setParameterValue(VectorJuicePlugin::paramX, paramX);
        repaint();
    }

    if (newY != paramY)
    {
        paramY = newY;
        setParameterValue(VectorJuicePlugin::paramY, paramY);
        repaint();
    }

    return true;
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new VectorJuiceUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
