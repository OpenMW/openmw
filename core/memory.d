/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (memory.d) is part of the OpenMW package.

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

module core.memory;

import util.regions;

// Global memory managers used by various loading routines
RegionManager
  esmRegion,	// Memory used by ESMs, BSAs and plugins. Cleared only
		// when all files are reloaded. Lifetime is current
		// plugin setting session - if we change the set of
		// plugins, this region is reset and everything is
		// reloaded.

  gameRegion,   // Memory used in current game. Cleared whenever a new
		// game is loaded. Lifetime is the current game
		// session, loading a saved game will reset the
		// region.

  nifRegion;	// Used by the NIF loader. Cleared when a NIF file has
		// been inserted into OGRE and the file data is no
		// longer needed. In other words this is cleared
		// immediately after loading each NIF file.

// AA allocator that uses esmRegion
struct ESMRegionAlloc
{
  static const bool autoinit = false;
  static void* alloc(uint size) { return esmRegion.allocate(size).ptr; }
  static void free(void* p) { }
}

void initializeMemoryRegions()
{
  // Default block sizes are probably ok
  esmRegion = new RegionManager("ESM");
  gameRegion = new RegionManager("GAME");
  nifRegion = new RegionManager("NIF");
}
