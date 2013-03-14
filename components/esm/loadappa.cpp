#include "loadappa.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
void Apparatus::load(ESMReader &esm)
{
    // we will not treat duplicated subrecords as errors here
    while (esm.hasMoreSubs())
    {
        esm.getSubName();
        NAME subName = esm.retSubName();

        if (subName == "MODL")
            mModel = esm.getHString();
        else if (subName == "FNAM")
            mName = esm.getHString();
        else if (subName == "AADT")
            esm.getHT(mData);
        else if (subName == "SCRI")
            mScript = esm.getHString();
        else if (subName == "ITEX")
            mIcon = esm.getHString();
        else
            esm.fail("wrong subrecord type " + subName.toString() + " for APPA record");
    }
}

void Apparatus::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    esm.writeHNT("AADT", mData, 16);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNCString("ITEX", mIcon);
}
}
