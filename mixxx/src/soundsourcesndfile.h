/***************************************************************************
                          soundsourcesndfile.h  -  description
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

#ifndef SOUNDSOURCESNDFILE_H
#define SOUNDSOURCESNDFILE_H

#include "soundsource.h"
#include <stdio.h>
#include <sndfile.h>
class TrackInfoObject;

class SoundSourceSndFile : public SoundSource {
public:
  SoundSourceSndFile(const char*);
  ~SoundSourceSndFile();
  long seek(long);
  unsigned read(unsigned long size, const SAMPLE*);
  inline long unsigned length();
  static void ParseHeader( TrackInfoObject * );	

 private:
  int channels;
  SNDFILE *fh;
  SF_INFO *info;
  unsigned long filelength;
};

#endif
