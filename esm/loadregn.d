/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadregn.d) is part of the OpenMW package.

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

module esm.loadregn;

import esm.imports;

import esm.loadlevlist;

/*
 * Region data
 */

struct Region
{
  align(1) struct WEATstruct
  {
    byte clear, // Don't know if these are probabilities or what.
      cloudy,
      foggy,
      overcast,
      rain,
      thunder,
      ash,
      blight,
      a,b;// Unknown weather, probably snow and something. Only
	  // present in file version 1.3.
  }
  static assert(WEATstruct.sizeof==10);

  LoadState state;
  char[] id, name;
  WEATstruct data;
  Color mapColor;

  // Leveled list of creatures you can meet if you sleep outside in
  // this region.
  LeveledCreatures *sleepList;

  // Sounds that are played randomly when you are in this region
  struct SoundRef{ Sound* sound; ubyte chance; }

  RegionBuffer!(SoundRef) soundList;

  void load()
    {with(esFile){
      name = getHNString("FNAM");

      if(isVer12())
	readHNExact(&data, data.sizeof-2, "WEAT");
      else if(isVer13())
	readHNExact(&data, data.sizeof, "WEAT");
      else fail("Don't know what to do in this version");

      // TODO: Calculate weather probabilities here? Or sum them, or
      // whatever?

      sleepList = getHNOPtr!(LeveledCreatures)("BNAM", creatureLists);

      /*
      if(getFileType == FileType.Savegame)
	{
	  // Probably says which weather condition this region is
	  // currently experiencing.
	  writefln("WNAM: ", getHNInt("WNAM"));
	  return
	}
      */

      readHNExact(&mapColor, mapColor.sizeof, "CNAM");

      soundList = esmRegion.getBuffer!(SoundRef)(0,20);

      while(isNextSub("SNAM"))
	{
	  char[32] buffer;

	  getSubHeaderIs(33);

	  soundList.length = soundList.length + 1;
	  with(soundList.array[$-1])
	    {
	      // Get and chop sound name
	      sound = cast(Sound*) sounds.lookup(getString(buffer));

	      // Get sound probability
	      getUByte(chance);
	    }
	}
    }}
}

ListID!(Region) regions;
