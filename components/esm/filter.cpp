#include "filter.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::Filter::sRecordId = REC_FILT;

void ESM::Filter::load (ESMReader& esm, bool &isDeleted)
{
    isDeleted = false;

    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().intval;
        switch (name)
        {
            case ESM::SREC_NAME:
                mId = esm.getHString();
                break;
            case ESM::FourCC<'F','I','L','T'>::value:
                mFilter = esm.getHString();
                break;
            case ESM::FourCC<'D','E','S','C'>::value:
                mDescription = esm.getHString();
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
}

void ESM::Filter::save (ESMWriter& esm, bool isDeleted) const
{
    esm.writeHNCString ("NAME", mId);

    if (isDeleted)
    {
        esm.writeHNString("DELE", "", 3);
        return;
    }

    esm.writeHNCString ("FILT", mFilter);
    esm.writeHNCString ("DESC", mDescription);
}

void ESM::Filter::blank()
{
    mFilter.clear();
    mDescription.clear();
}
