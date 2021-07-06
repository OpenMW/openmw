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
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Book"; }

    struct BKDTstruct
    {
        float mWeight;
        int mValue, mIsScroll, mSkillId, mEnchant;
    };

    BKDTstruct mData;
    std::string mName, mModel, mIcon, mScript, mEnchant, mText;
    unsigned int mRecordFlags;
    std::string mId;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
