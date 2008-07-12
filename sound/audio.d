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

ALCdevice  *Device = null;
ALCcontext *Context = null;

// Temporarilly use ALUT for data loading until something better is picked
extern (C) ALboolean alutInitWithoutContext(int *argc, char **argv);
extern (C) ALboolean alutExit();

class SoundException : Exception
{
  this(char[] caller, char[] msg) { super(caller ~ " SoundException: " ~ msg); }
}

MusicManager jukebox;
MusicManager battleMusic;

void initializeSound()
{
  Device = alcOpenDevice(null);
  Context = alcCreateContext(Device, null);

  if(!Device || !Context)
    throw new SoundException("initializeSound()",
			     "Failed to initialize music device");

  alcMakeContextCurrent(Context);
  alutInitWithoutContext(null, null);

  // Gross HACK: We should use the default model (inverse distance clamped).
  // But without a proper rolloff factor, distance attenuation is completely
  // wrong. This at least makes sure the max distance is the 'silence' point
  alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

  jukebox.initialize("Main");
  battleMusic.initialize("Battle");
}

void shutdownSound()
{
  jukebox.disableMusic();
  battleMusic.disableMusic();

  alutExit();

  alcMakeContextCurrent(null);
  if(Context) alcDestroyContext(Context);
  Context = null;
  if(Device) alcCloseDevice(Device);
  Device = null;
}

ALenum checkALError()
{
    ALenum err = alGetError();
    if(err != AL_NO_ERROR)
        writefln("WARNING: Received AL error (%x): %s", err,
                 toString(alGetString(err)));
    return err;
}
