#include "loadltex.hpp"

namespace ESM
{

void LandTexture::load(ESMReader &esm)
{
    esm.getHNT(index, "INTV");
    texture = esm.getHNString("DATA");
}
void LandTexture::save(ESMWriter &esm)
{
    esm.writeHNT("INTV", index);
    esm.writeHNCString("DATA", texture);
}

}
