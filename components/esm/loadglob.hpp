#ifndef _ESM_GLOB_H
#define _ESM_GLOB_H

#include "esm_reader.hpp"
#include "defs.hpp"

namespace ESM
{

/*
 * Global script variables
 */

struct Global
{
    unsigned value;
    VarType type;

    void load(ESMReader &esm);
};
}
#endif
