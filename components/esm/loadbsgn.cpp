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
    esm.writeHNString("FNAM", name);
    if (!texture.empty())
        esm.writeHNString("TNAM", texture);
    if (!description.empty())
        esm.writeHNString("DESC", description);

    powers.save(esm);
}

}
