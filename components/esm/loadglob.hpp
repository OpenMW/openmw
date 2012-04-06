#ifndef _ESM_GLOB_H
#define _ESM_GLOB_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"
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
    void save(ESMWriter &esm);
};
}
#endif
