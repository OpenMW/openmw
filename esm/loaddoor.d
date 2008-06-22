/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loaddoor.d) is part of the OpenMW package.

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

module esm.loaddoor;
import esm.imports;

/*
 * Doors
 */

struct Door
{
  LoadState state;
  char[] id, name;

  MeshIndex model;
  Script *script;
  Sound* openSound, closeSound;

  void load()
  {with(esFile){
    model = getMesh();
    name = getHNOString("FNAM");
    script = getHNOPtr!(Script)("SCRI", scripts);
    openSound = getHNOPtr!(Sound)("SNAM", sounds);
    closeSound = getHNOPtr!(Sound)("ANAM", sounds);
  }}
}
ListID!(Door) doors; // Break on through
