#ifndef __VINYLCONTROLXWAX_H__
#define __VINYLCONTROLXWAX_H__

#include "vinylcontrol.h"

extern "C" {
#include "timecoder.h"
}

#define XWAX_DEVICE_FRAME 32
#define XWAX_SMOOTHING (128 / XWAX_DEVICE_FRAME) /* result value is in frames */


class VinylControlXwax : public VinylControl
{
    public:
        VinylControlXwax(ConfigObject<ConfigValue> *pConfig, const char *_group);
        ~VinylControlXwax();
    	void ToggleVinylControl(bool enable);
    	bool isEnabled();
    	void AnalyseSamples(short* samples, size_t size);      
protected:
	void run();						// main thread loop

private:
	void syncPitch(double pitch);
	void syncPosition();

	double dFileLength; 			//The length (in samples) of the current song.

	double dOldPos;   				//The position read last time it was polled.
	double dOldDiff;  				//The old difference between the positions. (used to check if the needle's on the record...)
	double dOldPitch;

	bool bNeedleDown; 				//Is the needle on the record? (used for needle dropping)
	bool bSeeking; 					//Are we seeking through the record? (ie. is it moving really fast?)

    //TODO: Comment me!
    struct timecoder_t timecoder;

	short*  m_samples;
	size_t  m_SamplesSize;

	bool		   bShouldClose;
	bool		   bIsRunning;
};        

#endif
