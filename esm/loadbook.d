/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadbook.d) is part of the OpenMW package.

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

module esm.loadbook;
import esm.imports;

/*
 * Books
 */

struct Book
{
  align(1) struct BKDTstruct
  {
    float weight;
    int value, isScroll, skillID, enchant;

    static assert(BKDTstruct.sizeof == 20);
  }

  BKDTstruct data;

  MeshIndex model;
  IconIndex icon;
  Script *script;
  Enchantment *enchant;

  mixin LoadT;
  char[] text;

  LoadState state;

  void load()
    {with(esFile){
      model = getMesh();
      name = getHNString("FNAM");
      readHNExact(&data, data.sizeof, "BKDT");
      script = getHNOPtr!(Script)("SCRI", scripts);
      icon = getIcon();
      text = getHNOString("TEXT");
      enchant = getHNOPtr!(Enchantment)("ENAM", enchants);

      makeProto();

      proto.setInt("skillID", data.skillID);
      proto.setBool("isScroll", data.isScroll != 0);
    }}
}
ListID!(Book) books;
