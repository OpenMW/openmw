
#include "filter.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::Filter::sRecordId = REC_FILT;

void ESM::Filter::load (ESMReader& esm)
{
    mFilter = esm.getHNString ("FILT");
    mDescription = esm.getHNString ("DESC");
}

void ESM::Filter::save (ESMWriter& esm) const
{
    esm.writeHNCString ("FILT", mFilter);
    esm.writeHNCString ("DESC", mDescription);
}

void ESM::Filter::blank()
{
    mFilter.clear();
    mDescription.clear();
}
