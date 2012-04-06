#include "loadench.hpp"

namespace ESM
{

void Enchantment::load(ESMReader &esm)
{
    esm.getHNT(data, "ENDT", 16);
    effects.load(esm);
}
void Enchantment::save(ESMWriter &esm)
{
    esm.writeHNT("ENDT", data, 16);
    effects.save(esm);
}

}
