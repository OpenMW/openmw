#include "loadbook.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Book::sRecordId = REC_BOOK;

void Book::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "BKDT", 20);
    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
    mText = esm.getHNOString("TEXT");
    mEnchant = esm.getHNOString("ENAM");
}
void Book::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("BKDT", mData, 20);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
    esm.writeHNOString("TEXT", mText);
    esm.writeHNOCString("ENAM", mEnchant);
}

    void Book::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mIsScroll = 0;
        mData.mSkillID = 0;
        mData.mEnchant = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEnchant.clear();
        mText.clear();
    }
}
