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

#include "GrooveJuiceUI.hpp"

using DGL::Point;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

GrooveJuiceUI::GrooveJuiceUI()
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
    fImgBackground = Image(GrooveJuiceArtwork::backgroundData, GrooveJuiceArtwork::backgroundWidth, GrooveJuiceArtwork::backgroundHeight, GL_BGR);

    //roundlet
    fImgRoundlet = Image(GrooveJuiceArtwork::roundletData, GrooveJuiceArtwork::roundletWidth, GrooveJuiceArtwork::roundletHeight);

    //orbit
    fImgOrbit = Image(GrooveJuiceArtwork::orbitData, GrooveJuiceArtwork::orbitWidth, GrooveJuiceArtwork::orbitHeight);

    //subOrbit
    fImgSubOrbit = Image(GrooveJuiceArtwork::subOrbitData, GrooveJuiceArtwork::subOrbitWidth, GrooveJuiceArtwork::subOrbitHeight);

    // about
    Image imageAbout(GrooveJuiceArtwork::aboutData, GrooveJuiceArtwork::aboutWidth, GrooveJuiceArtwork::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // about button
    Image aboutImageNormal(GrooveJuiceArtwork::aboutButtonNormalData, GrooveJuiceArtwork::aboutButtonNormalWidth, GrooveJuiceArtwork::aboutButtonNormalHeight);
    Image aboutImageHover(GrooveJuiceArtwork::aboutButtonHoverData, GrooveJuiceArtwork::aboutButtonHoverWidth, GrooveJuiceArtwork::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(599, 17);
    fButtonAbout->setCallback(this);

    Image pageImageNormal(GrooveJuiceArtwork::pageButtonNormalData, GrooveJuiceArtwork::pageButtonNormalWidth, GrooveJuiceArtwork::pageButtonNormalHeight);
    Image pageImageHover(GrooveJuiceArtwork::pageButtonHoverData, GrooveJuiceArtwork::pageButtonHoverWidth, GrooveJuiceArtwork::pageButtonHoverHeight);

    Image randomizeImageNormal(GrooveJuiceArtwork::randomizeButtonNormalData, GrooveJuiceArtwork::randomizeButtonNormalWidth, GrooveJuiceArtwork::randomizeButtonNormalHeight);
    Image randomizeImageHover(GrooveJuiceArtwork::randomizeButtonHoverData, GrooveJuiceArtwork::randomizeButtonHoverWidth, GrooveJuiceArtwork::randomizeButtonHoverHeight);

    fButtonRandomize = new ImageButton(this, randomizeImageNormal, randomizeImageHover, randomizeImageHover);
    fButtonRandomize -> setPos(313, 586);
    fButtonRandomize -> setCallback(this);

    int oX = 15;
    int oY = 557;
    int mX = 103-oX;
    int mY = 0;
    for (int i=0; i<8; i++) {
		fButtonsPage[i] = new ImageButton(this, pageImageNormal, pageImageHover, pageImageHover);
		fButtonsPage[i]->setPos(oX+mX*i, oY);
		fButtonsPage[i]->setCallback(this);
    }

    // knobs
    Image knobImage(GrooveJuiceArtwork::knobData, GrooveJuiceArtwork::knobWidth, GrooveJuiceArtwork::knobHeight);
    Image knob2Image(GrooveJuiceArtwork::knob2Data, GrooveJuiceArtwork::knob2Width, GrooveJuiceArtwork::knob2Height);

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
    Image sliderImage(GrooveJuiceArtwork::sliderData, GrooveJuiceArtwork::sliderWidth, GrooveJuiceArtwork::sliderHeight);
    Point<int> sliderPosStart(410, 284);
    Point<int> sliderPosEnd(410+48, 284);

    // slider OrbitWaveX
    fSliderOrbitWaveX = new ImageSlider(this, sliderImage);
    fSliderOrbitWaveX->setStartPos(sliderPosStart);
    fSliderOrbitWaveX->setEndPos(sliderPosEnd);
    fSliderOrbitWaveX->setRange(1.0f, 4.0f);
    fSliderOrbitWaveX->setStep(1.0f);
    fSliderOrbitWaveX->setValue(3.0f);
    fSliderOrbitWaveX->setCallback(this);

    // slider OrbitWaveY
    sliderPosStart.setX(503);
    sliderPosEnd.setX(503+48);
    fSliderOrbitWaveY = new ImageSlider(this, sliderImage);
    fSliderOrbitWaveY->setStartPos(sliderPosStart);
    fSliderOrbitWaveY->setEndPos(sliderPosEnd);
    fSliderOrbitWaveY->setRange(1.0f, 4.0f);
    fSliderOrbitWaveY->setStep(1.0f);
    fSliderOrbitWaveY->setValue(3.0f);
    fSliderOrbitWaveY->setCallback(this);

    // slider OrbitPhaseX
    sliderPosStart.setX(410);
    sliderPosStart.setY(345);
    sliderPosEnd.setX(410+48);
    sliderPosEnd.setY(345);
    fSliderOrbitPhaseX = new ImageSlider(this, sliderImage);
    fSliderOrbitPhaseX->setStartPos(sliderPosStart);
    fSliderOrbitPhaseX->setEndPos(sliderPosEnd);
    fSliderOrbitPhaseX->setRange(0.0f, 1.0f);
    //fSliderOrbitPhaseX->setStep(1.0f); // FIXME?
    fSliderOrbitPhaseX->setValue(0.0f);
    fSliderOrbitPhaseX->setCallback(this);

    // slider OrbitPhaseY
    sliderPosStart.setX(503);
    sliderPosEnd.setX(503+48);
    fSliderOrbitPhaseY = new ImageSlider(this, sliderImage);
    fSliderOrbitPhaseY->setStartPos(sliderPosStart);
    fSliderOrbitPhaseY->setEndPos(sliderPosEnd);
    fSliderOrbitPhaseY->setRange(0.0f, 1.0f);
    //fSliderOrbitPhaseY->setStep(1.0f); // FIXME?
    fSliderOrbitPhaseY->setValue(0.0f);
    fSliderOrbitPhaseY->setCallback(this);

    //knobs graphics
    oX = 25; //offset
    oY = 480;
    mX = 113-oX; //margin
    mY = 545-oY;
    oX-=9;
    oY-=9;

    page = 0;



    //synth Knobs
	for (int x=0; x<8; x++) {
		fKnobsSynth[x] = new ImageKnob(this, knob2Image);
		fKnobsSynth[x]->setPos(oX+mX*x, oY);
		//fKnobsSynth[x]->setStep(1.0f);
		fKnobsSynth[x]->setRange(0.0f, 1.0f);
		fKnobsSynth[x]->setValue(0.5f);
		fKnobsSynth[x]->setRotationAngle(270);
		fKnobsSynth[x]->setCallback(this);
	}


	//default synthData
	for (int x=0; x<8; x++)
		for (int y=0; y<8; y++)
			synthData[x][y] = 0.5;



	//default squares
	oX = 20;
	oY = 47;
	mX = 372/8;
	for (int x=0; x<8; x++) {
		for (int y=0; y<8; y++) {
			squares[x][y].timer = 0;
			squares[x][y].maxTimer = d_getSampleRate()/8000;
			squares[x][y].x=oX+mX*x;
			squares[x][y].y=oY+mX*y;
			squares[x][y].size = mX;
		}
	}

	tabOX = 15;
	tabOY = 552;
	tabW = 64;
	tabH = 5;
	tabPosX = tabOX;
	tabTargetPosX = tabPosX;
	tabMarginX = 103-tabOX;
}

GrooveJuiceUI::~GrooveJuiceUI()
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

void GrooveJuiceUI::d_parameterChanged(uint32_t index, float value)
{


	if (index<17 || index>=17+64) {
	    switch (index)
	    {
	    case GrooveJuicePlugin::paramX:
		   if (paramX != value)
		   {
			  paramX = value;
			  fDragValid = false;
			  repaint();
		   }
		   break;
	    case GrooveJuicePlugin::paramY:
		   if (paramY != value)
		   {
			  paramY = value;
			  fDragValid = false;
			  repaint();
		   }
		   break;
	    case GrooveJuicePlugin::paramOrbitSizeX:
		   fKnobOrbitSizeX->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitSizeY:
		   fKnobOrbitSizeY->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitSpeedX:
		   fKnobOrbitSpeedX->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitSpeedY:
		   fKnobOrbitSpeedY->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramSubOrbitSize:
		   fKnobSubOrbitSize->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramSubOrbitSpeed:
		   fKnobSubOrbitSpeed->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramSubOrbitSmooth:
		   fKnobSubOrbitSmooth->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitWaveX:
		   fSliderOrbitWaveX->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitWaveY:
		   fSliderOrbitWaveY->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitPhaseX:
		   fSliderOrbitPhaseX->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitPhaseY:
		   fSliderOrbitPhaseY->setValue(value);
		   break;
	    case GrooveJuicePlugin::paramOrbitOutX:
		   if (orbitX != value)
		   {
			  orbitX = value;
			  repaint();
		   }
		   break;
	    case GrooveJuicePlugin::paramOrbitOutY:
		   if (orbitY != value)
		   {
			  orbitY = value;
			  repaint();
		   }
		   break;
	    case GrooveJuicePlugin::paramSubOrbitOutX:
		   if (subOrbitX != value)
		   {
			  subOrbitX = value;
			  repaint();
		   }
		   break;
	    case GrooveJuicePlugin::paramSubOrbitOutY:
		   if (subOrbitY != value)
		   {
			  subOrbitY = value;
			  repaint();
		   }
		   break;
		case GrooveJuicePlugin::paramW1Out:
			synthSound[0] = value;
			repaint();
			break;
		case GrooveJuicePlugin::paramW2Out:
			synthSound[1] = value;
			repaint();
			break;
		case GrooveJuicePlugin::paramMOut:
			synthSound[2] = value;
			repaint();
			break;
		case GrooveJuicePlugin::paramCOut:
			synthSound[3] = value;
			repaint();
			break;
		case GrooveJuicePlugin::paramROut:
			synthSound[4] = value;
			repaint();
			break;
		case GrooveJuicePlugin::paramSOut:
			synthSound[5] = value;
			repaint();
			break;
		case GrooveJuicePlugin::paramReOut:
			synthSound[6] = value;
			repaint();
			break;
		case GrooveJuicePlugin::paramShOut:
			synthSound[7] = value;
			repaint();
			break;
	    }
    } else {
		//synth param changed
		int num = (index-17); //synth params begin on #17
		int x = num%8; //synth param
		//synth page
		int y = (num-(num%8))/8;
		synthData[x][y] = value;
    }
}

void GrooveJuiceUI::d_programChanged(uint32_t index)
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

void GrooveJuiceUI::imageButtonClicked(ImageButton* button, int)
{
	if (button == fButtonAbout)
		fAboutWindow.exec();

	if (button == fButtonRandomize) {
		for (int y=0; y<8; y++) {
			for (int x=0; x<8; x++) {
				synthData[x][y] = getRandom();
				d_setParameterValue(17+y*8+x, synthData[x][y]);

			}
		}

		for (int x=0; x<8; x++) {
			fKnobsSynth[x]->setValue(synthData[x][page]);
		}

	}

    for (int i=0; i<8; i++) {
		if (button == fButtonsPage[i]) {
			page = i;
			tabTargetPosX = tabOX+page*tabMarginX;
			for (int x=0; x<8; x++) {
				fKnobsSynth[x]->setValue(synthData[x][page]);
			}
		}
    }


}

void GrooveJuiceUI::imageKnobDragStarted(ImageKnob* knob)
{

    if (knob == fKnobOrbitSpeedX)
        d_editParameter(GrooveJuicePlugin::paramOrbitSpeedX, true);
    else if (knob == fKnobOrbitSpeedY)
        d_editParameter(GrooveJuicePlugin::paramOrbitSpeedY, true);
    else if (knob == fKnobOrbitSizeX)
        d_editParameter(GrooveJuicePlugin::paramOrbitSizeX, true);
    else if (knob == fKnobOrbitSizeY)
        d_editParameter(GrooveJuicePlugin::paramOrbitSizeY, true);
    else if (knob == fKnobSubOrbitSize)
        d_editParameter(GrooveJuicePlugin::paramSubOrbitSize, true);
    else if (knob == fKnobSubOrbitSpeed)
        d_editParameter(GrooveJuicePlugin::paramSubOrbitSpeed, true);
    else if (knob == fKnobSubOrbitSmooth)
        d_editParameter(GrooveJuicePlugin::paramSubOrbitSmooth, true);

	for (int i=0; i<8; i++) {
		if (knob== fKnobsSynth[i]) {
			d_editParameter(i+17+(page*8), true);

		}
	}
}

void GrooveJuiceUI::imageKnobDragFinished(ImageKnob* knob)
{

    if (knob == fKnobOrbitSpeedX)
        d_editParameter(GrooveJuicePlugin::paramOrbitSpeedX, false);
    else if (knob == fKnobOrbitSpeedY)
        d_editParameter(GrooveJuicePlugin::paramOrbitSpeedY, false);
    else if (knob == fKnobOrbitSizeX)
        d_editParameter(GrooveJuicePlugin::paramOrbitSizeX, false);
    else if (knob == fKnobOrbitSizeY)
        d_editParameter(GrooveJuicePlugin::paramOrbitSizeY, false);
    else if (knob == fKnobSubOrbitSize)
        d_editParameter(GrooveJuicePlugin::paramSubOrbitSize, false);
    else if (knob == fKnobSubOrbitSpeed)
        d_editParameter(GrooveJuicePlugin::paramSubOrbitSpeed, false);
    else if (knob == fKnobSubOrbitSmooth)
        d_editParameter(GrooveJuicePlugin::paramSubOrbitSmooth, false);

	for (int i=0; i<8; i++) {
		if (knob== fKnobsSynth[i]) {
			d_editParameter(i+17+page*8, false);
		}
	}
}

void GrooveJuiceUI::imageKnobValueChanged(ImageKnob* knob, float value)
{

    if (knob == fKnobOrbitSpeedX)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitSpeedX, value);
    else if (knob == fKnobOrbitSpeedY)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitSpeedY, value);
    else if (knob == fKnobOrbitSizeX)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitSizeX, value);
    else if (knob == fKnobOrbitSizeY)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitSizeY, value);
    else if (knob == fKnobSubOrbitSize)
        d_setParameterValue(GrooveJuicePlugin::paramSubOrbitSize, value);
    else if (knob == fKnobSubOrbitSpeed)
        d_setParameterValue(GrooveJuicePlugin::paramSubOrbitSpeed, value);
    else if (knob == fKnobSubOrbitSmooth)
        d_setParameterValue(GrooveJuicePlugin::paramSubOrbitSmooth, value);

	for (int i=0; i<8; i++) {
		if (knob== fKnobsSynth[i]) {
			synthData[i][page] = value;
			d_setParameterValue(i+17+page*8, value);
		}
	}
}

