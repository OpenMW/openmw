#include "loadbody.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void BodyPart::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mRace = esm.getHNString("FNAM");
    esm.getHNT(mData, "BYDT", 4);
}
void BodyPart::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mRace);
    esm.writeHNT("BYDT", mData, 4);
}

}
