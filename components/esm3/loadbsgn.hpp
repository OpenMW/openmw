#ifndef OPENMW_ESM_BSGN_H
#define OPENMW_ESM_BSGN_H

#include <string>

#include "spelllist.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

struct BirthSign
{
    constexpr static RecNameInts sRecordId = REC_BSGN;

    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string_view getRecordType() { return "BirthSign"; }

    unsigned int mRecordFlags;
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
