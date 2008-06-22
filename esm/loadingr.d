/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadingr.d) is part of the OpenMW package.

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

module esm.loadingr;
import esm.imports;

/*
 * Alchemy ingredient
 */

struct Ingredient
{
  align(1) struct IRDTstruct
  {
    float weight;
    int value;
    int[4] effectID;         // Effect, 0 or -1 means none
    SkillEnum[4] skills;     // Skill related to effect
    Attribute[4] attributes; // Attribute related to effect

    static assert(IRDTstruct.sizeof==56);
  }

  IRDTstruct data;

  LoadState state;

  char[] id, name;
  MeshIndex model;
  IconIndex icon;
  Script *script;

  void load()
  {with(esFile){
    model = getMesh();
    name = getHNString("FNAM");
    readHNExact(&data, data.sizeof, "IRDT");

    script = getHNOPtr!(Script)("SCRI", scripts);
    icon = getOIcon();
  }}
}
ListID!(Ingredient) ingreds;
