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

import sound.avcodec;
import sound.audio;
import sound.al;

import core.config;
import core.resource;

import std.string;
import std.stdio;

// Handle for a sound resource. This struct represents one sound
// effect file (not a music file, those are handled differently
// because of their size.) From this handle we may get instances,
// which may be played and managed independently of each other. TODO:
// Let the resource manager worry about only opening one resource per
// file, when to kill resources, etc.
struct SoundFile
{
  ALuint bID;
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
    name = file;

    loaded = false;
    refs = 0;
    bID = 0;

    ubyte[] outData;
    AVFile fileHandle = cpp_openAVFile(toStringz(file));
    AVAudio audioHandle = cpp_getAVAudioStream(fileHandle, 0);

    if(!fileHandle)
      {
        writefln("Unable to open %s", file);
        goto errclose;
      }
    if(!audioHandle)
      {
        writefln("Unable to load sound %s", file);
        goto errclose;
      }

    int ch, bits, rate;
    if(cpp_getAVAudioInfo(audioHandle, &rate, &ch, &bits) != 0)
      {
        writefln("Unable to get info for sound %s", file);
        goto errclose;
      }

    int fmt = 0;
    if(bits == 8)
      {
        if(ch == 1) fmt = AL_FORMAT_MONO8;
        if(ch == 2) fmt = AL_FORMAT_STEREO8;
        if(ch == 4) fmt = alGetEnumValue("AL_FORMAT_QUAD8");
      }
    if(bits == 16)
      {
        if(ch == 1) fmt = AL_FORMAT_MONO16;
        if(ch == 2) fmt = AL_FORMAT_STEREO16;
        if(ch == 4) fmt = alGetEnumValue("AL_FORMAT_QUAD16");
      }

    if(fmt == 0)
      {
        writefln("Unhandled format (%d channels, %d bits) for sound %s", ch, bits, file);
        goto errclose;
      }

    int total = 0;
    do
      {
        // Grow by an arbitrary amount. Should be big enough to get the
        // whole sound in one or two iterations, but not allocate too much
        // memory in case its short
        outData.length = outData.length+8192;
        int length = cpp_getAVAudioData(audioHandle, outData.ptr+total, outData.length-total);
        total += length;
      }
    while(total == outData.length);

    if(total)
      {
        alGenBuffers(1, &bID);
        alBufferData(bID, fmt, outData.ptr, total, rate);
        if(checkALError() != AL_NO_ERROR)
          {
            writefln("Unable to load sound %s", file);
            alDeleteBuffers(1, &bID);
            bID = 0;
          }
        else loaded = true;
      }

  errclose:
    if(fileHandle) cpp_closeAVFile(fileHandle);
    fileHandle = null;
    audioHandle = null;
  }

  // Get an instance of this resource.
  // FIXME: Should not call fail() here since it's quite possible for this to
  // fail (on hardware drivers). When it does, it should check for an existing
  // sound it doesn't need and kill it, then try again
  SoundInstance getInstance()
  {
    SoundInstance si;
    si.owner = this;
    alGenSources(1, &si.inst);
    if(checkALError() != AL_NO_ERROR || !si.inst)
      fail("Failed to instantiate sound resource");

    alSourcei(si.inst, AL_BUFFER, cast(ALint)bID);
    if(checkALError() != AL_NO_ERROR)
      {
        alDeleteSources(1, &si.inst);
        fail("Failed to load sound resource");
      }
    refs++;

    return si;
  }

  // Return the sound instance when you're done with it
  private void returnInstance(ALuint sid)
  {
    refs--;
    alSourceStop(sid);
    alDeleteSources(1, &sid);
    if(refs == 0) unload();
  }

  // Unload the resource.
  void unload()
  {
    loaded = false;
    alDeleteBuffers(1, &bID);
  }
}

struct SoundInstance
{
  ALuint inst;
  SoundFile *owner;

  // Return this instance to the owner
  void kill()
  {
    owner.returnInstance(inst);
    owner = null;
  }

  // Start playing a sound.
  void play()
  {
    alSourcePlay(inst);
    checkALError();
  }

  // Go buy a cookie
  void stop()
  {
    alSourceStop(inst);
    checkALError();
  }

  // Set parameters such as max volume and range
  void setParams(float volume, float minRange, float maxRange, bool repeat=false)
  in
  {
    assert(volume >= 0 && volume <= 1.0, "Volume out of range");
  }
  body
  {
    alSourcef(inst, AL_GAIN, volume);
    alSourcef(inst, AL_REFERENCE_DISTANCE, minRange);
    alSourcef(inst, AL_MAX_DISTANCE, maxRange);
    alSourcei(inst, AL_LOOPING, repeat ? AL_TRUE : AL_FALSE);
    alSourcePlay(inst);
    checkALError();
  }

  // Set 3D position of sounds. Need to convert from app's world coords to
  // standard left-hand coords
  void setPos(float x, float y, float z)
  {
    alSource3f(inst, AL_POSITION, x, z, -y);
    checkALError();
  }

  static void setPlayerPos(float x, float y, float z,
                           float frontx, float fronty, float frontz,
                           float upx, float upy, float upz)
  {
    ALfloat orient[6];
    orient[0] = frontx;
    orient[1] = frontz;
    orient[2] =-fronty;
    orient[3] = upx;
    orient[4] = upz;
    orient[5] =-upy;
    alListener3f(AL_POSITION, x, z, -y);
    alListenerfv(AL_ORIENTATION, orient.ptr);
    checkALError();
  }
}
