#include "loaddoor.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Door::sRecordId = REC_DOOR;

void Door::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    mScript = esm.getHNOString("SCRI");
    mOpenSound = esm.getHNOString("SNAM");
    mCloseSound = esm.getHNOString("ANAM");
}

void Door::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("SNAM", mOpenSound);
    esm.writeHNOCString("ANAM", mCloseSound);
}

    void Door::blank()
    {
        mName.clear();
        mModel.clear();
        mScript.clear();
        mOpenSound.clear();
        mCloseSound.clear();
    }
}
