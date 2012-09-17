#ifndef _ESM_BSGN_H
#define _ESM_BSGN_H

#include <string>

#include "record.hpp"
#include "spelllist.hpp"

namespace ESM
{

struct BirthSign : public Record
{
    std::string mName, mDescription, mTexture;

    // List of powers and abilities that come with this birth sign.
    SpellList mPowers;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_BSGN; }
};
}
#endif
