#include "loadspel.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void Spell::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mEffects.mList.clear();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("SPDT"):
                    esm.getHTSized<12>(mData);
                    hasData = true;
                    break;
                case fourCC("ENAM"):
                    ENAMstruct s;
                    esm.getHTSized<24>(s);
                    mEffects.mList.push_back(s);
                    break;
                case SREC_DELE:
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
        mRecordFlags = 0;
        mData.mType = 0;
        mData.mCost = 0;
        mData.mFlags = 0;

        mName.clear();
        mEffects.mList.clear();
    }
}
