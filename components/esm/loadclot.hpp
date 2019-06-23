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
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Clothing"; }

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
        unsigned short mValue;
        unsigned short mEnchant;
    };
    CTDTstruct mData;

    PartReferenceList mParts;

    std::string mId, mName, mModel, mIcon, mEnchant, mScript;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
