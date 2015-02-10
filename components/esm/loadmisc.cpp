#include "loadmisc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Miscellaneous::sRecordId = REC_MISC;

    void Miscellaneous::load(ESMReader &esm)
    {
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            uint32_t name = esm.retSubName().val;
            switch (name)
            {
                case ESM::FourCC<'M','O','D','L'>::value:
                    mModel = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'M','C','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
                    break;
            }
        }
        if (!hasData)
            esm.fail("Missing MCDT subrecord");
    }

    void Miscellaneous::save(ESMWriter &esm) const
    {
        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("MCDT", mData, 12);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
    }

    void Miscellaneous::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mIsKey = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}
