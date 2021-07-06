#include "loadingr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Ingredient::sRecordId = REC_INGR;

    void Ingredient::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'M','O','D','L'>::value:
                    mModel = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'I','R','D','T'>::value:
                    esm.getHT(mData, 56);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing IRDT subrecord");

        // horrible hack to fix broken data in records
        for (int i=0; i<4; ++i)
        {
            if (mData.mEffectID[i] != 85 &&
                mData.mEffectID[i] != 22 &&
                mData.mEffectID[i] != 17 &&
                mData.mEffectID[i] != 79 &&
                mData.mEffectID[i] != 74)
            {
                mData.mAttributes[i] = -1;
            }

            // is this relevant in cycle from 0 to 4?
            if (mData.mEffectID[i] != 89 &&
                mData.mEffectID[i] != 26 &&
                mData.mEffectID[i] != 21 &&
                mData.mEffectID[i] != 83 &&
                mData.mEffectID[i] != 78)
            {
                mData.mSkills[i] = -1;
            }
        }
    }

    void Ingredient::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("IRDT", mData, 56);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
    }

    void Ingredient::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        for (int i=0; i<4; ++i)
        {
            mData.mEffectID[i] = 0;
            mData.mSkills[i] = 0;
            mData.mAttributes[i] = 0;
        }

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}
