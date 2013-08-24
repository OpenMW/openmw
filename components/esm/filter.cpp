
#include "filter.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::Filter::load (ESMReader& esm)
{
    mFilter = esm.getHNString ("FILT");
    mDescription = esm.getHNString ("DESC");
}

void ESM::Filter::save (ESMWriter& esm)
{
    esm.writeHNCString ("FILT", mFilter);
    esm.writeHNCString ("DESC", mDescription);
}

void ESM::Filter::blank()
{
    mFilter.clear();
    mDescription.clear();
}
