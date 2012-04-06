#include "loadstat.hpp"

namespace ESM
{

void Static::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
}
void Static::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
}

}
