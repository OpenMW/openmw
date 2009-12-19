/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadsndg.d) is part of the OpenMW package.

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

module esm.loadsndg;
import esm.imports;

import esm.loadcrea;

/*
 * Sound generator. This describes the sounds a creature make.
 */

struct SoundGenerator
{
  enum Type
    {
      LeftFoot	= 0,
      RightFoot = 1,
      SwimLeft	= 2,
      SwimRight = 3,
      Moan	= 4,
      Roar	= 5,
      Scream	= 6,
      Land	= 7
    }

  // Type 
  Type type;

  char[] id;
  LoadState state;

  // Which creature this applies to, if any.
  Creature *creature;

  Sound *sound;

  void load()
  {with(esFile){
    this.type = cast(Type)getHNInt("DATA");

    creature = getHNOPtr!(Creature)("CNAM", creatures);
    sound = getHNPtr!(Sound)("SNAM", sounds);
  }}
}
ListID!(SoundGenerator) soundGens;