void GrooveJuiceUI::imageSliderDragStarted(ImageSlider* slider)
{
    if (slider == fSliderOrbitWaveX)
        d_editParameter(GrooveJuicePlugin::paramOrbitWaveX, true);
    else if (slider == fSliderOrbitWaveY)
        d_editParameter(GrooveJuicePlugin::paramOrbitWaveY, true);
    else if (slider == fSliderOrbitPhaseX)
        d_editParameter(GrooveJuicePlugin::paramOrbitPhaseX, true);
    else if (slider == fSliderOrbitPhaseY)
        d_editParameter(GrooveJuicePlugin::paramOrbitPhaseY, true);
}

void GrooveJuiceUI::imageSliderDragFinished(ImageSlider* slider)
{
    if (slider == fSliderOrbitWaveX)
        d_editParameter(GrooveJuicePlugin::paramOrbitWaveX, false);
    else if (slider == fSliderOrbitWaveY)
        d_editParameter(GrooveJuicePlugin::paramOrbitWaveY, false);
    else if (slider == fSliderOrbitPhaseX)
        d_editParameter(GrooveJuicePlugin::paramOrbitPhaseX, false);
    else if (slider == fSliderOrbitPhaseY)
        d_editParameter(GrooveJuicePlugin::paramOrbitPhaseY, false);
}

