#include "loadmisc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Miscellaneous::sRecordId = REC_MISC;

void Miscellaneous::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "MCDT", 12);
    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
}
void Miscellaneous::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("MCDT", mData, 12);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}

    void Miscellaneous::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mIsKey = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}
