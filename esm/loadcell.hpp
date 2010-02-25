#ifndef _ESM_CELL_H
#define _ESM_CELL_H

#include "esm_reader.hpp"

namespace ESM {

/* Cells hold data about objects, creatures, statics (rocks, walls,
 * buildings) and landscape (for exterior cells). Cells frequently
 * also has other associated LAND and PGRD records. Combined, all this
 * data can be huge, and we cannot load it all at startup. Instead,
 * the strategy we use is to remember the file position of each cell
 * (using ESMReader::getContext()) and jumping back into place
 * whenever we need to load a given cell.
 */

struct Cell
{
  enum Flags
    {
      Interior  = 0x01, // Interior cell
      HasWater  = 0x02, // Does this cell have a water surface
      NoSleep   = 0x04, // Is it allowed to sleep here (without a bed)
      QuasiEx   = 0x80  // Behave like exterior (Tribunal+), with
                        // skybox and weather
    };

  struct DATAstruct
  {
    int flags;
    int gridX, gridY;
  };

  // Interior cells are indexed by this (it's the 'id'), for exterior
  // cells it is optional.
  std::string name,

  // Optional region name for exterior cells.
    region;

  // File position
  ESM_Context context;

  DATAstruct data;

  void load(ESMReader &esm)
  {
    // This is implicit?
    //name = esm.getHNString("NAME");

    // Ignore this for now, I assume it might mean we delete the entire cell?
    if(esm.isNextSub("DELE")) esm.skipHSub();

    esm.getHNT(data, "DATA", 12);

    // Save position and move on
    context = esm.getContext();
    esm.skipRecord();

    region = esm.getHNOString("RGNN");
  }
};
}
#endif
