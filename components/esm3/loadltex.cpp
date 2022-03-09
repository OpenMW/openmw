#include "loadltex.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int LandTexture::sRecordId = REC_LTEX;

    void LandTexture::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
        bool hasIndex = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::fourCC("INTV"):
                    esm.getHT(mIndex);
                    hasIndex = true;
                    break;
                case ESM::fourCC("DATA"):
                    mTexture = esm.getHString();
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
        if (!hasIndex)
            esm.fail("Missing INTV subrecord");
    }
    void LandTexture::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        esm.writeHNT("INTV", mIndex);
        esm.writeHNCString("DATA", mTexture);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
        }
    }

    void LandTexture::blank()
    {
        mId.clear();
        mTexture.clear();
    }
}
