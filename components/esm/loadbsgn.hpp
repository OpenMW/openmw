#ifndef OPENMW_ESM_BSGN_H
#define OPENMW_ESM_BSGN_H

#include <string>

#include "record.hpp"
#include "spelllist.hpp"

namespace ESM
{

struct BirthSign
{
    std::string mName, mDescription, mTexture;

    // List of powers and abilities that come with this birth sign.
    SpellList mPowers;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
