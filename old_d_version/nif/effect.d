/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (effect.d) is part of the OpenMW package.

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

module nif.effect;
import nif.record;

class Effect : Node {}

// Used for NiAmbientLight and NiDirectionalLight. Might also work for
// NiPointLight and NiSpotLight?
class Light : Effect
{
  float dimmer;
  Vector ambient, diffuse, specular;

 override:
  void read()
    {
      super.read();

      debug(check)
	if(flags != 2 && flags != 4) nifFile.warn("Unknown flags");

      nifFile.getIntIs(1);
      if(nifFile.getInt == 0) nifFile.warn("Light list might be missing");

      dimmer = nifFile.getFloat;
      ambient = nifFile.getVector;
      diffuse = nifFile.getVector;
      specular = nifFile.getVector;

      debug(verbose)
	{
	  writefln("Dimmer: ", dimmer);
	  writefln("Ambient: ", ambient.toString);
	  writefln("Diffuse: ", diffuse.toString);
	  writefln("Specular: ", specular.toString);
	}
    }
}

class NiTextureEffect : Effect
{
  NiSourceTexture texture;

 override:
  void read()
    {
      super.read();
      
      debug(check)
	if(flags != 2 && flags != 4) nifFile.warn("Unknown flags");

      int i = nifFile.wgetInt; // 0 or 1

      if(i == 1) {if(nifFile.getInt == 0) nifFile.warn("List might be missing");}
      else if(i != 0) nifFile.warn("Unknown value");

      for(int j; j<3; j++)
	{
	  nifFile.wgetFloatIs(1);
	  nifFile.wgetFloatIs(0);
	  nifFile.wgetFloatIs(0);
	  nifFile.wgetFloatIs(0);
	}

      nifFile.wgetIntIs(2);
      nifFile.wgetIntIs(0,3);
      nifFile.wgetIntIs(2);
      nifFile.wgetIntIs(2);
      debug(verbose) writef("Source Texture ");
      getIndex();
      nifFile.wgetByteIs(0);

      nifFile.wgetFloatIs(1);
      nifFile.wgetFloatIs(0);
      nifFile.wgetFloatIs(0);
      nifFile.wgetFloatIs(0);

      nifFile.wgetShortIs(0);
      nifFile.wgetShortIs(-75);
      nifFile.wgetShortIs(0);
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      texture = lookup!(NiSourceTexture)(list);
    }
}
