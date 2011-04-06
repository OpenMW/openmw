#include "loadacti.hpp"

namespace ESM
{
void Activator::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    script = esm.getHNOString("SCRI");
}
}
