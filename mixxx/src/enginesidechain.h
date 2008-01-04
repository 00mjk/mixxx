/***************************************************************************
                          enginesidechain.h
                             -------------------
    copyright            : (C) 2008 Albert Santoni
    email                : gamegod \a\t users.sf.net
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef ENGINESIDECHAIN_H
#define ENGINESIDECHAIN_H

#include "defs.h"
#include "configobject.h"
#include "controlobject.h"

#ifdef __SHOUTCAST__
class EngineShoutcast;
#endif

#define SIDECHAIN_BUFFER_SIZE 4096



class EngineSideChain : public QThread
{
    public:
        EngineSideChain(ConfigObject<ConfigValue> * pConfig);
        ~EngineSideChain();
        void submitSamples(CSAMPLE* buffer, int buffer_size);
        void run();
        void swapBuffers();
    private:
        ConfigObject<ConfigValue> * m_pConfig;
        const char* m_group;
        unsigned long m_iBufferEnd;             //Index of the last sample in the buffer.
        CSAMPLE* m_buffer;                      //Pointer to the fillable giant buffer (for double-buffering)
        CSAMPLE* m_filledBuffer;                //Pointer to the filled giant buffer (after swapping).
        CSAMPLE* m_bufferFront;                 //Giant buffer to store audio.
        CSAMPLE* m_bufferBack;                  //Another giant buffer to store audio.
        QMutex m_bufferLock;                    //Provides thread safety for the buffer.
        QMutex m_waitLock;                      //Provides thread safety around the wait condition below.
        QWaitCondition m_waitForFullBuffer;     //Allows sleeping until we have a full buffer.

#ifdef __SHOUTCAST__
        EngineShoutcast *shoutcast;
#endif
};

#endif
