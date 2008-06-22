/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadalch.d) is part of the OpenMW package.

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

module esm.loadalch;
import esm.imports;

/*
 * Alchemy item (potions)
 */

struct Potion
{
  char[] id;
  LoadState state;

  align(1) struct ALDTstruct
  {
    float weight;
    int value;
    int autoCalc;
    static assert(ALDTstruct.sizeof == 12);
  }

  ALDTstruct data;

  char[] name;

  MeshIndex model;
  IconIndex icon;

  Script *script;

  EffectList effects;

  void load()
    {with(esFile){
      model = getMesh();
      icon = getIcon("TEXT");
      script = getHNOPtr!(Script)("SCRI", scripts);
      name = getHNOString("FNAM");

      readHNExact(&data, data.sizeof, "ALDT");

      effects = getRegion().getBuffer!(ENAMstruct)(0,1);

      while(isNextSub("ENAM"))
	{
	  effects.length = effects.length + 1;
	  readHExact(&effects.array[$-1], effects.array[$-1].sizeof);
	}

    }}
}
ListID!(Potion) potions;
