#include "loadclas.hpp"

namespace ESM
{

void Class::load(ESMReader &esm)
{
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "CLDT", 60);

    if (data.isPlayable > 1)
        esm.fail("Unknown bool value");

    description = esm.getHNOString("DESC");
}

}
