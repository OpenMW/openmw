#include "loadligh.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/defs.hpp>
#include <components/esm/esmcommon.hpp>
#include <components/esm/fourcc.hpp>

namespace ESM
{
    void Light::load(ESMReader& esm, bool& isDeleted)
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
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
                case fourCC("LHDT"):
                    esm.getHTSized<24>(mData);
                    hasData = true;
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("SNAM"):
                    mSound = esm.getRefId();
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
            esm.fail("Missing LHDT subrecord");
    }
    void Light::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId.getRefIdString());

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNT("LHDT", mData, 24);
        esm.writeHNOCString("SCRI", mScript.getRefIdString());
        esm.writeHNOCString("SNAM", mSound.getRefIdString());
    }

    void Light::blank()
    {
        mRecordFlags = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mTime = 0;
        mData.mRadius = 0;
        mData.mColor = 0;
        mData.mFlags = 0;
        mSound = ESM::RefId::sEmpty;
        mScript = ESM::RefId::sEmpty;
        mModel.clear();
        mIcon.clear();
        mName.clear();
    }
}
