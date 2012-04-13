#include "loadbsgn.hpp"

namespace ESM
{

void BirthSign::load(ESMReader &esm)
{
    name = esm.getHNString("FNAM");
    texture = esm.getHNOString("TNAM");
    description = esm.getHNOString("DESC");

    powers.load(esm);
}
void BirthSign::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", name);
    esm.writeHNOCString("TNAM", texture);
    esm.writeHNOCString("DESC", description);

    powers.save(esm);
}

}
