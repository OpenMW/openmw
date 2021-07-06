#ifndef OPENMW_ESM_INGR_H
#define OPENMW_ESM_INGR_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Alchemy ingredient
 */

struct Ingredient
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Ingredient"; }

    struct IRDTstruct
    {
        float mWeight;
        int mValue;
        int mEffectID[4]; // Effect, 0 or -1 means none
        int mSkills[4]; // SkillEnum related to effect
        int mAttributes[4]; // Attribute related to effect
    };

    IRDTstruct mData;
    unsigned int mRecordFlags;
    std::string mId, mName, mModel, mIcon, mScript;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
