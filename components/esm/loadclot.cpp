#include "loadclot.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Clothing::sRecordId = REC_CLOT;

    void Clothing::load(ESMReader &esm)
    {
        mModel = esm.getHNString("MODL");
        mName = esm.getHNOString("FNAM");
        esm.getHNT(mData, "CTDT", 12);

        mScript = esm.getHNOString("SCRI");
        mIcon = esm.getHNOString("ITEX");

        mParts.load(esm);


        mEnchant = esm.getHNOString("ENAM");
    }

    void Clothing::save(ESMWriter &esm) const
    {
        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CTDT", mData, 12);

        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);

        mParts.save(esm);

        esm.writeHNOCString("ENAM", mEnchant);
    }

    void Clothing::blank()
    {
        mData.mType = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mEnchant = 0;
        mParts.mParts.clear();
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant.clear();
        mScript.clear();
    }
}
