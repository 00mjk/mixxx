/***************************************************************************
                          sounddevice.cpp
                             -------------------
    begin                : Sun Aug 12, 2007, past my bedtime
    copyright            : (C) 2007 Albert Santoni
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

#ifndef SOUNDDEVICE_H
#define SOUNDDEVICE_H

#include "soundmanager.h"

//Forward declarations
///....
class SoundDevice;
class SoundManager;



class SoundDevice
{
    public:
        SoundDevice(ConfigObject<ConfigValue> *config, SoundManager* sm);
        virtual ~SoundDevice();
        QString getName();
        QString getHostAPI();
        void setHostAPI(QString api);        
        virtual int open() = 0;
        virtual int close() = 0;
        void addSource(AudioSource src);
        void addReceiver(AudioReceiver recv);
        void clearSources();
        void clearReceivers();
        bool operator== (SoundDevice* other);
        bool operator== (QString other);
    protected:
    
    //TODO: Cleanup unused members
        ConfigObject<ConfigValue> *m_pConfig;
        SoundManager* m_pSoundManager;      //Pointer to the SoundManager object which we'll request audio from.
        QString m_strName;                  //The name of the soundcard
        int m_iNumOutputChannels;           //The number of output channels that the soundcard has
        int m_iNumInputChannels;            //The number of input channels that the soundcard has
        int m_iBufferSize;                  //The number of samples in a buffer.
        double m_dSampleRate;               //The current samplerate for the sound device.
        QString m_hostAPI;                  //The name of the audio API used by this device.
        //int m_iLatency;                       //The latency of the soundcard in milliseconds (TODO: Use bufferSize instead?)
        QList<AudioSource> m_audioSources;          //A list containing all the sources that we're going to receive/request audio from.
        QList<AudioReceiver> m_audioReceivers;      //A list containing all the "receivers" that we're going to send audio to.
        //QList<int> m_listActiveOutputChannels;    //A list containing the output channels which are currently active on the soundcard.
        //QList<int> m_listActiveInputChannels;     //A list containing the input channels which are currently active on the soundcard.

};

#endif
