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

    std::string mId, mName, mModel, mIcon, mScript;
    EffectList mEffects;

    bool mIsDeleted;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).

    };
}
#endif
