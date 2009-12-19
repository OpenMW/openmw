/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadarmo.d) is part of the OpenMW package.

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

module esm.loadarmo;
import esm.imports;

import esm.loadbody, esm.loadench;

/*
 * Armor
 */

// Reference to body parts
struct PartReference
{
  enum Index
    {
      Head		= 0,
      Hair		= 1,
      Neck		= 2,
      Cuirass		= 3,
      Groin		= 4,
      Skirt		= 5,
      RHand		= 6,
      LHand		= 7,
      RWrist		= 8,
      LWrist		= 9,
      Shield		= 10,
      RForearm		= 11,
      LForearm		= 12,
      RUpperarm		= 13,
      LUpperarm		= 14,
      RFoot		= 15,
      LFoot		= 16,
      RAnkle		= 17,
      LAnkle		= 18,
      RKnee		= 19,
      LKnee		= 20,
      RLeg		= 21,
      LLeg		= 22,
      RPauldron		= 23,
      LPauldron		= 24,
      Weapon		= 25,
      Tail		= 26	
    }
  Index part;

  BodyPart* male, female;
}

struct PartReferenceList
{
  RegionBuffer!(PartReference) parts;

  void load()
  {with(esFile){
    parts = getRegion().getBuffer!(PartReference)(0,1);
    while(isNextSub("INDX"))
      {
	parts.length = parts.length + 1;
	with(parts.array[$-1])
	  {
	    part = cast(PartReference.Index) getHByte;
	    male = getHNOPtr!(BodyPart)("BNAM", bodyParts);
	    female = getHNOPtr!(BodyPart)("CNAM", bodyParts);
	  }
      }
  }}
}

struct Armor
{
  enum Type : int
  {
    Helmet		= 0,
    Cuirass		= 1,
    LPauldron		= 2,
    RPauldron		= 3,
    Greaves		= 4,
    Boots		= 5,
    LGauntlet		= 6,
    RGauntlet		= 7,
    Shield		= 8,
    LBracer		= 9,
    RBracer		= 10
  }

  align(1) struct AODTstruct
  {
    Type type;
    float weight;
    int value, health, enchant, armor;

    static assert(AODTstruct.sizeof==24);
  }

  AODTstruct data;

  mixin LoadT!();

  PartReferenceList parts;

  MeshIndex model;
  IconIndex icon;

  Script *script;
  Enchantment *enchant;

  void load()
  {with(esFile){
    model = getMesh();
    name = getHNString("FNAM");
    script = getHNOPtr!(Script)("SCRI", scripts);
    readHNExact(&data, data.sizeof, "AODT");
    icon = getOIcon();

    parts.load();

    enchant = getHNOPtr!(Enchantment)("ENAM", enchants);

    makeProto();

    proto.setInt("type", data.type);
    proto.setInt("armor", data.armor);

    proto.setInt("health", data.health);
  }}
}
ListID!(Armor) armors;
