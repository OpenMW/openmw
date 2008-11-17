/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadacti.d) is part of the OpenMW package.

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

module esm.loadacti;

import esm.imports;

/*
 * Activator
 *
 * Activators are static in-world objects that pop up a caption when
 * you stand close and look at them, for example signs. Some
 * activators have scrips, such as beds that let you sleep when you
 * activate them.
 */

struct Activator
{
  mixin LoadT!();

  Script *script;
  MeshIndex model;

  void load()
  {with(esFile){
    model = getMesh();
    name = getHNString("FNAM");
    script = getHNOPtr!(Script)("SCRI", scripts);

    makeProto();
  }}
}
ListID!(Activator) activators;
