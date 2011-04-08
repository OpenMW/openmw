#include "loadpgrd.hpp"

namespace ESM
{

void PathGrid::load(ESMReader &esm)
{
    esm.getHNT(data, "DATA", 12);
    cell = esm.getHNString("NAME");

    // Remember this file position
    context = esm.getContext();

    // Check that the sizes match up. Size = 16 * s2 (path points?)
    if (esm.isNextSub("PGRP"))
    {
        esm.skipHSub();
        int size = esm.getSubSize();
        if (size != 16 * data.s2)
            esm.fail("Path grid table size mismatch");
    }

    // Size varies. Path grid chances? Connections? Multiples of 4
    // suggest either int or two shorts, or perhaps a float. Study
    // it later.
    if (esm.isNextSub("PGRC"))
    {
        esm.skipHSub();
        int size = esm.getSubSize();
        if (size % 4 != 0)
            esm.fail("PGRC size not a multiple of 4");
    }
}

}
