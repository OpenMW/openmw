#ifndef OPENMW_ESM_GLOB_H
#define OPENMW_ESM_GLOB_H

#include "record.hpp"
#include "defs.hpp"

namespace ESM
{

/*
 * Global script variables
 */

struct Global
{
    unsigned mValue;
    VarType mType;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
