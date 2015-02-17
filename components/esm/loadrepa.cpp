#include "loadrepa.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Repair::sRecordId = REC_REPA;

void Repair::load(ESMReader &esm)
{
    bool hasData = true;
    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().val;
        switch (name)
        {
            case ESM::FourCC<'M','O','D','L'>::value:
                mModel = esm.getHString();
                break;
            case ESM::FourCC<'F','N','A','M'>::value:
                mName = esm.getHString();
                break;
            case ESM::FourCC<'R','I','D','T'>::value:
                esm.getHT(mData, 16);
                hasData = true;
                break;
            case ESM::FourCC<'S','C','R','I'>::value:
                mScript = esm.getHString();
                break;
            case ESM::FourCC<'I','T','E','X'>::value:
                mIcon = esm.getHString();
                break;
            default:
                esm.fail("Unknown subrecord");
        }
    }
    if (!hasData)
        esm.fail("Missing RIDT subrecord");
}

void Repair::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);

    esm.writeHNT("RIDT", mData, 16);
    esm.writeHNOString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
}

    void Repair::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mQuality = 0;
        mData.mUses = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}
