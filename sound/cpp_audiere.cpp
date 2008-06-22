/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_audiere.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#include <audiere.h>
#include <iostream>
using namespace audiere;
using namespace std;

/*
 * Sound, using Audiere. Creates a very simple C interface for
 * opening, closing and manipulating sounds.
 */

AudioDevicePtr device;

extern "C" int cpp_openDevice()
{
  device = OpenDevice("");
  if (!device) return 1;
  return 0;
}

// Opens a new sample buffer from a file
extern "C" SampleBuffer *cpp_openSound(char* filename)
{
  SampleSourcePtr sample = OpenSampleSource(filename);
  if(!sample) return NULL;
  SampleBufferPtr buf = CreateSampleBuffer(sample);
  buf->ref();
  return buf.get();
}

// Delete a sample buffer
extern "C" void cpp_closeSound(SampleBuffer *buf)
{
  buf->unref();
}

// Get an output stream from a sample buffer.
extern "C" OutputStream *cpp_createInstance(SampleBuffer *buf)
{
  SampleSourcePtr sample = buf->openStream();
  if(!sample) return NULL;

  OutputStreamPtr sound = OpenSound(device, sample, false);
  
  sound->ref();
  return sound.get();
}

// Stream a file directly. Used for music.
extern "C" OutputStream *cpp_playStream(char* filename, float volume)
{
  OutputStreamPtr sound = OpenSound(device, filename, true);
  if(sound)
    {
      sound->ref();
      sound->setVolume(volume);
      sound->play();
    }
  return sound.get();
}

extern "C" void cpp_destroyInstance(OutputStream *sound)
{
  sound->unref();
}

extern "C" int cpp_isPlaying(OutputStream *sound)
{
  if(sound && sound->isPlaying()) return 1;
  return 0;
}

extern "C" void cpp_setParams(OutputStream *sound, float vol, float pan)
{
  if(sound)
    {
      sound->setVolume(vol);
      sound->setPan(pan);
    }
}

extern "C" void cpp_playSound(OutputStream *sound)
{
  sound->play();
}

extern "C" void cpp_setRepeat(OutputStream *sound)
{
  sound->setRepeat(true);
}

// Stop the sound
extern "C" void cpp_stopSound(OutputStream *sound)
{
  if(sound) sound->stop();
}
