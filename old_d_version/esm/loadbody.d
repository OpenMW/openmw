/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadbody.d) is part of the OpenMW package.

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

module esm.loadbody;
import esm.imports;

/*
 * Body part mesh. NPCs have several mesh files, one for each element
 * in the enum below. One BODY record is given for each part.
 */

struct BodyPart
{
  enum MeshType : byte
    {
      Head	= 0,
      Hair	= 1,
      Neck	= 2,
      Chest	= 3,
      Groin	= 4,
      Hand	= 5,
      Wrist	= 6,
      Forearm	= 7,
      Upperarm	= 8,
      Foot	= 9,
      Ankle	= 10,
      Knee	= 11,
      Upperleg	= 12,
      Clavicle	= 13,
      Tail	= 14
    }

  enum Type : byte
    {
      Skin	= 0,
      Clothing	= 1,
      Armor	= 2
    }

  enum Flags : byte
    {
      Female	= 1,
      Playable  = 2
    }

  align(1) struct BYDTstruct
  {
    MeshType part;
    byte vampire;
    Flags flags;
    Type type;
    static assert(BYDTstruct.sizeof==4);
  }

  BYDTstruct data;

  mixin LoadT;

  MeshIndex model;

  void load()
    {with(esFile){
      model = getMesh();
      name = getHNString("FNAM");
      readHNExact(&data, data.sizeof, "BYDT");

      // don't need to run makeProto here yet, no BodyPart monster
      // class.
    }}

}

ListID!(BodyPart) bodyParts;
