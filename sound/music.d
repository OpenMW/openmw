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

import std.stdio;
import std.string;

import core.config;
import core.resource;

// Simple music player, has a playlist and can pause/resume music.
struct MusicManager
{
  private:

  // How much to add to the volume each second when fading
  const float fadeInRate = 0.10;
  const float fadeOutRate = 0.20;

  // Maximum buffer length, divided up among OpenAL buffers
  const uint bufLength = 128*1024;

  // Volume
  ALfloat volume, maxVolume;

  char[] name;

  void fail(char[] msg)
  {
    throw new SoundException(name ~ " Jukebox", msg);
  }

  // List of songs to play
  char[][] playlist;
  int index; // Index of next song to play

  bool musicOn;
  ALuint sID;
  ALuint bIDs[4];

  ALenum bufFormat;
  ALint bufRate;

  AVFile fileHandle;
  AVAudio audioHandle;
  ubyte[] outData;

  // Which direction are we currently fading, if any
  enum Fade {  None = 0, In, Out  }
  Fade fading;

  public:

  // Initialize the jukebox
  void initialize(char[] name)
  {
    this.name = name;
    sID = 0;
    foreach(ref b; bIDs) b = 0;
    outData.length = bufLength / bIDs.length;
    fileHandle = null;
    musicOn = false;
  }

  // Get the new volume setting.
  void updateVolume()
  {
    maxVolume = config.calcMusicVolume();

    if(!musicOn) return;

    // Adjust volume up to new setting, unless we are in the middle of
    // a fade. Even if we are fading, though, the volume should never
    // be over the max.
    if(fading == Fade.None || volume > maxVolume) volume = maxVolume;
    if(sID)
      alSourcef(sID, AL_GAIN, volume);
  }

  // Give a music play list
  void setPlaylist(char[][] pl)
  {
    playlist = pl;
    index = 0;

    randomize();
  }

  // Randomize playlist. If the argument is true, then we don't want
  // the old last to be the new first.
  private void randomize(bool checklast = false)
  {
    if(playlist.length < 2) return;

    // Get the index of the last song played
    int lastidx = ((index==0) ? (playlist.length-1) : (index-1));

    foreach(int i, char[] s; playlist)
      {
        int idx = rnd.randInt(i,playlist.length-1);

        // Don't put the last idx as the first entry
        if(i == 0 && checklast && lastidx == idx)
          {
            idx++;
            if(idx == playlist.length)
              idx = i;
          }
        if(idx == i) /* skip if swapping with self */
          continue;
        playlist[i] = playlist[idx];
        playlist[idx] = s;
      }
  }

  // Skip to the next track
  void playNext()
  {
    // If music is disabled, do nothing
    if(!musicOn) return;

    // No tracks to play?
    if(!playlist.length) return;

    // Generate a source to play back with if needed
    if(!sID)
      {
        alGenSources(1, &sID);
        if(checkALError())
            fail("Couldn't generate music sources");
        alSourcei(sID, AL_SOURCE_RELATIVE, AL_TRUE);

        updateVolume();
      }
    else
      {
        // Kill current track
        alSourceStop(sID);
        alSourcei(sID, AL_BUFFER, 0);
        alDeleteBuffers(bIDs.length, bIDs.ptr);
        bIDs[] = 0;
        checkALError("killing current track");
      }

    if(fileHandle) avc_closeAVFile(fileHandle);
    fileHandle = null;
    audioHandle = null;

    // End of list? Randomize and start over
    if(index == playlist.length)
      {
	randomize(true);
	index = 0;
      }

    alGenBuffers(bIDs.length, bIDs.ptr);

    // If something fails, clean everything up.
    scope(failure)
      {
        // This block is only executed if an exception is thrown.

        if(fileHandle) avc_closeAVFile(fileHandle);

        fileHandle = null;
        audioHandle = null;

        alSourceStop(sID);
        alDeleteSources(1, &sID);
        alDeleteBuffers(bIDs.length, bIDs.ptr);

        checkALError("cleaning up after music failure");

        sID = 0;
        bIDs[] = 0;

        // Try the next track if playNext is called again
        index++;

        // The function exits here.
      }

    if(checkALError())
      fail("Couldn't generate buffers");

    fileHandle = avc_openAVFile(toStringz(playlist[index]));
    if(!fileHandle)
      fail("Unable to open " ~ playlist[index]);

    audioHandle = avc_getAVAudioStream(fileHandle, 0);
    if(!audioHandle)
      fail("Unable to load music track " ~ playlist[index]);

    int ch, bits, rate;
    if(avc_getAVAudioInfo(audioHandle, &rate, &ch, &bits) != 0)
      fail("Unable to get info for music track " ~ playlist[index]);

    bufRate = rate;
    bufFormat = 0;
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
      fail(format("Unhandled format (%d channels, %d bits) for music track %s", ch, bits, playlist[index]));

    foreach(int i, ref b; bIDs)
      {
        int length = avc_getAVAudioData(audioHandle, outData.ptr, outData.length);
        if(length) alBufferData(b, bufFormat, outData.ptr, length, bufRate);
        if(length == 0 || checkALError())
          {
            if(i == 0)
              fail("No audio data in music track " ~ playlist[index]);

            alDeleteBuffers(bIDs.length-i, bIDs.ptr+i);
            checkALError("running alDeleteBuffers");
            bIDs[i..$] = 0;
            break;
          }
      }

    alSourceQueueBuffers(sID, bIDs.length, bIDs.ptr);
    alSourcePlay(sID);
    if(checkALError())
      fail("Unable to start music track " ~ playlist[index]);

    index++;
    return;
  }

