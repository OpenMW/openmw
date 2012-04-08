#ifndef _ESM_GLOB_H
#define _ESM_GLOB_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"
#include "defs.hpp"

namespace ESM
{

/*
 * Global script variables
 */

struct Global : public Record
{
    unsigned value;
    VarType type;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_GLOB; }
};
}
#endif
