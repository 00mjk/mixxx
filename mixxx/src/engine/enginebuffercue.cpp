/***************************************************************************
                          enginebuffercue.cpp  -  description
                             -------------------
    copyright            : (C) 2005 by Tue Haste Andersen
    email                :
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QtCore>
#include "playerinfo.h"
#include "trackinfoobject.h"
#include "controlpushbutton.h"
#include "controlobject.h"
#include "enginebuffercue.h"
#include "enginebuffer.h"
#include "mathstuff.h"

EngineBufferCue::EngineBufferCue(const char * group, EngineBuffer * pEngineBuffer)
{
    m_pEngineBuffer = pEngineBuffer;
    m_bCuePreview = false;
    m_group = group;

    // Get pointer to play button
    playButton = ControlObject::getControl(ConfigKey(group, "play"));
    connect(playButton, SIGNAL(valueChanged(double)), this, SLOT(slotControlPlay(double)));

    // Cue set button:
    buttonCueSet = new ControlPushButton(ConfigKey(group, "cue_set"));
    connect(buttonCueSet, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueSet(double)));

    // Cue goto button:
    buttonCueGoto = new ControlPushButton(ConfigKey(group, "cue_goto"));
    connect(buttonCueGoto, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueGoto(double)));

    // Cue goto and stop button:
    buttonCueGotoAndStop = new ControlPushButton(ConfigKey(group, "cue_gotoandstop"));
    connect(buttonCueGotoAndStop, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueGotoAndStop(double)));

    // Cue "simple-style" button:
    buttonCueSimple = new ControlPushButton(ConfigKey(group, "cue_simple"));
    connect(buttonCueSimple, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueSimple(double)));

    // Cue point
    cuePoint = new ControlObject(ConfigKey(group, "cue_point"));

    // Cue preview button:
    buttonCuePreview = new ControlPushButton(ConfigKey(group, "cue_preview"));
    connect(buttonCuePreview, SIGNAL(valueChanged(double)), this, SLOT(slotControlCuePreview(double)));

    // Cue button CDJ style
    buttonCueCDJ = new ControlPushButton(ConfigKey(group, "cue_cdj"));
    connect(buttonCueCDJ, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueCDJ(double)));
    
    // Cue button generic handler
    buttonCueDefault = new ControlPushButton(ConfigKey(group, "cue_default"));
    connect(buttonCueDefault, SIGNAL(valueChanged(double)), this, SLOT(slotControlCueDefault(double)));
    
    // Cue behavior setting
    m_pControlCueDefault = new ControlObjectThreadMain(ControlObject::getControl(ConfigKey(group,"cue_simple")));
}

EngineBufferCue::~EngineBufferCue()
{
    delete buttonCueSet;
    delete buttonCueGoto;
    delete buttonCueGotoAndStop;
    delete buttonCuePreview;
    delete buttonCueSimple;
    delete buttonCueDefault;
    delete buttonCueCDJ;
    delete m_pControlCueDefault;
    delete cuePoint;
}

void EngineBufferCue::saveCuePoint(double cue)
{
    //Save the cue point inside the TrackInfoObject for that player.
    int channel = m_group[8] - '0'; //The 8th char of [Channel#] is the #.
    TrackInfoObject* currentTrack = PlayerInfo::Instance().getTrackInfo(channel);
    if (currentTrack) //Ensures a track is loaded :)
        currentTrack->setCuePoint(cue);
    else
        Q_ASSERT("currentTrack not found in EngineBufferCue::slotControlCueSet");
}

// Set the cue point at the current play position:
//THIS FUNCTION IS DEPRECATED AND DOESN'T GET USED ANYMORE -- Albert (found out the hard way) Oct 20/08
void EngineBufferCue::slotControlCueSet(double v)
{
    if (v)
    {
        double cue = math_max(0.,round(m_pEngineBuffer->getAbsPlaypos()));
        if (!even((int)cue))
            cue--;
        cuePoint->set(cue);
    
        saveCuePoint(cue);
   }
}

// Goto the cue point:
void EngineBufferCue::slotControlCueGoto(double pos)
{
    if (pos!=0.)
    {
        // Set cue point if play is not pressed
        if (playButton->get()==0.)
        {
            slotControlCueSet();

            // Start playing
            playButton->set(1.);
        }
        else
        {
            // Seek to cue point
            m_pEngineBuffer->slotControlSeekAbs(cuePoint->get(), false);
        }
    }
}

// Goto the cue point and stop, regardless of playback status:
void EngineBufferCue::slotControlCueGotoAndStop(double pos)
{
    if(pos == 0.) return;
    //Seek to the cue point...
    m_pEngineBuffer->slotControlSeekAbs(cuePoint->get(), false);

    //... and stop.
    playButton->set(0.);
}

void EngineBufferCue::slotControlCuePreview(double)
{
    // Set cue point if play is not pressed
    //if (playButton->get()==0.)
    //    slotControlCueSet();

    if (buttonCuePreview->get()==0.)
    {
        // Stop playing (set playbutton to stoped) and seek to cue point
        playButton->set(0.);
        m_bCuePreview = false;
        m_pEngineBuffer->slotControlSeekAbs(cuePoint->get(), false);
    }
    else if (!m_bCuePreview)
    {
        // Seek to cue point and start playing
        m_bCuePreview = true;

        if (playButton->get()==0.)
            playButton->set(1.);
        else
        {
            // Seek to cue point
            m_pEngineBuffer->slotControlSeekAbs(cuePoint->get(), false);
        }
    }
}
//Simple" method for using cue points (Thread's idea):
void EngineBufferCue::slotControlCueSimple(double v)
{
    if (v) //Left-clicked on cue button
    {
        if (playButton->get() == 0.) //If playback is stopped, set a cue-point.
        {
            double cue = math_max(0.,round(m_pEngineBuffer->getAbsPlaypos()));
            if (!even((int)cue))
                cue--;
            cuePoint->set(cue);
            saveCuePoint(cue);
        }
        else  //If playback is ongoing, then jump to the cue-point.
            m_pEngineBuffer->slotControlSeekAbs(cuePoint->get(), false);
    }
}

void EngineBufferCue::slotControlPlay(double v)
{
    if (v==0.)
        slotControlCueSet();
}

void EngineBufferCue::slotControlCueCDJ(double v) {
    /* This is how CDJ cue buttons work:
     * If pressed while playing, stop playback at go to cue.
     * If pressed while stopped and at cue, play while pressed.
     * If pressed while stopped and not at cue, set new cue point.
     * TODO: If play is pressed while holding cue, the deck is now playing.
     */
    
    if ((v==0. && playButton->get()==1.) || playButton->get()==1.)
    // If we are previewing on button release, or the track is currently playing
    // and cue is pressed
    {
        // Stop playing (set playbutton to stopped) and seek to cue point
        playButton->set(0.);
        m_pEngineBuffer->slotControlSeekAbs(cuePoint->get(), false);
    }
    else if (v!=0. && playButton->get()==0.)
    // On button press, If the track is not playing and we are not previewing
    {
        // Get current cue for comparison
        double cue = math_max(0.,round(m_pEngineBuffer->getAbsPlaypos()));
        if (!even((int)cue)) cue--;
        
        if (cue == cuePoint->get()) {
            // If at cue point, start playing
            playButton->set(1.);
        } else {
            // Set new cue point
            cuePoint->set(cue);
            saveCuePoint(cue);
        }
    }
}

void EngineBufferCue::slotControlCueDefault(double v)
{
    // Deicde which cue implementation to call based on the user preference
    if (m_pControlCueDefault->get() == 0) {
        // CDJ Mode
        slotControlCueCDJ(v);
    } else {
        // Simple Mode
        slotControlCueSimple(v);
    }
}

void EngineBufferCue::process(const CSAMPLE *, const CSAMPLE *, const int)
{
}
