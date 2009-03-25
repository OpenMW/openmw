/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (music.d) is part of the OpenMW package.

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

module sound.music;

import sound.avcodec;
import sound.audio;
import sound.al;

import monster.monster;

import std.stdio;
import std.string;

import core.config;
import core.resource;

class Idle_waitUntilFinished : IdleFunction
{
 override:
  IS initiate(Thread*) { return IS.Poll; }

  bool hasFinished(Thread* trd)
  {
    Jukebox mgr = cast(Jukebox)trd.extraData.obj;
    assert(mgr !is null);

    // Return when the music is no longer playing
    return !mgr.isPlaying();
  }
}

struct Music
{
  private:
  static:

  // The Monster classes and objects
  MonsterClass jukeC, controlC;

  public:

  MonsterObject *controlM;

  void init()
  {
    assert(jukeC is null);
    assert(controlC is null);
    assert(controlM is null);

    jukeC = MonsterClass.get("Jukebox");
    jukeC.bind("waitUntilFinished",
            new Idle_waitUntilFinished);

    jukeC.bind("setSound", { Jukebox.get().setSound(); });
    jukeC.bind("setVolume", { Jukebox.get().setVolume(); });
    jukeC.bind("playSound", { Jukebox.get().playSound(); });
    jukeC.bind("stopSound", { Jukebox.get().stopSound(); });

    jukeC.bindConst({new Jukebox(params.obj()); });

    controlC = MonsterClass.get("Music");
    controlM = controlC.getSing();
    controlM.call("setup");
  }

  private void pushSArray(char[][] strs)
  {
    AIndex arr[];
    arr.length = strs.length;

    // Create the array indices for each element string
    foreach(i, ref elm; arr)
      elm = arrays.create(strs[i]).getIndex();

    // Push the final array
    stack.pushArray(arr);
  }

  void setPlaylists(char[][] normal, char[][] battle)
  {
    if(controlM is null) return;

    pushSArray(normal);
    pushSArray(battle);
    controlM.call("setPlaylists");
  }

  void play()
  {
    if(controlM !is null)
      controlM.call("play");
  }

  void toggle()
  {
    if(controlM !is null)
      controlM.call("toggle");
  }

  void toggleMute()
  {
    if(controlM !is null)
      controlM.call("toggleMute");
  }

  void updateBuffers()
  {
    if(controlM is null) return;

    foreach(ref MonsterObject b; jukeC)
      Jukebox.get(b).updateBuffers();
  }

  void shutdown()
  {
    if(controlM is null) return;

    foreach(ref MonsterObject b; jukeC)
      Jukebox.get(b).shutdown();
  }
}

// Simple music player, has a playlist and can pause/resume
// music. Most of the high-level code is in mscripts/jukebox.mn.
class Jukebox
{
  private:

  // Maximum buffer length, divided up among OpenAL buffers
  const uint bufLength = 128*1024;
  const uint numBufs = 4;

  char[] getName()
    {
      if(mo is null) return "(no name)";
      else return mo.getString8("name");
    }

  void fail(char[] msg)
  {
    throw new SoundException(getName() ~ " Jukebox", msg);
  }

  ALuint sID; // Sound id
  ALuint bIDs[numBufs]; // Buffers

  ALenum bufFormat;
  ALint bufRate;

  AVFile fileHandle;
  AVAudio audioHandle;

  static ubyte[] outData;

  static this()
  {
    outData.length = bufLength / numBufs;
  }

  // The jukebox Monster object
  MonsterObject *mo;

  public:

  this(MonsterObject *m)
  {
    mo = m;
    m.getExtra(Music.jukeC).obj = this;

    sID = 0;
    bIDs[] = 0;
  }

  static Jukebox get(MonsterObject *m)
    {
      auto j = cast(Jukebox)m.getExtra(Music.jukeC).obj;
      assert(j !is null);
      assert(j.mo == m);
      return j;
    }

  static Jukebox get(ref MonsterObject m)
    { return get(&m); }

  static Jukebox get()
  { return get(params.obj()); }

  private:

  // Disable music
  void shutdown()
  {
    mo.call("stop");

    if(fileHandle) avc_closeAVFile(fileHandle);
    fileHandle = null;
    audioHandle = null;

    if(sID)
      {
        alSourceStop(sID);
        alDeleteSources(1, &sID);
        checkALError("disabling music");
        sID = 0;

        alDeleteBuffers(bIDs.length, bIDs.ptr);
        checkALError("deleting buffers");
        bIDs[] = 0;
      }
  }

