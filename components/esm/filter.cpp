
#include "filter.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::Filter::load (ESMReader& esm)
{
    mFilter = esm.getHNString ("FILT");
}

void ESM::Filter::save (ESMWriter& esm)
{
    esm.writeHNCString ("FILT", mFilter);
}

void ESM::Filter::blank()
{
    mFilter.clear();
}
