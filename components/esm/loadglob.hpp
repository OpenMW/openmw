#ifndef _ESM_GLOB_H
#define _ESM_GLOB_H

#include "record.hpp"
#include "defs.hpp"

namespace ESM
{

/*
 * Global script variables
 */

struct Global : public Record
{
    unsigned mValue;
    VarType mType;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_GLOB; }
};
}
#endif
