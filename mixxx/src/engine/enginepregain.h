/***************************************************************************
                          enginepregain.h  -  description
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

#ifndef ENGINEPREGAIN_H
#define ENGINEPREGAIN_H

#include "engine/engineobject.h"
#include "controlobject.h"


class ControlLogpotmeter;
class ControlPotmeter;
class ControlObject;
class EnginePregain : public EngineObject
{
public:
    EnginePregain( const char *group);
    ~EnginePregain();
    void process(const CSAMPLE *pIn, const CSAMPLE *pOut, const int iBufferSize);



private:
    ControlLogpotmeter *potmeterPregain;
    ControlPotmeter *m_pReplayGainBoost, *m_pEnableReplayGain;
    ControlObject *m_pControlReplayGain, *m_pTotalGain;
    float m_fReplayGainCorrection;
    bool m_bSmoothFade;
    float m_fClock;
    float m_fSumClock;

};

#endif
