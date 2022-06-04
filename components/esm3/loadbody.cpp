#include "loadbody.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void BodyPart::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

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
                case fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case fourCC("FNAM"):
                    mRace = esm.getHString();
                    break;
                case fourCC("BYDT"):
                    esm.getHTSized<4>(mData);
                    hasData = true;
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
            esm.fail("Missing BYDT subrecord");
    }

    void BodyPart::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mRace);
        esm.writeHNT("BYDT", mData, 4);
    }

    void BodyPart::blank()
    {
        mRecordFlags = 0;
        mData.mPart = 0;
        mData.mVampire = 0;
        mData.mFlags = 0;
        mData.mType = 0;

        mModel.clear();
        mRace.clear();
    }
}
