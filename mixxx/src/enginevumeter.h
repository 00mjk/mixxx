/***************************************************************************
                          enginevumeter.h  -  description
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

#ifndef ENGINEVUMETER_H
#define ENGINEVUMETER_H

#include "engineobject.h"

// Rate at which the vumeter is updated (using a sample rate of 44100 Hz):
#define UPDATE_RATE 5

class ControlPotmeter;

class EngineVuMeter : public EngineObject {
public:
    EngineVuMeter(const char *);
    ~EngineVuMeter();
    void process(const CSAMPLE *pIn, const CSAMPLE *pOut, const int iBufferSize);

private:
    ControlPotmeter *m_ctrlVuMeter;
    FLOAT_TYPE m_fRMSvolume;
    FLOAT_TYPE m_iSamplesCalculated;
};

#endif
