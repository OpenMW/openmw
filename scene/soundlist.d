/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (soundlist.d) is part of the OpenMW package.

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

module scene.soundlist;

import esm.loadsoun;
import sound.audio;

SoundList soundScene;

// Has a list over all sounds currently playing in the
// scene. Currently only holds static, looping sounds, mainly
// torches. Later I will have to decide what to do with play-once
// sounds. Do I bother to update their position, or do I assume that
// once it starts playing, it is short enough that it doesn't matter?
// The last option is probably not viable, since some sounds can be
// very long.
struct SoundList
{
  // TODO: This is really just a test, a hack. Will be replaced by a
  // list or similar later.
  SoundInstance list[50];
  int index = 0;

  // Get a sound instance from a Sound struct
  static SoundInstance getInstance(Sound *s, bool loop=false)
  {
    const distFactor = 40.0; // Just guessing, really.

    SoundInstance inst = s.sound.getInstance();
    inst.setParams(s.data.volume/255.0,
		   s.data.minRange*distFactor,
		   s.data.maxRange*distFactor,
		   loop);
    return inst;
  }

  SoundInstance *insert(Sound *snd, bool loop=false)
  {
    if(index == 50) return null;

    SoundInstance *s = &list[index++];
    *s = getInstance(snd, loop);
    return s;
  }

  void update(float x, float y, float z)
  {
    foreach(ref s; list[0..index])
      s.setPlayerPos(x,y,z);
  }
}
