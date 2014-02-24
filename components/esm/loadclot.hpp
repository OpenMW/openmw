#ifndef OPENMW_ESM_CLOT_H
#define OPENMW_ESM_CLOT_H

#include <string>

#include "loadarmo.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Clothing
 */

struct Clothing
{
    static unsigned int sRecordId;

    enum Type
    {
        Pants = 0,
        Shoes = 1,
        Shirt = 2,
        Belt = 3,
        Robe = 4,
        RGlove = 5,
        LGlove = 6,
        Skirt = 7,
        Ring = 8,
        Amulet = 9
    };

    struct CTDTstruct
    {
        int mType;
        float mWeight;
        short mValue;
        short mEnchant;
    };
    CTDTstruct mData;

    PartReferenceList mParts;

    std::string mId, mName, mModel, mIcon, mEnchant, mScript;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
