#ifndef OPENMW_ESM_BOOK_H
#define OPENMW_ESM_BOOK_H

#include <string>

namespace ESM
{
/*
 * Books, magic scrolls, notes and so on
 */

class ESMReader;
class ESMWriter;

struct Book
{
    struct BKDTstruct
    {
        float mWeight;
        int mValue, mIsScroll, mSkillID, mEnchant;
    };

    BKDTstruct mData;
    std::string mName, mModel, mIcon, mScript, mEnchant, mText;
    std::string mId;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
