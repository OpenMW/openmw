/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadltex.d) is part of the OpenMW package.

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

module esm.loadltex;
import esm.imports;

/*
 * Texture used for texturing landscape?
 *
 * Not sure how to store these yet. They are probably indexed by
 * 'num', not 'id', but I don't know for sure. And num is not unique
 * between files, so one option is to keep a separate list for each
 * input file (that has LTEX records, of course.) We also need to
 * resolve references to already existing land textures to save space.

 * I'm not sure if it is even possible to override existing land
 * textures, probably not. I'll have to try it, and have to mimic the
 * behaviour of morrowind. First, check what you are allowed to do in
 * the editor. Then make an esp which changes a commonly used land
 * texture, and see if it affects the game.
 */

class LandTextureList : ListKeeper
{
  // Number of textures inserted in this file.
  int num;

  alias RegionBuffer!(TextureIndex) TextureList;

  // Contains the list of land textures for each file, indexed by
  // file. TODO: Use some handle system here too instead of raw
  // filename?
  HashTable!(char[], TextureList, ESMRegionAlloc, CITextHash) files;

  // The texture list for the current file
  TextureList current;

  this()
    {
      num = 0;
      endFile();

      // The first file (Morrowind.esm) typically needs a little more
      // than most others
      current = esmRegion.getBuffer!(TextureIndex)(0,120);
    }

  void load()
    {with(esFile){
      getHNString("NAME");

      int n = getHNInt("INTV");
      if(n != num++)
	{
	  //writefln("Warning: Wanted land texture %d, got %d", num-1, n);
	  current.length = n;
	  num = n+1;
	}

      current ~= resources.lookupTexture(getHNString("DATA"));
    }}

  void endFile()
    {
      if(num)
	{
	  files[esFile.getFilename()] = current;
	  current = esmRegion.getBuffer!(TextureIndex)(0,50);
	  num = 0;
	}
    }

  uint length() { return files.length; }

  void* lookup(char[] s) { assert(0); }

  // This requires the correct file to be open
  TextureIndex lookup(int index)
    {
      return files[esFile.getFilename()][index];
    }
}
LandTextureList landTextures;
