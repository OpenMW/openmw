#include "loadalch.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
void Potion::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mIcon = esm.getHNOString("TEXT"); // not ITEX here for some reason
    mScript = esm.getHNOString("SCRI");
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "ALDT", 12);
    mEffects.load(esm);
}
void Potion::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNOCString("TEXT", mIcon);
    esm.writeHNOCString("SCRI", mScript);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("ALDT", mData, 12);
    mEffects.save(esm);
}
}