  // Start playing the jukebox
  void enableMusic()
  {
    if(!config.useMusic) return;

    musicOn = true;
    fading = Fade.None;
    playNext();
  }

  // Disable music
  void disableMusic()
  {
    if(fileHandle) avc_closeAVFile(fileHandle);
    fileHandle = null;
    audioHandle = null;

    if(sID)
      {
        alSourceStop(sID);
        alDeleteSources(1, &sID);
        checkALError("disabling music");
        sID = 0;
      }

    alDeleteBuffers(bIDs.length, bIDs.ptr);
    checkALError("deleting music buffers");
    bIDs[] = 0;

    musicOn = false;
  }

  // Pause current track
  void pauseMusic()
  {
    fading = Fade.Out;
  }

  // Resume. Can also be called in place of enableMusic for fading in.
  void resumeMusic()
  {
    if(!config.useMusic) return;

    volume = 0.0;
    fading = Fade.In;
    musicOn = true;
    if(sID) addTime(0);
    else playNext();
  }

  // Checks if a stream is playing, filling more data as needed, and restarting
  // if it stalled or was paused.
  private bool isPlaying()
  {
    if(!sID) return false;

    ALint count;
    alGetSourcei(sID, AL_BUFFERS_PROCESSED, &count);
    if(checkALError("in isPlaying()")) return false;

    for(int i = 0;i < count;i++)
      {
        int length = avc_getAVAudioData(audioHandle, outData.ptr, outData.length);
        if(length <= 0)
          {
            if(i == 0)
              {
                ALint state;
                alGetSourcei(sID, AL_SOURCE_STATE, &state);
                if(checkALError() || state == AL_STOPPED)
                  return false;
              }
            break;
          }

        ALuint bid;
        alSourceUnqueueBuffers(sID, 1, &bid);
        if(checkALError() == AL_NO_ERROR)
          {
            alBufferData(bid, bufFormat, outData.ptr, length, bufRate);
            alSourceQueueBuffers(sID, 1, &bid);
            checkALError();
          }
      }

    ALint state = AL_PLAYING;
    alGetSourcei(sID, AL_SOURCE_STATE, &state);
    if(state != AL_PLAYING) alSourcePlay(sID);
    return (checkALError() == AL_NO_ERROR);
  }

  // Check if the music has died. This function is also used for fading.
  void addTime(float time)
  {
    if(!musicOn) return;

    if(!isPlaying()) playNext();

    if(fading)
      {
	// Fade the volume
	if(fading == Fade.In)
	  {
	    volume += fadeInRate * time;
	    if(volume >= maxVolume)
	      {
		fading = Fade.None;
		volume = maxVolume;
	      }
	  }
	else
	  {
	    assert(fading == Fade.Out);
	    volume -= fadeOutRate * time;
	    if(volume <= 0.0)
	      {
		fading = Fade.None;
		volume = 0.0;

                // We are done fading out, disable music. Don't call
                // enableMusic (or isPlaying) unless you want it to start
                // again.
                if(sID) alSourcePause(sID);
		musicOn = false;
	      }
	  }

	// Set the new volume
        if(sID) alSourcef(sID, AL_GAIN, volume);
      }
  }
}
