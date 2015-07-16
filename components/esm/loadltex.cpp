#include "loadltex.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int LandTexture::sRecordId = REC_LTEX;

    LandTexture::LandTexture()
        : mIsDeleted(false)
    {}

    void LandTexture::load(ESMReader &esm)
    {
        mIsDeleted = false;

        bool hasName = false;
        bool hasIndex = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            uint32_t name = esm.retSubName().val;
            switch (name)
            {
                case ESM::FourCC<'N','A','M','E'>::value:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'D','E','L','E'>::value:
                    esm.skipHSub();
                    mIsDeleted = true;
                    break;
                case ESM::FourCC<'I','N','T','V'>::value:
                    esm.getHT(mIndex);
                    hasIndex = true;
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    mTexture = esm.getHString();
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
    void LandTexture::save(ESMWriter &esm) const
    {
        if (mIsDeleted)
        {
            esm.writeHNCString("DELE", "");
        }

        esm.writeHNCString("NAME", mId);
        esm.writeHNT("INTV", mIndex);
        esm.writeHNCString("DATA", mTexture);
    }

    void LandTexture::blank()
    {
        mTexture.clear();
        mIndex = -1;
        mIsDeleted = false;
    }
}
