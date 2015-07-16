#include "filter.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::Filter::sRecordId = REC_FILT;

ESM::Filter::Filter()
    : mIsDeleted(false)
{}

void ESM::Filter::load (ESMReader& esm)
{
    mIsDeleted = false;

    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().val;
        switch (name)
        {
            case ESM::FourCC<'N','A','M','E'>::value:
                mId = esm.getHString();
                break;
            case ESM::FourCC<'D','E','L','E'>::value:
                esm.skipHSub();
                mIsDeleted = true;
                break;
            case ESM::FourCC<'F','I','L','T'>::value:
                mFilter = esm.getHString();
                break;
            case ESM::FourCC<'D','E','S','C'>::value:
                mDescription = esm.getHString();
                break;
            default:
                esm.fail("Unknown subrecord");
                break;
        }
    }
}

void ESM::Filter::save (ESMWriter& esm) const
{
    esm.writeHNCString ("NAME", mId);

    if (mIsDeleted)
    {
        esm.writeHNCString("DELE", "");
        return;
    }

    esm.writeHNCString ("FILT", mFilter);
    esm.writeHNCString ("DESC", mDescription);
}

void ESM::Filter::blank()
{
    mFilter.clear();
    mDescription.clear();
    mIsDeleted = false;
}
