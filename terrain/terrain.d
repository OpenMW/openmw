/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2009  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (terrain.d) is part of the OpenMW package.

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

module terrain.terrain;

import std.stdio;
import std.file;
import monster.util.string;

void fail(char[] msg)
{
  throw new Exception(msg);
}

// Move elsewhere, make part of the general cache system later
void makeDir(char[] pt)
{
  if(exists(pt))
    {
      if(!isdir(pt))
        fail(pt ~ " is not a directory");
    }
  else
    mkdir(pt);
}

void makePath(char[] pt)
{
  assert(!pt.begins("/"));
  foreach(int i, char c; pt)
    if(c == '/')
      makeDir(pt[0..i]);

  if(!pt.ends("/"))
    makeDir(pt);
}

void initTerrain()
{
  makePath("cache/terrain");

  //terr_genData();
  terr_setupRendering();
}

extern(C):
void terr_genData();
void terr_setupRendering();
