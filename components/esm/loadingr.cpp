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

    // horrible hack to fix broken data in records
    for (int i=0; i<4; ++i)
    {
        if (data.effectID[i]!=85 && data.effectID[i]!=22 && data.effectID[i]!=17 && data.effectID[i]!=79 &&
            data.effectID[i]!=74)
            data.attributes[i] = -1;

        if (data.effectID[i]!=89 && data.effectID[i]!=26 && data.effectID[i]!=21 && data.effectID[i]!=83 &&
            data.effectID[i]!=78)
            data.skills[i] = -1;            
    }
}

}
