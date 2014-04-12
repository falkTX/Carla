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

#include "VectorJuiceUI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

VectorJuiceUI::VectorJuiceUI()
    : UI(),
      fAboutWindow(this)
{
    // xy params
    paramX = paramY = 0.5f;

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
    Image imageAbout(VectorJuiceArtwork::aboutData, VectorJuiceArtwork::aboutWidth, VectorJuiceArtwork::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // about button
    Image aboutImageNormal(VectorJuiceArtwork::aboutButtonNormalData, VectorJuiceArtwork::aboutButtonNormalWidth, VectorJuiceArtwork::aboutButtonNormalHeight);
    Image aboutImageHover(VectorJuiceArtwork::aboutButtonHoverData, VectorJuiceArtwork::aboutButtonHoverWidth, VectorJuiceArtwork::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(599, 17);
    fButtonAbout->setCallback(this);

    // knobs
    Image knobImage(VectorJuiceArtwork::knobData, VectorJuiceArtwork::knobWidth, VectorJuiceArtwork::knobHeight);

    // knob KnobOrbitSpeedX
    fKnobOrbitSpeedX = new ImageKnob(this, knobImage);
    fKnobOrbitSpeedX->setPos(423, 185);
    fKnobOrbitSpeedX->setStep(1.0f);
    fKnobOrbitSpeedX->setRange(1.0f, 128.0f);
    fKnobOrbitSpeedX->setValue(4.0f);
    fKnobOrbitSpeedX->setRotationAngle(270);
    fKnobOrbitSpeedX->setCallback(this);

    // knob KnobOrbitSpeedY
    fKnobOrbitSpeedY = new ImageKnob(this, knobImage);
    fKnobOrbitSpeedY->setPos(516, 185);
    fKnobOrbitSpeedY->setStep(1.0f);
    fKnobOrbitSpeedY->setRange(1.0f, 128.0f);
    fKnobOrbitSpeedY->setValue(4.0f);
    fKnobOrbitSpeedY->setRotationAngle(270);
    fKnobOrbitSpeedY->setCallback(this);

    // knob KnobOrbitSizeX
    fKnobOrbitSizeX = new ImageKnob(this, knobImage);
    fKnobOrbitSizeX->setPos(423, 73);
    fKnobOrbitSizeX->setRange(0.0f, 1.0f);
    fKnobOrbitSizeX->setValue(0.5f);
    fKnobOrbitSizeX->setRotationAngle(270);
    fKnobOrbitSizeX->setCallback(this);

    // knob KnobOrbitSizeY
    fKnobOrbitSizeY = new ImageKnob(this, knobImage);
    fKnobOrbitSizeY->setPos(516, 73);
    fKnobOrbitSizeY->setRange(0.0f, 1.0f);
    fKnobOrbitSizeY->setValue(0.5f);
    fKnobOrbitSizeY->setRotationAngle(270);
    fKnobOrbitSizeY->setCallback(this);

    // knob KnobSubOrbitSpeed
    fKnobSubOrbitSpeed = new ImageKnob(this, knobImage);
    fKnobSubOrbitSpeed->setPos(620, 185);
    fKnobSubOrbitSpeed->setStep(1.0f);
    fKnobSubOrbitSpeed->setRange(1.0f, 128.0f);
    fKnobSubOrbitSpeed->setValue(32.0f);
    fKnobSubOrbitSpeed->setRotationAngle(270);
    fKnobSubOrbitSpeed->setCallback(this);

    // knob KnobSubOrbitSize
    fKnobSubOrbitSize = new ImageKnob(this, knobImage);
    fKnobSubOrbitSize->setPos(620, 73);
    fKnobSubOrbitSize->setRange(0.0f, 1.0f);
    fKnobSubOrbitSize->setValue(0.5f);
    fKnobSubOrbitSize->setRotationAngle(270);
    fKnobSubOrbitSize->setCallback(this);

    // knob KnobSubOrbitSmooth
    fKnobSubOrbitSmooth = new ImageKnob(this, knobImage);
    fKnobSubOrbitSmooth->setPos(620, 297);
    fKnobSubOrbitSmooth->setRange(0.0f, 1.0f);
    fKnobSubOrbitSmooth->setValue(0.5f);
    fKnobSubOrbitSmooth->setRotationAngle(270);
    fKnobSubOrbitSmooth->setCallback(this);

    // sliders
    Image sliderImage(VectorJuiceArtwork::sliderData, VectorJuiceArtwork::sliderWidth, VectorJuiceArtwork::sliderHeight);
    Point<int> sliderPosStart(410+48, 284);
    Point<int> sliderPosEnd(410, 284);

    // slider OrbitWaveX
    fSliderOrbitWaveX = new ImageSlider(this, sliderImage);
    fSliderOrbitWaveX->setStartPos(sliderPosStart);
    fSliderOrbitWaveX->setEndPos(sliderPosEnd);
    fSliderOrbitWaveX->setRange(1.0f, 4.0f);
    fSliderOrbitWaveX->setValue(3.0f);
    fSliderOrbitWaveX->setCallback(this);

    // slider OrbitWaveY
    sliderPosStart.setX(503+48);
    sliderPosEnd.setX(503);
    fSliderOrbitWaveY = new ImageSlider(this, sliderImage);
    fSliderOrbitWaveY->setStartPos(sliderPosStart);
    fSliderOrbitWaveY->setEndPos(sliderPosEnd);
    fSliderOrbitWaveY->setRange(1.0f, 4.0f);
    fSliderOrbitWaveY->setStep(1.0f);
    fSliderOrbitWaveY->setValue(3.0f);
    fSliderOrbitWaveY->setCallback(this);

    // slider OrbitPhaseX
    sliderPosStart.setX(410+48);
    sliderPosStart.setY(345);
    sliderPosEnd.setX(410);
    sliderPosEnd.setY(345);
    fSliderOrbitPhaseX = new ImageSlider(this, sliderImage);
    fSliderOrbitPhaseX->setStartPos(sliderPosStart);
    fSliderOrbitPhaseX->setEndPos(sliderPosEnd);
    fSliderOrbitPhaseX->setRange(0.0f, 1.0f);
    fSliderOrbitPhaseX->setStep(1.0f);
    fSliderOrbitPhaseX->setValue(0.0f);
    fSliderOrbitPhaseX->setCallback(this);

    // slider OrbitPhaseY
    sliderPosStart.setX(503+48);
    sliderPosEnd.setX(503);
    fSliderOrbitPhaseY = new ImageSlider(this, sliderImage);
    fSliderOrbitPhaseY->setStartPos(sliderPosStart);
    fSliderOrbitPhaseY->setEndPos(sliderPosEnd);
    fSliderOrbitPhaseY->setRange(0.0f, 1.0f);
    fSliderOrbitPhaseY->setStep(1.0f);
    fSliderOrbitPhaseY->setValue(0.0f);
    fSliderOrbitPhaseY->setCallback(this);
}

VectorJuiceUI::~VectorJuiceUI()
{
    delete fButtonAbout;

    //knobs
    delete fKnobOrbitSpeedX;
    delete fKnobOrbitSpeedY;
    delete fKnobOrbitSizeX;
    delete fKnobOrbitSizeY;
    delete fKnobSubOrbitSpeed;
    delete fKnobSubOrbitSize;
    delete fKnobSubOrbitSmooth;

    //sliders
    delete fSliderOrbitWaveX;
    delete fSliderOrbitWaveY;
    delete fSliderOrbitPhaseX;
    delete fSliderOrbitPhaseY;
}

// -----------------------------------------------------------------------
// DSP Callbacks

void VectorJuiceUI::d_parameterChanged(uint32_t index, float value)
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

void VectorJuiceUI::d_programChanged(uint32_t index)
{
    if (index != 0)
        return;

    // Default values
    paramX = paramY = 0.5f;
    fKnobOrbitSpeedX->setValue(4.0f);
    fKnobOrbitSpeedY->setValue(4.0f);
    fKnobOrbitSizeX->setValue(1.0f);
    fKnobOrbitSizeY->setValue(1.0f);
    fKnobSubOrbitSize->setValue(1.0f);
    fKnobSubOrbitSpeed->setValue(32.0f);
    fKnobSubOrbitSmooth->setValue(0.5f);
    fSliderOrbitWaveX->setValue(3.0f);
    fSliderOrbitWaveY->setValue(3.0f);
    fSliderOrbitPhaseX->setValue(0.0f);
    fSliderOrbitPhaseY->setValue(0.0f);
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
    if (knob == fKnobOrbitSpeedX)
        d_editParameter(VectorJuicePlugin::paramOrbitSpeedX, true);
    else if (knob == fKnobOrbitSpeedY)
        d_editParameter(VectorJuicePlugin::paramOrbitSpeedY, true);
    else if (knob == fKnobOrbitSizeX)
        d_editParameter(VectorJuicePlugin::paramOrbitSizeX, true);
    else if (knob == fKnobOrbitSizeY)
        d_editParameter(VectorJuicePlugin::paramOrbitSizeY, true);
    else if (knob == fKnobSubOrbitSize)
        d_editParameter(VectorJuicePlugin::paramSubOrbitSize, true);
    else if (knob == fKnobSubOrbitSpeed)
        d_editParameter(VectorJuicePlugin::paramSubOrbitSpeed, true);
    else if (knob == fKnobSubOrbitSmooth)
        d_editParameter(VectorJuicePlugin::paramSubOrbitSmooth, true);
}

void VectorJuiceUI::imageKnobDragFinished(ImageKnob* knob)
{
    if (knob == fKnobOrbitSpeedX)
        d_editParameter(VectorJuicePlugin::paramOrbitSpeedX, false);
    else if (knob == fKnobOrbitSpeedY)
        d_editParameter(VectorJuicePlugin::paramOrbitSpeedY, false);
    else if (knob == fKnobOrbitSizeX)
        d_editParameter(VectorJuicePlugin::paramOrbitSizeX, false);
    else if (knob == fKnobOrbitSizeY)
        d_editParameter(VectorJuicePlugin::paramOrbitSizeY, false);
    else if (knob == fKnobSubOrbitSize)
        d_editParameter(VectorJuicePlugin::paramSubOrbitSize, false);
    else if (knob == fKnobSubOrbitSpeed)
        d_editParameter(VectorJuicePlugin::paramSubOrbitSpeed, false);
    else if (knob == fKnobSubOrbitSmooth)
        d_editParameter(VectorJuicePlugin::paramSubOrbitSmooth, false);
}

void VectorJuiceUI::imageKnobValueChanged(ImageKnob* knob, float value)
{
    if (knob == fKnobOrbitSpeedX)
        d_setParameterValue(VectorJuicePlugin::paramOrbitSpeedX, value);
    else if (knob == fKnobOrbitSpeedY)
        d_setParameterValue(VectorJuicePlugin::paramOrbitSpeedY, value);
    else if (knob == fKnobOrbitSizeX)
        d_setParameterValue(VectorJuicePlugin::paramOrbitSizeX, value);
    else if (knob == fKnobOrbitSizeY)
        d_setParameterValue(VectorJuicePlugin::paramOrbitSizeY, value);
    else if (knob == fKnobSubOrbitSize)
        d_setParameterValue(VectorJuicePlugin::paramSubOrbitSize, value);
    else if (knob == fKnobSubOrbitSpeed)
        d_setParameterValue(VectorJuicePlugin::paramSubOrbitSpeed, value);
    else if (knob == fKnobSubOrbitSmooth)
        d_setParameterValue(VectorJuicePlugin::paramSubOrbitSmooth, value);
}

void VectorJuiceUI::imageSliderDragStarted(ImageSlider* slider)
{
    if (slider == fSliderOrbitWaveX)
        d_editParameter(VectorJuicePlugin::paramOrbitWaveX, true);
    else if (slider == fSliderOrbitWaveY)
        d_editParameter(VectorJuicePlugin::paramOrbitWaveY, true);
    else if (slider == fSliderOrbitPhaseX)
        d_editParameter(VectorJuicePlugin::paramOrbitPhaseX, true);
    else if (slider == fSliderOrbitPhaseY)
        d_editParameter(VectorJuicePlugin::paramOrbitPhaseY, true);
}

void VectorJuiceUI::imageSliderDragFinished(ImageSlider* slider)
{
    if (slider == fSliderOrbitWaveX)
        d_editParameter(VectorJuicePlugin::paramOrbitWaveX, false);
    else if (slider == fSliderOrbitWaveY)
        d_editParameter(VectorJuicePlugin::paramOrbitWaveY, false);
    else if (slider == fSliderOrbitPhaseX)
        d_editParameter(VectorJuicePlugin::paramOrbitPhaseX, false);
    else if (slider == fSliderOrbitPhaseY)
        d_editParameter(VectorJuicePlugin::paramOrbitPhaseY, false);
}

void VectorJuiceUI::imageSliderValueChanged(ImageSlider* slider, float value)
{
    if (slider == fSliderOrbitWaveX)
        d_setParameterValue(VectorJuicePlugin::paramOrbitWaveX, value);
    else if (slider == fSliderOrbitWaveY)
        d_setParameterValue(VectorJuicePlugin::paramOrbitWaveY, value);
    else if (slider == fSliderOrbitPhaseX)
        d_setParameterValue(VectorJuicePlugin::paramOrbitPhaseX, value);
    else if (slider == fSliderOrbitPhaseY)
        d_setParameterValue(VectorJuicePlugin::paramOrbitPhaseY, value);
}

void VectorJuiceUI::onDisplay()
{
    fImgBackground.draw();

    /*
    // TESTING - remove later
    // this paints the 'fCanvasArea' so we can clearly see its bounds
    {
        const int x = fCanvasArea.getX();
        const int y = fCanvasArea.getY();
        const int w = fCanvasArea.getWidth();
        const int h = fCanvasArea.getHeight();

        glColor4f(0.0f, 1.0f, 0.0f, 0.1f);

        glBegin(GL_QUADS);
          glTexCoord2f(0.0f, 0.0f);
          glVertex2i(x, y);

          glTexCoord2f(1.0f, 0.0f);
          glVertex2i(x+w, y);

          glTexCoord2f(1.0f, 1.0f);
          glVertex2i(x+w, y+h);

          glTexCoord2f(0.0f, 1.0f);
          glVertex2i(x, y+h);
        glEnd();

        // reset color
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    */

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
    fImgRoundlet.draw(x, y);
    fImgOrbit.draw(nOrbitX, nOrbitY);
    fImgSubOrbit.draw(nSubOrbitX, nSubOrbitY);
}

bool VectorJuiceUI::onMouse(int button, bool press, int x, int y)
{
    if (button != 1)
        return false;

    if (press)
    {
        if (! fCanvasArea.contains(x, y))
            return false;

        fDragging = true;
        fDragValid = true;
        fLastX = x;
        fLastY = y;
        return true;
    }
    else if (fDragging)
    {
        fDragging = false;
        return true;
    }

    return false;
}

bool VectorJuiceUI::onMotion(int x, int y)
{
    if (! fDragging)
        return false;
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
        d_setParameterValue(VectorJuicePlugin::paramX, paramX);
        repaint();
    }

    if (newY != paramY)
    {
        paramY = newY;
        d_setParameterValue(VectorJuicePlugin::paramY, paramY);
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
