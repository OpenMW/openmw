#ifndef OPENMW_ESM_BOOK_H
#define OPENMW_ESM_BOOK_H

#include "record.hpp"

namespace ESM
{
/*
 * Books, magic scrolls, notes and so on
 */

struct Book : public Record
{
    struct BKDTstruct
    {
        float mWeight;
        int mValue, mIsScroll, mSkillID, mEnchant;
    };

    BKDTstruct mData;
    std::string mName, mModel, mIcon, mScript, mEnchant, mText;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
    
    int getName() { return REC_BOOK; }
};
}
#endif
