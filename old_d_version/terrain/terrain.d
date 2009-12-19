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

import terrain.generator;
import terrain.archive;
import terrain.bindings;
import terrain.quad;
import std.file, std.stdio;

char[] cacheDir = "cache/terrain/";

// Enable this to render single terrain meshes instead of the entire
// data set
//debug=singleMesh;

void initTerrain(bool doGen)
{
  char[] fname = cacheDir ~ "landscape.cache";

  if(!exists(fname))
    {
      writefln("Cache file '%s' not found. Creating:",
               fname);
      doGen = true;
    }

  if(doGen)
    generate(fname);

  // Load the archive file
  g_archive.openFile(fname);

  terr_setupRendering();

  debug(singleMesh)
    {
      int X = 22;
      int Y = 0;
      bool next = false;

      void doQuad(int x, int y, int lev, int diffx, int diffy)
        {
          if(!g_archive.hasQuad(x,y,lev))
            return;

          diffx *= 8192;
          diffy *= 8192;

          auto node = terr_createChildNode(20000+diffx,-60000+diffy,null);
          auto info = g_archive.getQuad(x,y,lev);
          g_archive.mapQuad(info);
          auto mi = g_archive.getMeshInfo(0);
          terr_makeMesh(node, mi, info.level, TEX_SCALE);
        }

      doQuad(X,Y,1, 0,0);
      doQuad(X+1,Y,1, 1,0);
      doQuad(X,Y+1,1, 0,1);
      doQuad(X+1,Y+1,1, 1,1);

      doQuad(X + (next?2:0),Y,2, 2,0);

      doQuad(20,Y,3, 0,2);
    }
  else
    {
      // Create the root quad
      rootQuad = new Quad;
    }
}

extern(C) void d_terr_terrainUpdate()
{
  debug(singleMesh) return;

  // Update the root quad each frame.
  assert(rootQuad !is null);
  rootQuad.update();
}

Quad rootQuad;
