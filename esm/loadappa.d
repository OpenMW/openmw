/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadappa.d) is part of the OpenMW package.

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

module esm.loadappa;
import esm.imports;

/*
 * Alchemist apparatus
 */

struct Apparatus
{
  enum AppaType : int
    {
      MortarPestle	= 0,
      Albemic		= 1,
      Calcinator	= 2,
      Retort		= 3
    }

  align(1) struct AADTstruct
  {
    AppaType type;
    float quality;
    float weight;
    int value;

    static assert(AADTstruct.sizeof == 16);
  }
  mixin LoadT!();

  AADTstruct data;
  MeshIndex model;
  IconIndex icon;
  Script *script;

  void load()
    {with(esFile){
      model = getMesh();
      name = getHNString("FNAM");
      readHNExact(&data, data.sizeof, "AADT");
      script = getHNOPtr!(Script)("SCRI", scripts);
      icon = getIcon();

      makeProto();

      proto.setFloat("quality", data.quality);
      proto.setInt("type", data.type);
    }}
}
ListID!(Apparatus) appas;
