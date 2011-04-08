#include "loadench.hpp"

namespace ESM
{

void Enchantment::load(ESMReader &esm)
{
    esm.getHNT(data, "ENDT", 16);
    effects.load(esm);
}

}