void GrooveJuiceUI::imageSliderValueChanged(ImageSlider* slider, float value)
{
    if (slider == fSliderOrbitWaveX)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitWaveX, value);
    else if (slider == fSliderOrbitWaveY)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitWaveY, value);
    else if (slider == fSliderOrbitPhaseX)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitPhaseX, value);
    else if (slider == fSliderOrbitPhaseY)
        d_setParameterValue(GrooveJuicePlugin::paramOrbitPhaseY, value);
}

void GrooveJuiceUI::onDisplay()
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

	//draw tab highlight

	tabPosX -= (tabPosX-tabTargetPosX)/3;
	glColor4f(0.5f, 0.5f, 1.0f, 1.0f);
	glBegin(GL_POLYGON);
		glVertex2i(tabPosX, tabOY);
		glVertex2i(tabPosX+tabW, tabOY);
		glVertex2i(tabPosX+tabW, tabOY+tabH);
		glVertex2i(tabPosX, tabOY+tabH);
	glEnd();

	//draw real knob values
	int kPosX = 6;
	int kPosY = 550;
	int kH = 112;
	int kW = 83;
	int kM = 94-kPosX;
	glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
	for (int i=0; i<8; i++) {
		glBegin(GL_POLYGON);
			glVertex2i(kPosX+kM*i, kPosY);
			glVertex2i(kPosX+kM*i+kW, kPosY);
			glVertex2i(kPosX+kM*i+kW, kPosY-synthSound[i]*kH);
			glVertex2i(kPosX+kM*i, kPosY-synthSound[i]*kH);
		glEnd();

	}

	/*for (int x=0; x<8; x++) {
			for (int y=0; y<8; y++) {
				if (isWithinSquare(orbitX, orbitY, x/8.0, y/8.0)) {
					if (squares[x][y].timer<squares[x][y].maxTimer) {
						squares[x][y].timer++;

					}
				} else {
					if (squares[x][y].timer>0) {
						squares[x][y].timer--;
					}
				}
				if (squares[x][y].timer>0) {
					//draw this square
					glColor4f(0.0f, 1.0f, 0.0f, squares[x][y].timer/squares[x][y].maxTimer/8);
					//printf("blend: %f\n", squares[x][y].timer/squares[x][y].maxTimer);
					glBegin(GL_POLYGON);
						glVertex2i(squares[x][y].x, squares[x][y].y);
						glVertex2i(squares[x][y].x+squares[x][y].size, squares[x][y].y);
						glVertex2i(squares[x][y].x+squares[x][y].size, squares[x][y].y+squares[x][y].size);
						glVertex2i(squares[x][y].x, squares[x][y].y+squares[x][y].size);
					glEnd();

				}
			}
	}*/


	// reset color
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// draw roundlet and orbits
	fImgRoundlet.draw(x, y);
	fImgOrbit.draw(nOrbitX, nOrbitY);
	fImgSubOrbit.draw(nSubOrbitX, nSubOrbitY);



}

bool GrooveJuiceUI::onMouse(int button, bool press, int x, int y)
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

bool GrooveJuiceUI::onMotion(int x, int y)
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
        d_setParameterValue(GrooveJuicePlugin::paramX, paramX);
        repaint();
    }

    if (newY != paramY)
    {
        paramY = newY;
        d_setParameterValue(GrooveJuicePlugin::paramY, paramY);
        repaint();
    }

    return true;
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new GrooveJuiceUI();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
