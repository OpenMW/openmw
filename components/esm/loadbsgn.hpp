#ifndef OPENMW_ESM_BSGN_H
#define OPENMW_ESM_BSGN_H

#include <string>

#include "spelllist.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

struct BirthSign
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "BirthSign"; }

    std::string mId, mName, mDescription, mTexture;

    // List of powers and abilities that come with this birth sign.
    SpellList mPowers;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID/index).
};
}
#endif
