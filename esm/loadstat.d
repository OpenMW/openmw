/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadstat.d) is part of the OpenMW package.

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

module esm.loadstat;

import esm.imports;

/*
 * Definition of static object.
 *
 * A stat record is basically just a reference to a nif file. Some
 * esps seem to contain copies of the STAT entries from the esms, and
 * the esms themselves contain several identical entries. Perhaps all
 * statics referenced in a file is also put in the file? Since we are
 * only reading files it doesn't much matter to us, but it would if we
 * were writing. You can check some files later, when you decode the
 * CELL blocks, if you want to test this hypothesis.
 */

struct Static
{
  char[] id;
  LoadState state;

  MeshIndex model;

  void load()
  {
    model = esFile.getMesh();
  }
}

ListID!(Static) statics;
