/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (audio.d) is part of the OpenMW package.

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

module sound.audio;

public import sound.sfx;
public import sound.music;

import sound.al;
import sound.alc;

import std.stdio;
import std.string;

class SoundException : Exception
{
  this(char[] caller, char[] msg) { super(caller ~ " SoundException: " ~ msg); }
}

ALCdevice  *Device = null;
ALCcontext *Context = null;

void initializeSound()
{
  Device = alcOpenDevice(null);
  Context = alcCreateContext(Device, null);

  if(!Device || !Context)
    throw new SoundException("initializeSound()",
			     "Failed to initialize music device");

  alcMakeContextCurrent(Context);

  Music.init();
}

void shutdownSound()
{
  Music.shutdown();

  alcMakeContextCurrent(null);
  if(Context) alcDestroyContext(Context);
  Context = null;
  if(Device) alcCloseDevice(Device);
  Device = null;
}

float saneVol(float vol)
{
  if(!(vol >= 0)) vol = 0;
  else if(!(vol <= 1)) vol = 1;
  return vol;
}

void checkALError(char[] what)
{
  ALenum err = alGetError();
  what = " while " ~ what;
  if(err != AL_NO_ERROR)
    throw new Exception(format("OpenAL error%s: (%x) %s", what, err,
               toString(alGetString(err))));
}

bool noALError()
{ return alGetError() == AL_NO_ERROR; }
