/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadmisc.d) is part of the OpenMW package.

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

module esm.loadmisc;
import esm.imports;

/*
 * Misc inventory items, basically things that have no use but can be
 * carried, bought and sold. It also includes keys.
 */

struct Misc
{
  struct MCDTstruct
  {
    float weight;
    int value;
    int isKey; // There are many keys in Morrowind.esm that has this
	       // set to 0. TODO: Check what this field corresponds to
	       // in the editor.

    static assert(MCDTstruct.sizeof == 12);
  }
  MCDTstruct data;

  mixin LoadT;

  MeshIndex model;
  IconIndex icon;
  Script *script;

  void load()
  {with(esFile){
    model = getMesh();
    name = getHNOString("FNAM");
    readHNExact(&data, data.sizeof, "MCDT");
    script = getHNOPtr!(Script)("SCRI", scripts);
    icon = getOIcon();

    makeProto();
    proto.setInt("isKey", data.isKey);
  }}
}
ListID!(Misc) miscItems;
