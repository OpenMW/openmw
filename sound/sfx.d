/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (sfx.d) is part of the OpenMW package.

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

module sound.sfx;

import sound.audiere;
import sound.audio;

import core.config;
import core.resource;

import std.string;

// Handle for a sound resource. This struct represents one sound
// effect file (not a music file, those are handled differently
// because of their size.) From this handle we may get instances,
// which may be played and managed independently of each other. TODO:
// Let the resource manager worry about only opening one resource per
// file, when to kill resources, etc.
struct SoundFile
{
  AudiereResource res;
  char[] name;
  bool loaded;
  
  private int refs;

  private void fail(char[] msg)
  {
    throw new SoundException(format("SoundFile '%s'", name), msg);
  }

  // Load a sound resource.
  void load(char[] file)
  {
    // Make sure the string is null terminated
    assert(*(file.ptr+file.length) == 0);

    name = file;

    loaded = true;
    refs = 0;

    res = cpp_openSound(file.ptr);

    if(!res) fail("Failed to open sound file " ~ file);
  }

  // Get an instance of this resource.
  SoundInstance getInstance()
  {
    SoundInstance si;
    si.owner = this;
    si.inst = cpp_createInstance(res);
    if(!si.inst) fail("Failed to instantiate sound resource");
    refs++;

    return si;
  }

  // Return the sound instance when you're done with it
  private void returnInstance(AudiereInstance inst)
  {
    refs--;
    cpp_destroyInstance(inst);
    if(refs == 0) unload();
  }

  // Unload the resource.
  void unload()
  {
    loaded = false;
    cpp_closeSound(res);
  }
}

struct SoundInstance
{
  AudiereInstance inst;
  SoundFile *owner;
  float volume, min, max;
  float xx, yy, zz; // 3D position
  bool playing;
  bool repeat;

  // Return this instance to the owner
  void kill()
  {
    owner.returnInstance(inst);
  }

  // Start playing a sound.
  void play()
  {
    playing = true;
    cpp_playSound(inst);
    if(repeat) cpp_setRepeat(inst);
  }

  // Go buy a cookie
  void stop()
  {
    cpp_stopSound(inst);
    playing = false;
  }

  // Set parameters such as max volume and range
  void setParams(float volume, float minRange, float maxRange, bool repeat=false)
  in
  {
    assert(volume >= 0 && volume <= 1.0, "Volume out of range");
  }
  body
  {
    this.volume = volume;
    min = minRange;
    max = maxRange;
    this.repeat = repeat;
    playing = false;
  }

  // Set 3D position of sound
  void setPos(float x, float y, float z)
  {
    xx = x;
    yy = y;
    zz = z;
  }

  // Currently VERY experimental, panning disabled. At some point we
  // will likely switch to OpenAL with built-in 3D sound and dump this
  // entirely.
  void setPlayerPos(float x, float y, float z)
  {
    //writef("{%s %s %s} ", x, y, z);
    // Distance squared
    x -= xx;
    y -= yy;
    z -= zz;
    //writef("[%s %s %s] ", x, y, z);
    float r2 = (x*x + y*y + z*z);
    //writefln(r2, " (%s)", max*max);

    // If outside range, disable
    if(r2 > max*max)
      {
	// We just moved out of range
	if(playing)
	  {
	    //writefln("Out of range");
	    stop();
	  }
      }
    else
      {
	// We just moved into range
	if(!playing)
	  {
	    //writefln("In range!");
	    play();
	  }
      }

    if(!playing) return;

    // Invert distance
    if(r2 < 1) r2 = 1;
    else r2 = 1/r2;

    float vol = 2*r2*min*min;
    float pan = 0;//80*x*r2;

    //writefln("x=%s, vol=%s, pan=%s", x, vol, pan);

    if(vol>1.0) vol = 1.0;
    if(pan<-1.0) pan = -1.0;
    else if(pan > 1.0) pan = 1.0;
    //writefln("vol=", vol, " volume=", vol*volume*config.calcSfxVolume());

    cpp_setParams(inst, vol*volume*config.calcSfxVolume(), pan);
  }
}
