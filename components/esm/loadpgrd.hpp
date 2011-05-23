#ifndef _ESM_PGRD_H
#define _ESM_PGRD_H

#include "esm_reader.hpp"

namespace ESM
{

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

    void load(ESMReader &esm);
};
}
#endif
