/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadbsgn.d) is part of the OpenMW package.

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

module esm.loadbsgn;
import esm.imports;

/*
 * Birth signs
 */

struct BirthSign
{
  LoadState state;

  char[] id, name, description;

  TextureIndex texture;

  // List of powers and abilities that come with this birth sign.
  SpellList powers;

  void load()
  {with(esFile){
    name = getHNString("FNAM");

    texture = getOTexture();

    description = getHNOString("DESC");

    powers.load();
  }}
}
ListID!(BirthSign) birthSigns;
