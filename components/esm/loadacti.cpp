#include "loadacti.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Activator::sRecordId = REC_ACTI;

void Activator::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNString("FNAM");
    mScript = esm.getHNOString("SCRI");
}
void Activator::save(ESMWriter &esm) const
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    esm.writeHNOCString("SCRI", mScript);
}

    void Activator::blank()
    {
        mName.clear();
        mScript.clear();
        mModel.clear();
    }
}
