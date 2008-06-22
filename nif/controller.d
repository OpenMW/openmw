/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (controller.d) is part of the OpenMW package.

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

module nif.controller;

import nif.record;

// Time controller
abstract class Controller : Record
{
  Controller next;
  ushort flags;
  float frequency, phase; // Don't know what these do.
  float timeStart, timeStop;
  Controlled target;

 override:
  void read()
    {with(nifFile){
      super.read();

      debug(verbose) writef("Next controller ");
      getIndex();

      flags = getUshort;

      frequency = getFloatIs(1);
      phase = getFloat;
      timeStart = getFloat;
      timeStop = getFloat;

      debug(verbose) writef("Target ");
      getIndex();

      debug(verbose)
	{
	  writefln("Flags: %x", flags);
	  writefln("Frequency: ", frequency);
	  writefln("Phase: ", phase);
	  writefln("Start time: ", timeStart);
	  writefln("Stop time: ", timeStop);
	}
    }}

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      next = lookup!(Controller)(list);
      target = lookup!(Controlled)(list);
    }
}

alias NiBSPArrayController NiParticleSystemController;

class NiBSPArrayController : Controller
{
 override:
  void read()
    {with(nifFile){
      super.read();

      // At the moment, just skip it all

      //*
      // 23 floats = 92 bytes
      // 3 ints    = 12 bytes
      // 3 shorts  = 6 bytes
      // 1 byte    = 1 byte
      // Total:      111 bytes
      seekCur(111);

      short s = wgetShort;

      seekCur(15 + s*40);
      /*/
      float speed, speedRandom, angle, angleRandom;

      speed = wgetFloat;
      speedRandom = wgetFloat;
      angle = wgetFloat;
      angleRandom = wgetFloat;

      wgetFloat(); // Unknown
      wgetFloat(); // Sometimes pi

      for(int i; i<10; i++)
	wgetFloat();

      wgetByte();

      wgetFloat();
      wgetFloat();
      wgetFloat();

      wgetShort();

      wgetVector();

      debug(verbose) writef("Emitter: ");
      wgetInt();

      wgetShort();

      wgetFloat();

      wgetInt();
      wgetInt();

      wgetShort();      

      debug(verbose) writefln("begin particle group");
      short s = wgetShort;
      wgetShort();
      for(short i; i<s; i++)
	{
	  Matrix m;
	  getMatrix(m);
	  debug(verbose) writefln("Matrix: ", m.toString);
	  wgetShort();
	  wgetShort();
	}
      debug(verbose) writefln("end particle group");

      debug(verbose) writefln("Links:");
      wgetInt();
      wgetInt();
      wgetInt();
      wgetByte();
      //*/
    }}
}

class NiMaterialColorController : Controller
{
  NiPosData data;

 override:
  void read()
    {
      super.read();
      debug(verbose) writef("PosData ");
      getIndex();
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiPosData)(list);
    }
}

class NiPathController : Controller
{
  NiPosData posData;
  NiFloatData floatData;

 override:
  void read()
    {with(nifFile){
      super.read();

      wgetIntIs(1);
      wgetFloat();
      wgetFloat();
      wgetShortIs(0,1);
      debug(verbose) writef("Pos Data ");
      getIndex();
      debug(verbose) writef("Float Data ");
      getIndex();
    }}

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      posData = lookup!(NiPosData)(list);
      floatData = lookup!(NiFloatData)(list);
    }
}

class NiUVController : Controller
{
  NiUVData data;

 override:
  void read()
    {
      super.read();

      short s = nifFile.getShortIs(0);

      debug(verbose) writef("UV Data ");
      getIndex();
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiUVData)(list);
    }
}

//*
// If there was more than four of these, I would template it.
class NiKeyframeController : Controller
{
  NiKeyframeData data;

 override:
  void read()
    {
      super.read();

      debug(check)
	if(flags & 0xf0) nifFile.warn("Unknown flags");

      debug(verbose) writef("KeyframeData ");
      getIndex();
      // If this index is empty, timeStart/timeStop are set to +/-
      // float.max.
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiKeyframeData)(list);
    }
}

class NiAlphaController : Controller
{
  NiFloatData data;

 override:
  void read()
    {
      super.read();

      debug(controllerCheck)
	if(flags != 12) nifFile.warn("Unknown flags");

      debug(verbose) writef("Float Data ");
      getIndex();
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiFloatData)(list);
    }
}

class NiGeomMorpherController : Controller
{
  NiMorphData data;

 override:
  void read()
    {
      super.read();
      debug(controllerCheck)
	if(flags != 12) nifFile.warn("Unknown flags");

      debug(verbose) writef("Morph Data ");
      getIndex();

      byte b = nifFile.wgetByteIs(0);
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiMorphData)(list);
    }
}

class NiVisController : Controller
{
  NiVisData data;

 override:
  void read()
    {
      super.read();
      debug(controllerCheck)
	if(flags != 12) nifFile.warn("Unknown flags");

      debug(verbose) writef("Vis Data ");
      getIndex();
    }

  void sortOut(Record[] list)
    {
      super.sortOut(list);
      data = lookup!(NiVisData)(list);
    }
}

