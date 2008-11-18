/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadlocks.d) is part of the OpenMW package.

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

module esm.loadlocks;
import esm.imports;

/*
 * This file covers lockpicks, probes and armor repair items, since
 * these have nearly identical structure.
 */

struct Tool
{
  align(1) struct Data
  {
    float weight;
    int value;

    float quality; // And when I say nearly identical structure, I
    int uses;      // mean perfectly identical except that these two
                   // variables are swaped for repair items. Don't ask
                   // me why.
  }
  static assert(Data.sizeof == 16);

  Data data;

  mixin LoadT;

  MeshIndex model;
  IconIndex icon;
  Script* script;

  void load()
    {with(esFile){
      model = getMesh();
      name = getHNString("FNAM");

      char[] type;
      bool isRep = false;

      if(isNextSub("LKDT")) type = "Lockpick";
      else if(isNextSub("PBDT")) type = "Probe";
      else
        {
	  getSubNameIs("RIDT");
          type = "RepairItem";
          isRep = true;
        }

      readHExact(&data, data.sizeof);

      if(isRep)
        {
	  // Swap t.data.quality and t.data.uses (sigh)
	  float tmp = *(cast(float*)&data.uses);
	  data.uses = *(cast(int*)&data.quality);
	  data.quality = tmp;
	}

      script = getHNOPtr!(Script)("SCRI", scripts);
      icon = getOIcon();

      
      makeProto(type);
      proto.setFloat("quality", data.quality);
      proto.setInt("uses", data.uses);
    }}
}
ListID!(Tool) lockpicks, probes, repairs;
