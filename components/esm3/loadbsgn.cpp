#include "loadbsgn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void BirthSign::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mPowers.mList.clear();

        bool hasName = false;
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
                case fourCC("TNAM"):
                    mTexture = esm.getHString();
                    break;
                case fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                case fourCC("NPCS"):
                    mPowers.add(esm);
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
    }

    void BirthSign::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("TNAM", mTexture);
        esm.writeHNOCString("DESC", mDescription);

        mPowers.save(esm);
    }

    void BirthSign::blank()
    {
        mRecordFlags = 0;
        mName.clear();
        mDescription.clear();
        mTexture.clear();
        mPowers.mList.clear();
    }

}
