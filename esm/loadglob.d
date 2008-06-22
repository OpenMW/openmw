/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadglob.d) is part of the OpenMW package.

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

module esm.loadglob;
import esm.imports;

/*
 * Global script variables
 */

struct Global
{
  LoadState state;
  char[] id;

  float value;

  VarType type;

  void load()
  {
    VarType t;
    char[] tmp = esFile.getHNString("FNAM");
    switch(tmp)
      {
      case "s": t = VarType.Short; break;
      case "l": t = VarType.Int; break;
      case "f": t = VarType.Float; break;
      default:
	esFile.fail("Illegal global variable type " ~ tmp);
      }
    if(state == LoadState.Previous && t != type)
      esFile.fail(format("Variable changed type from %d to %d",
		  cast(int)type, cast(int)t));

    type = t;

    value = esFile.getHNFloat("FLTV");
  }
}
ListID!(Global) globals;
