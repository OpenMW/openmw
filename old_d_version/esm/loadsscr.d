/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadsscr.d) is part of the OpenMW package.

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

module esm.loadsscr;
import esm.imports;

/*
 * Startup script. I think this is simply a 'main' script that is run
 * from the begining. The SSCR records contain a DATA identifier which
 * is totally useless, and a NAME which is simply a script reference.
 */

struct StartScriptList
{
  RegionBuffer!(Script*) list;

  void init()
  {
    // Set up the list
    list = esmRegion.getBuffer!(Script*)(0,1);
  }

  // Load a record and add it to the list
  void load()
    {with(esFile){
      getSubNameIs("DATA");
      skipHSub();
      list ~= getHNPtr!(Script)("NAME", scripts);
    }}

  uint length()
  { return list.length; }
}

StartScriptList startScripts;
