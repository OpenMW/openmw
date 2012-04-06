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
    esm.writeHNString("MODL", model);
    esm.writeHNString("FNAM", name);
    if (!script.empty())
    {
        esm.writeHNString("SCRI", script);
    }
}
}
