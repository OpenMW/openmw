#include "globalmap.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{

unsigned int GlobalMap::sRecordId = REC_GMAP;

void GlobalMap::load (ESMReader &esm)
{
    esm.getHNT(mBounds, "BNDS");

    esm.getSubNameIs("DATA");
    esm.getSubHeader();
    mImageData.resize(esm.getSubSize());
    esm.getExact(&mImageData[0], mImageData.size());

    while (esm.isNextSub("MRK_"))
    {
        esm.getSubHeader();
        CellId cell;
        esm.getT(cell.first);
        esm.getT(cell.second);
        mMarkers.insert(cell);
    }
}

void GlobalMap::save (ESMWriter &esm) const
{
    esm.writeHNT("BNDS", mBounds);

    esm.startSubRecord("DATA");
    esm.write(&mImageData[0], mImageData.size());
    esm.endRecord("DATA");

    for (std::set<CellId>::const_iterator it = mMarkers.begin(); it != mMarkers.end(); ++it)
    {
        esm.startSubRecord("MRK_");
        esm.writeT(it->first);
        esm.writeT(it->second);
        esm.endRecord("MRK_");
    }
}

}
