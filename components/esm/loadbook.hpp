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
        int mValue, mIsScroll, mSkillID, mEnchant;
    };

    BKDTstruct mData;
    std::string mName, mModel, mIcon, mScript, mEnchant, mText;
    std::string mId;

    bool mIsDeleted;

    Book();

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
