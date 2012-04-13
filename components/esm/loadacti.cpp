#include "loadacti.hpp"

namespace ESM
{
void Activator::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    script = esm.getHNOString("SCRI");
}
void Activator::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", model);
    esm.writeHNCString("FNAM", name);
    esm.writeHNOCString("SCRI", script);
}
}
