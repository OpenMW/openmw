#include "loadstat.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Static::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
}
void Static::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
}

}
