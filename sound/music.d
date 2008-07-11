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

import sound.audiere;
import sound.audio;

import std.string;

import core.config;
import core.resource;

// Simple music player, has a playlist and can pause/resume music.
struct MusicManager
{
  private:

  // How often we check if the music has died.
  const float pollInterval = 2.0;

  // Max file size to load. Files larger than this are streamed.
  const int loadSize = 1024*1024;

  // How much to add to the volume each second when fading
  const float fadeInRate = 0.10;
  const float fadeOutRate = 0.35;

  // Volume
  float volume, maxVolume;

  // Time since last time we polled
  float sumTime;

  char[] name;

  void fail(char[] msg)
  {
    throw new SoundException(name ~ " Jukebox", msg);
  }

  // Used when randomizing playlist
  struct rndListStruct
  {
    char[] entry;
    bool used;
  }
  rndListStruct[] rndList;

  // TODO: How do we handle the play list? Randomize should be an
  // option. We could also support things like M3U files, etc.
  char[][] playlist;
  int index; // Index of next song to play

  bool musicOn;
  AudiereInstance music;

  // Which direction are we currently fading, if any
  enum Fade {  None = 0, In, Out  }
  Fade fading;

  public:

  // Initialize the jukebox
  void initialize(char[] name)
  {
    this.name = name;
    musicOn = false;
    updateVolume();
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
    cpp_setParams(music, volume, 0.0);
  }

  // Give a music play list
  void setPlaylist(char[][] pl)
  {
    playlist = pl;
    index = 0;

    randomize();
  }

  // Randomize playlist. An N^2 algorithm, but our current playlists
  // are small. Improve it later if you really have to. If the
  // argument is true, then we don't want the old last to be the new
  // first.
  private void randomize(bool checklast = false)
  {
    if(playlist.length < 2) return;

    char[] last = playlist[0];

    int left = playlist.length;
    rndList.length = left;

    foreach(int i, ref s; rndList)
      {
	s.used = false;
	s.entry = playlist[i];
      }

    while(left--)
      {
	int index = rnd.randInt(0,left);
	int i = 0;
	foreach(ref s; rndList)
	  {
	    // Skip the ones we have used already
	    if(s.used) continue;

	    // Is this the correct index?
	    if(i == index)
	      {
		s.used = true;
		playlist[left] = s.entry;
		break;
	      }
	    i++;
	  }	  
      }

    // Check that we don't replay the previous song, if the caller
    // requested this.
    if(checklast && playlist[0] == last)
      {
	playlist[0] = playlist[$-1];
	playlist[$-1] = last;
      }
  }

  // Skip to the next track
  void playNext()
  {
    // If music is disabled, do nothing
    if(!musicOn) return;

    // Kill current track
    if(music) cpp_destroyInstance(music);
    music = null;

    // No tracks to play?
    if(!playlist.length) return;

    // End of list? Randomize and start over
    if(index == playlist.length)
      {
	randomize(true);
	index = 0;
      }

    music = cpp_playStream(toStringz(playlist[index]), volume);

    if(!music) fail("Unable to start music track " ~ playlist[index]);

    index++;
  }

  // Start playing the jukebox
  void enableMusic()
  {
    if(!config.useMusic) return;

    sumTime = 0;
    musicOn = true;
    volume = maxVolume;
    fading = Fade.None;
    playNext();
  }

  // Disable music
  void disableMusic()
  {
    if(music) cpp_destroyInstance(music);
    music = null;
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

    sumTime = 0;
    volume = 0.0;
    fading = Fade.In;
    musicOn = true;
    if(music) cpp_playSound(music);
    else playNext();
  }

  // Add time since last frame to the counter. If the accumulated time
  // has passed the polling interval, then check if the music has
  // died. The Audiere library has a callback functionality, but it
  // turned out not to be terribly reliable. Sometimes it was never
  // called at all. So we have to poll at regular intervals .. :( This
  // function is also used for fading.
  void addTime(float time)
  {
    if(!musicOn) return;
    sumTime += time;
    if(sumTime > pollInterval)
      {
	sumTime = 0;
	if(!cpp_isPlaying(music)) playNext();
      }

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

		// We are done fading out, disable music.
		cpp_stopSound(music);
		musicOn = false;
	      }
	  }

	// Set the new volume
	cpp_setParams(music, volume, 0.0);
      }
  }
}
