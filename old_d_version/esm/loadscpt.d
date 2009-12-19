/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadscpt.d) is part of the OpenMW package.

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

module esm.loadscpt;

import esm.imports;

//private import std.string;

/*
 * Script
 */

struct Script
{
  char[] id;
  LoadState state;

  uint numShorts, numLongs, numFloats;
  char[][] localVars;
  ubyte[] scriptData;
  char[] scriptText;

  uint scriptDataSize, localVarSize;

  void load()
    {with(esFile){
      /* Assume start of the header has already been read
      getSubNameIs("SCHD");
      getSubHeaderIs(52);
      id = getString(32);
      */

      getUint(numShorts);
      getUint(numLongs);
      getUint(numFloats);
      getUint(scriptDataSize);
      getUint(localVarSize);

      /* // In save games this is all that follows the header
      if(getFileType == FileType.Ess)
	{
	  getSubNameIs("SLCS");
	  skipHSubSize(12);

	  if(isNextSub("SLSD"))
	    skipHSub();

	  if(isNextSub("SLFD"))
	    skipHSub();

	  if(isNextSub("RNAM"))
	    skipHSubSize(4);

	  return;
	}//*/

      // List of local variables
      if(isNextSub("SCVR"))
	{
	  char[] tmp = getRegion().getString(localVarSize);

	  readHExact(tmp.ptr, tmp.length);

	  // At this point we can't use GC allocations at all, since
	  // our references are not in a root area. Thus the data
	  // could be collected while still in use.
	  localVars = getRegion().allocateT!(char[])
	    ( numShorts + numLongs + numFloats );
	  
	  // The tmp buffer is a null-byte separated string list, we
	  // just have to pick out one string at a time.
	  foreach(ref char[] result; localVars)
	    {
	      result = stripz(tmp);
	      tmp = tmp[result.length+1..$];
	    }

	  if(tmp.length) fail("Variable table size mismatch");
	}
      else localVars = null;

      // Script data
      scriptData = getRegion().allocate(scriptDataSize);
      readHNExact(scriptData.ptr, scriptData.length, "SCDT");

      // Script text
      scriptText = getHNOString("SCTX");
    }}
}

class ScriptList : ListID!(Script)
{
  this(uint s) { super(s); }

  override char[] getID()
    {
      // Script header
      esFile.getSubNameIs("SCHD");
      esFile.getSubHeaderIs(52);

      char[] id = esFile.getString(32);
      // TODO: Handle multiple Main scripts here. With tribunal,
      // modders got the ability to add 'start scripts' to their mods,
      // which is a script that is run at startup (I think.) Before
      // that, there was only one startup script, called
      // "Main". Although most mods have switched to using startup
      // scripts, some legacy mods might still overwrite Main, and
      // this can cause problems if several mods do it. I think the
      // best course of action is to NEVER overwrite main, but instead
      // add each with a separate unique name and add them to the
      // start script list.
      if(esFile.getSpecial() != SpecialFile.Morrowind && icmp(id,"Main")==0)
	writefln("Found MAIN script in %s ", esFile.getFilename());

      return id;
    }
}

ScriptList scripts;
