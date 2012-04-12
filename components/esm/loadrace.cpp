#include "loadrace.hpp"

namespace ESM
{

void Race::load(ESMReader &esm)
{
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "RADT", 140);
    powers.load(esm);
    description = esm.getHNOString("DESC");
}
void Race::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", name);
    esm.writeHNT("RADT", data, 140);
    powers.save(esm);
    esm.writeHNOString("DESC", description);
}

}
