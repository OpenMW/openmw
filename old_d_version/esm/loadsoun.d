/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadsoun.d) is part of the OpenMW package.

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

module esm.loadsoun;

import esm.imports;

/*
 * Sounds
 */

align(1) struct SOUNstruct
{
  ubyte volume, minRange, maxRange;
}
static assert(SOUNstruct.sizeof == 3);

struct Sound
{
  LoadState state;
  char[] id;

  SOUNstruct data;

  SoundIndex sound;

  void load()
    {
      sound = esFile.getSound();
      esFile.readHNExact(&data, data.sizeof, "DATA");
    }
}

ListID!(Sound) sounds;
