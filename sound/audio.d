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

import sound.audiere;

class SoundException : Exception
{
  this(char[] caller, char[] msg) { super(caller ~ " SoundException: " ~ msg); }
}

MusicManager jukebox;
MusicManager battleMusic;

void initializeSound()
{
  if(cpp_openDevice())
    throw new SoundException("initializeSound()",
			     "Failed to initialize music device");
  jukebox.initialize("Main");
  battleMusic.initialize("Battle");
}
