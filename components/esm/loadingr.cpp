#include "loadingr.hpp"

namespace ESM
{

void Ingredient::load(ESMReader &esm, const std::string& id)
{
    mId = id;

    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "IRDT", 56);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
}

}
