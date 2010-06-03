/***************************************************************************
                          enginesidechain.cpp
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

/* This class provides a way to do audio processing that does not need
 * to be executed in real-time. For example, m_shoutcast encoding/broadcasting
 * and recording encoding can be done here. This class uses double-buffering
 * to increase the amount of time the CPU has to do whatever work needs to
 * be done, and that work is executed in a separate thread. (Threading
 * allows the next buffer to be filled while processing a buffer that's is
 * already full.)
 *
 */

#include <QtCore>
#include <QtDebug>
#include "enginesidechain.h"
#include "enginebuffer.h"

#ifdef __SHOUTCAST__
#include "engineshoutcast.h"
#endif

#include "recording/enginerecord.h"


EngineSideChain::EngineSideChain(ConfigObject<ConfigValue> * pConfig)
{
    m_pConfig = pConfig;
    m_bStopThread = false;

    m_bufferFront = new CSAMPLE[SIDECHAIN_BUFFER_SIZE];
    m_bufferBack  = new CSAMPLE[SIDECHAIN_BUFFER_SIZE];
    m_buffer = m_bufferFront;

    m_iBufferEnd = 0; 

#ifdef __SHOUTCAST__
    // Shoutcast
    ControlObject* m_pShoutcastNeedUpdateFromPrefs = new ControlObject(ConfigKey("[Shoutcast]","update_from_prefs"));
    m_pShoutcastNeedUpdateFromPrefsCOTM = new ControlObjectThreadMain(m_pShoutcastNeedUpdateFromPrefs);
	m_shoutcast = new EngineShoutcast(m_pConfig);
#endif
	
 	m_recReadyCO = new ControlObject(ConfigKey("[Master]", "Record"));
    m_recReady = new ControlObjectThread(m_recReadyCO);
    m_rec = new EngineRecord(m_pConfig);

   	start(QThread::LowPriority);    //Starts the thread and goes to the "run()" function below.
}

EngineSideChain::~EngineSideChain()
{
    m_backBufferLock.lock();

    m_stopLock.lock();
    m_bStopThread = true;
    m_stopLock.unlock();

    m_waitLock.lock();
    m_waitForFullBuffer.wakeAll();
    m_waitLock.unlock();

    wait(); //Wait until the thread has finished.

#ifdef __SHOUTCAST__
    if (m_shoutcast)
        m_shoutcast->shutdown();
#endif

    //Free up memory
    delete [] m_bufferFront;
    delete [] m_bufferBack;

#ifdef __SHOUTCAST__
    delete m_shoutcast;
#endif
	delete m_recReadyCO;
	delete m_recReady;
	if(m_rec)
    	delete m_rec;

    m_backBufferLock.unlock();
}

/** Submit a buffer of samples to be processed in the sidechain*/
void EngineSideChain::submitSamples(CSAMPLE* newBuffer, int buffer_size)
{
    //Copy samples into m_buffer.
    if (m_iBufferEnd + buffer_size <= SIDECHAIN_BUFFER_SIZE)    //FIXME: is <= correct?
    {
        memcpy(&m_buffer[m_iBufferEnd], newBuffer, buffer_size * sizeof(CSAMPLE));
        m_iBufferEnd += buffer_size;
    }
    else //If the new buffer won't fit, copy as much of it as we can over and then copy the rest after swapping.
    {
        memcpy(&m_buffer[m_iBufferEnd], newBuffer, (SIDECHAIN_BUFFER_SIZE - m_iBufferEnd)*sizeof(CSAMPLE));
        //Save the number of samples written because m_iBufferEnd gets reset in swapBuffers:
        int iNumSamplesWritten = SIDECHAIN_BUFFER_SIZE - m_iBufferEnd;

        // This will block the callback thread if the buffering overflows. As of
        // 10/2009 this lock is only used to protect the buffer pointers, so it
        // won't cause blocking.
        m_backBufferLock.lock();
        swapBuffers(); //Swaps buffers and resets m_iBufferEnd to zero.
        m_backBufferLock.unlock();

        //Since we swapped buffers, we now have a full buffer that needs processing.
        m_waitLock.lock();
        m_waitForFullBuffer.wakeAll(); //... so wake the thread up and get processing. :)
        m_waitLock.unlock();

        //Calculate how many leftover samples need to be written to the other buffer.
        int iNumSamplesStillToWrite = buffer_size - iNumSamplesWritten;

        //Check to see if the remaining samples will fit in the other empty buffer.
        if (iNumSamplesStillToWrite > SIDECHAIN_BUFFER_SIZE)
        {
            iNumSamplesStillToWrite = SIDECHAIN_BUFFER_SIZE; //Drop samples if they won't fit.
            qDebug() << "EngineSideChain warning: dropped samples";
        }
        memcpy(&m_buffer[m_iBufferEnd], &newBuffer[iNumSamplesWritten], iNumSamplesStillToWrite*sizeof(CSAMPLE));
        m_iBufferEnd += iNumSamplesStillToWrite;

    }
}