  void setSound()
  {
    char[] fname = stack.popString8();

    // Generate a source to play back with if needed
    if(sID == 0)
      {
        alGenSources(1, &sID);
        checkALError("generating buffers");

        // Set listner relative coordinates (sound follows the player)
        alSourcei(sID, AL_SOURCE_RELATIVE, AL_TRUE);

        alGenBuffers(bIDs.length, bIDs.ptr);
      }
    else
      {
        // Kill current track, but keep the sID source.
        alSourceStop(sID);
        checkALError("stopping current track");

        alSourcei(sID, AL_BUFFER, 0);
        //alDeleteBuffers(bIDs.length, bIDs.ptr);
        //bIDs[] = 0;
        checkALError("resetting buffer");
      }

    if(fileHandle) avc_closeAVFile(fileHandle);
    fileHandle = null;
    audioHandle = null;

    //alGenBuffers(bIDs.length, bIDs.ptr);

    // If something fails, clean everything up.
    scope(failure) shutdown();

    fileHandle = avc_openAVFile(toStringz(fname));
    if(!fileHandle)
      fail("Unable to open " ~ fname);

    audioHandle = avc_getAVAudioStream(fileHandle, 0);
    if(!audioHandle)
      fail("Unable to load music track " ~ fname);

    int rate, ch, bits;
    if(avc_getAVAudioInfo(audioHandle, &rate, &ch, &bits) != 0)
      fail("Unable to get info for music track " ~ fname);

    // Translate format from avformat to OpenAL

    bufRate = rate;
    bufFormat = 0;

    // TODO: These don't really fail gracefully for 4 and 6 channels
    // if these aren't supported.
    if(bits == 8)
      {
        if(ch == 1) bufFormat = AL_FORMAT_MONO8;
        if(ch == 2) bufFormat = AL_FORMAT_STEREO8;
        if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
          {
            if(ch == 4) bufFormat = alGetEnumValue("AL_FORMAT_QUAD8");
            if(ch == 6) bufFormat = alGetEnumValue("AL_FORMAT_51CHN8");
          }
      }
    if(bits == 16)
      {
        if(ch == 1) bufFormat = AL_FORMAT_MONO16;
        if(ch == 2) bufFormat = AL_FORMAT_STEREO16;
        if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
          {
            if(ch == 4) bufFormat = alGetEnumValue("AL_FORMAT_QUAD16");
            if(ch == 6) bufFormat = alGetEnumValue("AL_FORMAT_51CHN16");
          }
      }

    if(bufFormat == 0)
      fail(format("Unhandled format (%d channels, %d bits) for music track %s",
                  ch, bits, fname));

    // Fill the buffers
    foreach(int i, ref b; bIDs)
      {
        int length = avc_getAVAudioData(audioHandle, outData.ptr, outData.length);
        if(length) alBufferData(b, bufFormat, outData.ptr, length, bufRate);
        if(length == 0 || !noALError())
          {
            if(i == 0)
              fail("No audio data in music track " ~ fname);

            alDeleteBuffers(bIDs.length-i, bIDs.ptr+i);
            checkALError("running alDeleteBuffers");
            bIDs[i..$] = 0;
            break;
          }
      }

    // Associate the buffers with the sound id
    alSourceQueueBuffers(sID, bIDs.length, bIDs.ptr);
  }

  void setVolume()
  {
    float volume = stack.popFloat();

    volume = saneVol(volume);

    // Set the new volume
    if(sID) alSourcef(sID, AL_GAIN, volume);
  }

  void playSound()
  {
    if(!sID) return;

    alSourcePlay(sID);
    checkALError("starting music");
  }

  void stopSound()
  {
    // How to stop / pause music
    if(sID) alSourcePause(sID);
  }

  bool isPlaying()
  {
    ALint state;
    alGetSourcei(sID, AL_SOURCE_STATE, &state);
    checkALError("getting status");

    return state == AL_PLAYING;
  }

  void updateBuffers()
  {
    if(!sID || !isPlaying)
      return;

    // Get the number of processed buffers
    ALint count;
    alGetSourcei(sID, AL_BUFFERS_PROCESSED, &count);

    checkALError("getting number of unprocessed buffers");

    for(int i = 0;i < count;i++)
      {
        int length = avc_getAVAudioData(audioHandle, outData.ptr, outData.length);
        if(length <= 0)
          break;

        ALuint bid;
        alSourceUnqueueBuffers(sID, 1, &bid);
        if(noALError())
          {
            alBufferData(bid, bufFormat, outData.ptr, length, bufRate);
            alSourceQueueBuffers(sID, 1, &bid);
            checkALError("queueing buffers");
          }
      }
  }
}
