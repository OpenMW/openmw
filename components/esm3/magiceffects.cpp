#include "magiceffects.hpp"

#include "esmwriter.hpp"
#include "esmreader.hpp"

namespace ESM
{

void MagicEffects::save(ESMWriter &esm) const
{
    for (const auto& [key, params] : mEffects)
    {
        esm.writeHNT("EFID", key);
        esm.writeHNT("BASE", params.first);
        esm.writeHNT("MODI", params.second);
    }
}

void MagicEffects::load(ESMReader &esm)
{
    while (esm.isNextSub("EFID"))
    {
        int id;
        std::pair<int, float> params;
        esm.getHT(id);
        esm.getHNT(params.first, "BASE");
        if(esm.getFormat() < 17)
            params.second = 0.f;
        else
            esm.getHNT(params.second, "MODI");
        mEffects.emplace(id, params);
    }
}

}
