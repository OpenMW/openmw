#include "loadspel.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Spell::sRecordId = REC_SPEL;

    void Spell::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mEffects.mList.clear();

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
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'S','P','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'E','N','A','M'>::value:
                    ENAMstruct s;
                    esm.getHT(s, 24);
                    mEffects.mList.push_back(s);
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
            esm.fail("Missing SPDT subrecord");
    }

    void Spell::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("SPDT", mData, 12);
        mEffects.save(esm);
    }

    void Spell::blank()
    {
        mData.mType = 0;
        mData.mCost = 0;
        mData.mFlags = 0;

        mName.clear();
        mEffects.mList.clear();
    }
}
