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

}
