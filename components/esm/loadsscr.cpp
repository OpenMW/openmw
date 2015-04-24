#include "loadsscr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int StartScript::sRecordId = REC_SSCR;

    void StartScript::load(ESMReader &esm)
    {
        bool hasData = false;
        bool hasName = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            uint32_t name = esm.retSubName().val;
            switch (name)
            {
                case ESM::FourCC<'D','A','T','A'>::value:
                    mData = esm.getHString();
                    hasData = true;
                    break;
                case ESM::FourCC<'N','A','M','E'>::value:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }
        if (!hasData)
            esm.fail("Missing DATA");
        if (!hasName)
            esm.fail("Missing NAME");
    }
    void StartScript::save(ESMWriter &esm) const
    {
        esm.writeHNString("DATA", mData);
        esm.writeHNString("NAME", mId);
    }

    void StartScript::blank()
    {
        mData.clear();
    }
}
