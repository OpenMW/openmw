#include "magiceffects.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm3/loadmgef.hpp>

namespace ESM
{

    void MagicEffects::save(ESMWriter& esm) const
    {
        for (const auto& [key, params] : mEffects)
        {
            esm.writeHNT("EFID", ESM::MagicEffect::refIdToIndex(key));
            esm.writeHNT("BASE", params.first);
            esm.writeHNT("MODI", params.second);
        }
    }

    void MagicEffects::load(ESMReader& esm)
    {
        while (esm.isNextSub("EFID"))
        {
            int32_t id;
            std::pair<int32_t, float> params;
            esm.getHT(id);
            esm.getHNT(params.first, "BASE");
            if (esm.getFormatVersion() <= MaxClearModifiersFormatVersion)
                params.second = 0.f;
            else
                esm.getHNT(params.second, "MODI");
            mEffects.emplace(ESM::MagicEffect::indexToRefId(id), params);
        }
    }

}
