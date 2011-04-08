#ifndef _ESM_LAND_H
#define _ESM_LAND_H

#include "esm_reader.hpp"

namespace ESM
{
/*
 * Landscape data.
 */

struct Land
{
    int flags; // Only first four bits seem to be used, don't know what
    // they mean.
    int X, Y; // Map coordinates.

    // File context. This allows the ESM reader to be 'reset' to this
    // location later when we are ready to load the full data set.
    ESM_Context context;

    bool hasData;

    void load(ESMReader &esm);
};
}
#endif
