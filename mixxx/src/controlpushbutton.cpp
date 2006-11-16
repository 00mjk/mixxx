/***************************************************************************
                          controlpushbutton.cpp  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
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

#include "controlpushbutton.h"

/* -------- ------------------------------------------------------
   Purpose: Creates a new simulated latching push-button. 
   Input:   key - Key for the configuration file
   -------- ------------------------------------------------------ */
ControlPushButton::ControlPushButton(ConfigKey key, bool bMidiSimulateLatching) : ControlObject(key)
{
    m_bMidiSimulateLatching = bMidiSimulateLatching;
    m_bIsToggleButton = false;
}

ControlPushButton::~ControlPushButton()
{
}

//Tell this PushButton whether or not it's a "toggle" push button...
void ControlPushButton::setToggleButton(bool bIsToggleButton)
{
	m_bIsToggleButton = bIsToggleButton;
}

void ControlPushButton::setValueFromMidi(MidiCategory c, double v)
{
    //if (m_bMidiSimulateLatching)
    
        //qDebug("bMidiSimulateLatching is true!");
        // Only react on NOTE_ON midi events if simulating latching...
        
        
    if (m_bIsToggleButton) //This block makes push-buttons act as toggle buttons.
    { 
        qDebug("Is a toggle button!");
        if (c==NOTE_ON && v>0.) //Only react to "NOTE_ON" midi events.
        {
        	//qDebug("NOTE_ON caught!");
            if (m_dValue==0.)
                m_dValue = 1.;
            else
                m_dValue = 0.;
        }
    }
    else //Not a toggle button (trigger only when button pushed)
    {
        qDebug("Is NOT a toggle button!");
    	if (c == NOTE_ON)
    		m_dValue = v;
    	else if (c == NOTE_OFF)
    		m_dValue = 0;
    }
    if (c==NOTE_OFF)
        {

        	//qDebug("NOTE_OFF caught!");
        }
  
    
/*    else
    {
    	qDebug("bMidiSimulateLatching is false!");
    	qDebug("m_dValue is: %f, v is: %f", m_dValue, v);
    	
        if (v==0.)
            m_dValue = 0.;
        else
            m_dValue = 1.;
        
    }
*/
/*
        if (v>0.)
        {
            if (m_dValue==0.)
                m_dValue = 1.;
            else
                m_dValue = 0.;
        }
*/
    emit(valueChanged(m_dValue));
}

