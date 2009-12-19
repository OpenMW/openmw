/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadench.d) is part of the OpenMW package.

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

module esm.loadench;
import esm.imports;

/*
 * Enchantments
 */

struct Enchantment
{
  enum Type : int
    {
      CastOnce		= 0,
      WhenStrikes	= 1,
      WhenUsed		= 2,
      ConstantEffect	= 3
    }

  align(1) struct ENDTstruct
  {
    Type type;
    int cost;
    int charge;
    int autocalc; // Guessing this is 1 if we are supposed to auto
		  // calculate

    static assert(ENDTstruct.sizeof == 16);
  }
  ENDTstruct data;

  EffectList effects;

  char[] id;
  LoadState state;

  void load()
    {with(esFile){
      readHNExact(&data, data.sizeof, "ENDT");

      effects = getRegion().getBuffer!(ENAMstruct)(0,1);
      while(isNextSub("ENAM"))
	{
	  effects.length = effects.length + 1;
	  readHExact(&effects.array[$-1], effects.array[$-1].sizeof);
	}
    }}
}
ListID!(Enchantment) enchants;
