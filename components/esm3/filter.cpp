#include "filter.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{

unsigned int Filter::sRecordId = REC_FILT;

void Filter::load (ESMReader& esm, bool &isDeleted)
{
    isDeleted = false;
    mRecordFlags = esm.getRecordFlags();

    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        uint32_t name = esm.retSubName().toInt();
        switch (name)
        {
            case SREC_NAME:
                mId = esm.getHString();
                break;
            case fourCC("FILT"):
                mFilter = esm.getHString();
                break;
            case fourCC("DESC"):
                mDescription = esm.getHString();
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
}

void Filter::save (ESMWriter& esm, bool isDeleted) const
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

void Filter::blank()
{
    mFilter.clear();
    mDescription.clear();
}

}
