/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadclot.d) is part of the OpenMW package.

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

module esm.loadclot;
import esm.imports;
import esm.loadarmo;

/*
 * Clothing
 */

struct Clothing
{
  enum Type : int
    {
      Pants	= 0,
      Shoes	= 1,
      Shirt	= 2,
      Belt	= 3,
      Robe	= 4,
      RGlove	= 5,
      LGlove	= 6,
      Skirt	= 7,
      Ring	= 8,
      Amulet	= 9
    }

  struct CTDTstruct
  {
    Type type;
    float weight;
    short value;
    short enchantPoints;

    static assert(CTDTstruct.sizeof == 12);
  }

  CTDTstruct data;

  LoadState state;
  char[] id, name;
  PartReferenceList parts;

  MeshIndex model;
  IconIndex icon;
  Enchantment *enchant;
  Script *script;

  void load()
  {with(esFile){
    model = getMesh();
    name = getHNOString("FNAM");
    readHNExact(&data, data.sizeof, "CTDT");

    script = getHNOPtr!(Script)("SCRI", scripts);
    icon = getOIcon();

    parts.load();

    enchant = getHNOPtr!(Enchantment)("ENAM", enchants);
  }}
}
ListID!(Clothing) clothes;
