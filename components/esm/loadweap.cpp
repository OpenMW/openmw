#include "loadweap.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Weapon::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "WPDT", 32);
    mScript = esm.getHNOString("SCRI");
    mIcon = esm.getHNOString("ITEX");
    mEnchant = esm.getHNOString("ENAM");
}
void Weapon::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("WPDT", mData, 32);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("ITEX", mIcon);
    esm.writeHNOCString("ENAM", mEnchant);
}

}
