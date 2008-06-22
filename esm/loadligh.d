/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadligh.d) is part of the OpenMW package.

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
module esm.loadligh;
import esm.imports;

/*
 * Lights
 */

struct Light
{
  char[] id;
  LoadState state;

  enum Flags : uint
    {
      Dynamic		= 0x001,
      Carry		= 0x002, // Can be carried
      Negative		= 0x004, // Negative light?
      Flicker		= 0x008,
      Fire		= 0x010,
      OffDefault	= 0x020, // Off by default
      FlickerSlow	= 0x040,
      Pulse		= 0x080,
      PulseSlow		= 0x100
    }

  align(1) struct LHDTstruct
  {
    float weight;
    int value;
    int time; // Duration
    int radius;
    Color color;
    Flags flags;

    static assert(LHDTstruct.sizeof == 24);
  }

  LHDTstruct data;

  char[] name;

  MeshIndex model;
  IconIndex icon;

  Sound* sound;
  Script* script;

  void load()
    {with(esFile){
      model = getMesh();
      name = getHNOString("FNAM");
      icon = getOIcon();

      readHNExact(&data, data.sizeof, "LHDT");

      script = getHNOPtr!(Script)("SCRI", scripts);
      sound = getHNOPtr!(Sound)("SNAM", sounds);
    }}
}
ListID!(Light) lights;
