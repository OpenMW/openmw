#include "metadata.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadtes3.hpp>

void CSMWorld::MetaData::blank()
{
    // ESM::Header::CurrentFormat is `1` but since new records are not yet used in opencs
    // we use the format `0` for compatibility with old versions.
    mFormatVersion = ESM::DefaultFormatVersion;
    mAuthor.clear();
    mDescription.clear();
}

void CSMWorld::MetaData::load(ESM::ESMReader& esm)
{
    mFormatVersion = esm.getHeader().mFormatVersion;
    mAuthor = esm.getHeader().mData.author;
    mDescription = esm.getHeader().mData.desc;
}

void CSMWorld::MetaData::save(ESM::ESMWriter& esm) const
{
    esm.setFormatVersion(mFormatVersion);
    esm.setAuthor(mAuthor);
    esm.setDescription(mDescription);
}
