#ifndef _ESM_INGR_H
#define _ESM_INGR_H

#include <string>

#include "record.hpp"

namespace ESM
{
/*
 * Alchemy ingredient
 */

struct Ingredient : public Record
{
    struct IRDTstruct
    {
        float mWeight;
        int mValue;
        int mEffectID[4]; // Effect, 0 or -1 means none
        int mSkills[4]; // SkillEnum related to effect
        int mAttributes[4]; // Attribute related to effect
    };

    IRDTstruct mData;
    std::string mName, mModel, mIcon, mScript;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_INGR; }
};
}
#endif
