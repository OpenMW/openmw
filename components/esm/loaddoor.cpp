#include "loaddoor.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Door::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    mScript = esm.getHNOString("SCRI");
    mOpenSound = esm.getHNOString("SNAM");
    mCloseSound = esm.getHNOString("ANAM");
}

void Door::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("SNAM", mOpenSound);
    esm.writeHNOCString("ANAM", mCloseSound);
}

}
