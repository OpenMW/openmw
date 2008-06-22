/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (extra.d) is part of the OpenMW package.

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

module nif.extra;
import nif.record;
import nif.controlled;

abstract class Extra : Record
{
  Extra extra;

 override:
  void read()
    {
      super.read();
      debug(verbose) nifFile.writef("Extra Data ");
      getIndex();
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      extra = lookup!(Extra)(list);
    }
}

class NiVertWeightsExtraData : Extra
{
  override:
  void read()
    {
      super.read();

      int i = nifFile.getInt;
      short s = nifFile.getShort; // = number of vertices

      if(s*4+2 != i)
	nifFile.warn("Sizes don't add up");

      debug(verbose)
	{
	  writefln("Total bytes in record: ", i);
	  writefln("Number of vertex weights: ", s);
	}

      for(int j; j<s; j++)
	{
	  float ff = nifFile.getFloat;
	  debug(verbose) writefln("Float: ", ff);
	}
    }
}

class NiTextKeyExtraData : Extra
{
  struct Key
  {
    float time;
    char[] string;
  }

  Key[] keys;

  override void read()
    {
      super.read();

      nifFile.getIntIs(0);

      int keynum = nifFile.getInt;

      nifFile.fitArray(keynum,8);

      keys = nifRegion.allocateT!(Key)(keynum);
      foreach(int i, ref Key k; keys)
	{
	  k.time = nifFile.getFloat;
	  k.string = nifFile.getString;

	  debug(verbose)
	    writefln("  %d: %s @ %f ", i, k.string.clean().chomp(), k.time);
	}
    }
}

class NiStringExtraData : Extra
{
  char[] string;

  override void read()
    {
      super.read();

      int size = nifFile.getInt;
      string = nifFile.getString;

      if(size != string.length + 4)
	nifFile.warn("String size was incorrect.");

      debug(verbose)
	{
	  // "NCO" means 'no collision', I think
	  writefln("String: %s", string.clean());
	}
    }
}

class NiParticleGrowFade : Controlled
{
  //float f1, f2;
 override:
  void read()
    {
      super.read();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
    }
}

class NiParticleColorModifier : Controlled
{
  NiColorData data;

 override:
  void read()
    {
      super.read();
      debug(verbose) writef("Color Data ");
      getIndex();
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      lookup!(NiColorData)(list);
    }
}

class NiGravity : Controlled
{
 override:
  void read()
    {
      super.read();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetInt();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
    }
}

class NiPlanarCollider : Controlled
{
 override:
  void read()
    {
      super.read();
      
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      
      nifFile.wgetVector();
      nifFile.wgetVector();
      nifFile.wgetVector();
      nifFile.wgetVector();
    }
}

class NiParticleRotation : Controlled
{
 override:
  void read()
    {
      super.read();

      byte b = nifFile.wgetByteIs(0,1);

      nifFile.wgetFloatIs(1);
      nifFile.wgetFloat();
      nifFile.wgetFloat();
      nifFile.wgetFloat();
    }
}
