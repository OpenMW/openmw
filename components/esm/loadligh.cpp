#include "loadligh.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Light::sRecordId = REC_LIGH;

    void Light::load(ESMReader &esm)
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
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
                    break;
                case ESM::FourCC<'L','H','D','T'>::value:
                    esm.getHT(mData, 24);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                    mSound = esm.getHString();
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }
        if (!hasData)
            esm.fail("Missing LHDT subrecord");
    }
    void Light::save(ESMWriter &esm) const
    {
        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNT("LHDT", mData, 24);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("SNAM", mSound);
    }

    void Light::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mTime = 0;
        mData.mRadius = 0;
        mData.mColor = 0;
        mData.mFlags = 0;
        mSound.clear();
        mScript.clear();
        mModel.clear();
        mIcon.clear();
        mName.clear();
    }
}
