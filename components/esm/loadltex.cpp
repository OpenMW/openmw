#include "loadltex.hpp"

namespace ESM
{

void LandTexture::load(ESMReader &esm)
{
    esm.getHNT(index, "INTV");
    texture = esm.getHNString("DATA");
}

}
