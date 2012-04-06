#include "loadspel.hpp"

namespace ESM
{

void Spell::load(ESMReader &esm)
{
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "SPDT", 12);
    effects.load(esm);
}
void Spell::save(ESMWriter &esm)
{
    esm.writeHNOString("FNAM", name);
    esm.writeHNT("SPDT", data, 12);
    effects.save(esm);
}

}