/* Swaps the buffers in the double-buffering mechanism we use */
void EngineSideChain::swapBuffers()
{
    if (m_buffer == m_bufferFront)
    {
        m_buffer = m_bufferBack;
        m_filledBuffer = m_bufferFront;
    }
    else
    {
        m_buffer = m_bufferFront;
        m_filledBuffer = m_bufferBack;
    }

    m_iBufferEnd = 0;
}

void EngineSideChain::run()
{
    unsigned static id = 0; //the id of this thread, for debugging purposes //XXX copypasta (should factor this out somehow), -kousu 2/2009
    QThread::currentThread()->setObjectName(QString("EngineSideChain %1").arg(++id));
    while (true)
    {
        m_waitLock.lock();
        m_waitForFullBuffer.wait(&m_waitLock);  //Sleep until the buffer has been filled.
        m_waitLock.unlock();

        //Check to see if we're supposed to exit/stop this thread.
        m_stopLock.lock();
        if (m_bStopThread)
        {
            m_stopLock.unlock();
            return;
        }
        m_stopLock.unlock();


        //This portion of the code should be able to touch the buffer without having to use
        //the m_bufferLock mutex, because the buffers should have been swapped.

        //IMPORTANT: The filled buffer is "m_filledBuffer" - that's the audio we need to process here.

        //Do CPU intensive and non-realtime processing here.

        //m_backBufferLock.lock(); //This will cause the audio/callback thread to block if the buffers overflow,
                                   //so don't even think about enabling this. (I'm leaving it here as a
                                   //warning to anyone who wants to work on this code in the future.) - Albert

        // We need to use this lock when copying the pointer to the buffer or
        // else we could end up with a bogus pointer. We don't have to hold the
        // lock during the processing though.

        m_backBufferLock.lock();
        CSAMPLE* pBuffer = m_filledBuffer;
        m_backBufferLock.unlock();

#ifdef __SHOUTCAST__

        //Important note: We're "allowed" to access a ConfigKey here (below) because it doesn't take place
        //                in the callback thread. ConfigKey access is slow, but we can afford the performance
        //                hit here.

        //Check to see if Shoutcast is enabled, and pass the samples off to be broadcast if necessary.

        bool prefEnabled = (m_pConfig->getValueString(ConfigKey("[Shoutcast]","enabled")).toInt() == 1);
        

        
        if (prefEnabled) {
			if(!m_shoutcast->isConnected()){
				//Initialize the m_pShout structure with the info from Mixxx's m_shoutcast preferences.
				m_shoutcast->updateFromPreferences();
				if(m_shoutcast->serverConnect()){	

					ErrorDialogProperties* props = ErrorDialogHandler::instance()->newDialogProperties();
				    props->setType(DLG_INFO);
				    props->setTitle(tr("Live broadcasting"));
				    props->setText(tr("Mixxx has successfully connected to the m_shoutcast server"));
				    
				    ErrorDialogHandler::instance()->requestErrorDialog(props);
				}
				
			}
			//send to soutcast
			m_shoutcast->process(pBuffer, pBuffer, SIDECHAIN_BUFFER_SIZE);

			if (m_pShoutcastNeedUpdateFromPrefsCOTM->get() > 0.0f){
				/*
				 * You cannot change bitrate, hostname, etc while connected to a stream
				 */
				m_shoutcast->serverDisconnect(); 
				m_shoutcast->updateFromPreferences();
				m_shoutcast->serverConnect();  
			}	
       	}
		else{
			if(m_shoutcast->isConnected()){
				m_shoutcast->serverDisconnect(); 
				ErrorDialogProperties* props = ErrorDialogHandler::instance()->newDialogProperties();
				props->setType(DLG_INFO);
			 	props->setTitle(tr("Live broadcasting"));
				props->setText(tr("Mixxx has successfully disconnected to the m_shoutcast server"));
				    
				ErrorDialogHandler::instance()->requestErrorDialog(props);
			}
		} 
#endif
	
		//if recording is disabled			
		if(m_recReady->get() == RECORD_OFF){
			if(m_rec->fileOpen()){
				m_rec->closeFile();	//close file and 
			}
		}
		//if we are ready for recording, we open a new file
		if(m_recReady->get() == RECORD_READY){
			m_rec->updateFromPreferences();	//update file location from pref
			if(m_rec->openFile()){
				qDebug("Setting record flag to: ON");
            	m_recReady->slotSet(RECORD_ON);
			}
			else{ //Maybe the encoder could not be initialized
				qDebug("Setting record flag to: OFF");
            	m_recReady->slotSet(RECORD_OFF);
			}	
		}
		if(m_recReady->get() == RECORD_ON){	
        	m_rec->process(pBuffer, pBuffer, SIDECHAIN_BUFFER_SIZE);
		}
        //m_backBufferLock.unlock();
		
    }

}


