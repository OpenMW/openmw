#ifndef _ESM_BSGN_H
#define _ESM_BSGN_H

#include "defs.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct BirthSign
{
    std::string name, description, texture;

    // List of powers and abilities that come with this birth sign.
    SpellList powers;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
