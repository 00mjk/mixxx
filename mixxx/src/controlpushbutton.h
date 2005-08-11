/***************************************************************************
                          controlpushbutton.h  -  description
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

#ifndef CONTROLPUSHBUTTON_H
#define CONTROLPUSHBUTTON_H

#include <qaction.h>
#include "controlobject.h"
#include "defs.h"

/**
  *@author Tue and Ken Haste Andersen
  */
  
class ControlPushButton : public ControlObject
{
    Q_OBJECT
public:
    ControlPushButton(ConfigKey key, bool bMidiSimulateLatching=false);
    ~ControlPushButton();

protected:
    void setValueFromMidi(MidiCategory c, double v);

private:
    bool m_bMidiSimulateLatching;
};

#endif
