#include "loadappa.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Apparatus::sRecordId = REC_APPA;

void Apparatus::load(ESMReader &esm)
{
    bool hasData = false;
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
            case ESM::FourCC<'A','A','D','T'>::value:
                esm.getHT(mData);
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
        esm.fail("Missing AADT");
}

void Apparatus::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    esm.writeHNT("AADT", mData, 16);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNCString("ITEX", mIcon);
}

    void Apparatus::blank()
    {
        mData.mType = 0;
        mData.mQuality = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mName.clear();
    }
}
