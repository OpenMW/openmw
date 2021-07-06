#ifndef OPENMW_ESM_ALCH_H
#define OPENMW_ESM_ALCH_H

#include <string>

#include "effectlist.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Alchemy item (potions)
 */

struct Potion
{
    static unsigned int sRecordId;

    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Potion"; }

    struct ALDTstruct
    {
        float mWeight;
        int mValue;
        int mAutoCalc;
    };
    ALDTstruct mData;

    unsigned int mRecordFlags;
    std::string mId, mName, mModel, mIcon, mScript;
    EffectList mEffects;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).

    };
}
#endif
