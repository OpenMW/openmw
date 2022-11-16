#include "loadclot.hpp"

#include <vector>

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/defs.hpp>
#include <components/esm/esmcommon.hpp>
#include <components/esm/fourcc.hpp>
#include <components/esm3/loadarmo.hpp>

namespace ESM
{
    void Clothing::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mParts.mParts.clear();

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
                case fourCC("CTDT"):
                    esm.getHTSized<12>(mData);
                    hasData = true;
                    break;
                case fourCC("SCRI"):
                    mScript = esm.getRefId();
                    break;
                case fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
                case fourCC("ENAM"):
                    mEnchant = esm.getRefId();
                    break;
                case fourCC("INDX"):
                    mParts.add(esm);
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
            esm.fail("Missing CTDT subrecord");
    }

    void Clothing::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId.getRefIdString());

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CTDT", mData, 12);

        esm.writeHNOCString("SCRI", mScript.getRefIdString());
        esm.writeHNOCString("ITEX", mIcon);

        mParts.save(esm);

        esm.writeHNOCString("ENAM", mEnchant.getRefIdString());
    }

    void Clothing::blank()
    {
        mRecordFlags = 0;
        mData.mType = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mEnchant = 0;
        mParts.mParts.clear();
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant = ESM::RefId::sEmpty;
        mScript = ESM::RefId::sEmpty;
    }
}
