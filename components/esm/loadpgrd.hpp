#ifndef _ESM_PGRD_H
#define _ESM_PGRD_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Path grid.
 */
struct PathGrid
{
  struct DATAstruct
  {
    int x, y; // Grid location, matches cell for exterior cells
    short s1; // ?? Usually but not always a power of 2. Doesn't seem
          // to have any relation to the size of PGRC.
    short s2; // Number of path points? Size of PGRP block is always 16 * s2;
  }; // 12 bytes

  std::string cell; // Cell name
  DATAstruct data;
  ESM_Context context; // Context so we can return here later and
                       // finish the job

  void load(ESMReader &esm)
  {
    esm.getHNT(data, "DATA", 12);
    cell = esm.getHNString("NAME");

    // Remember this file position
    context = esm.getContext();
    
    // Check that the sizes match up. Size = 16 * s2 (path points?)
    if(esm.isNextSub("PGRP"))
      {
        esm.skipHSub();
    int size = esm.getSubSize();
    if(size != 16*data.s2)
      esm.fail("Path grid table size mismatch");
      }

    // Size varies. Path grid chances? Connections? Multiples of 4
    // suggest either int or two shorts, or perhaps a float. Study
    // it later.
    if(esm.isNextSub("PGRC"))
      {
    esm.skipHSub();
    int size = esm.getSubSize();
    if(size % 4 != 0)
      esm.fail("PGRC size not a multiple of 4");
      }
  }
};
}
#endif
