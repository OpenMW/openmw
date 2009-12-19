/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadcrec.d) is part of the OpenMW package.

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

module esm.loadcrec;
import esm.imports;

/*
 * Creature and container change information? Used in save games only.
 */

struct LoadCREC
{
  void load(TES3File f)
    {
      writefln("Creature-changer %s", f.getHNString("NAME"));
      writefln("  Index: ", f.getHNInt("INDX"));

      while(f.hasMoreSubs)
	{
	  f.getSubName();
	  f.skipHSub();
	}
    }
}

struct LoadCNTC
{
  void load(TES3File f)
    {
      writefln("Itemlist-changer %s", f.getHNString("NAME"));
      writefln("  Index: ", f.getHNInt("INDX"));

      while(f.hasMoreSubs)
	{
	  f.getSubName();
	  f.skipHSub();
	}
    }
}
