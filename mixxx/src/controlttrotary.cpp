/***************************************************************************
                          controlttrotary.cpp  -  description
                             -------------------
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

#include "controlttrotary.h"

/* -------- ------------------------------------------------------
   Purpose: Creates a new rotary encoder
   Input:   key
   -------- ------------------------------------------------------ */
ControlTTRotary::ControlTTRotary(ConfigKey key) : ControlObject(key)
{
}

double ControlTTRotary::getValueFromWidget(double dValue)
{
    // Non-linear scaling
    double temp = (((dValue-64.)*(dValue-64.))/64.)/100.;
    if ((dValue-64.)<0)
        return -temp;
    else
        return temp;
}

double ControlTTRotary::getValueToWidget(double dValue)
{
    return dValue*200.+64.;
}

void ControlTTRotary::setValueFromMidi(MidiCategory, double v)
{
    m_dValue = getValueFromWidget(v);
    emit(valueChanged(m_dValue));
}

