#include "loadbsgn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int BirthSign::sRecordId = REC_BSGN;

    void BirthSign::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mPowers.mList.clear();

        bool hasName = false;
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
                case ESM::FourCC<'T','N','A','M'>::value:
                    mTexture = esm.getHString();
                    break;
                case ESM::FourCC<'D','E','S','C'>::value:
                    mDescription = esm.getHString();
                    break;
                case ESM::FourCC<'N','P','C','S'>::value:
                    mPowers.add(esm);
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
        mName.clear();
        mDescription.clear();
        mTexture.clear();
        mPowers.mList.clear();
    }

}
