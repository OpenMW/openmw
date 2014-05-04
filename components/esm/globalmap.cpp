#include "globalmap.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

unsigned int ESM::GlobalMap::sRecordId = ESM::REC_GMAP;

void ESM::GlobalMap::load (ESMReader &esm)
{
    esm.getHNT(mBounds, "BNDS");

    esm.getSubNameIs("DATA");
    esm.getSubHeader();
    mImageData.resize(esm.getSubSize());
    esm.getExact(&mImageData[0], mImageData.size());
}

void ESM::GlobalMap::save (ESMWriter &esm) const
{
    esm.writeHNT("BNDS", mBounds);

    esm.startSubRecord("DATA");
    esm.write(&mImageData[0], mImageData.size());
    esm.endRecord("DATA");
}
