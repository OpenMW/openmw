#include "metadata.hpp"

#include <components/esm/loadtes3.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

void CSMWorld::MetaData::blank()
{
    mFormat = ESM::Header::CurrentFormat;
    mAuthor.clear();
    mDescription.clear();
}

void CSMWorld::MetaData::load (ESM::ESMReader& esm)
{
    mFormat = esm.getHeader().mFormat;
    mAuthor = esm.getHeader().mData.author;
    mDescription = esm.getHeader().mData.desc;
}

void CSMWorld::MetaData::save (ESM::ESMWriter& esm) const
{
    esm.setFormat (mFormat);
    esm.setAuthor (mAuthor);
    esm.setDescription (mDescription);
}
