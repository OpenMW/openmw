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
            esm.writeHNRefId("EFID", key);
            esm.writeHNT("BASE", params.first);
            esm.writeHNT("MODI", params.second);
        }
    }

    void MagicEffects::load(ESMReader& esm)
    {
        while (esm.isNextSub("EFID"))
        {
            std::pair<int32_t, float> params;
            if (esm.getFormatVersion() <= MaxSerializeEffectRefIdFormatVersion)
            {
                int32_t id;
                esm.getHT(id);
                esm.getHNT(params.first, "BASE");
                if (esm.getFormatVersion() <= MaxClearModifiersFormatVersion)
                    params.second = 0.f;
                else
                    esm.getHNT(params.second, "MODI");
                mEffects.emplace(ESM::MagicEffect::indexToRefId(id), params);
            }
            else
            {
                RefId effectId = esm.getRefId();
                esm.getHNT(params.first, "BASE");
                esm.getHNT(params.second, "MODI");
                mEffects.emplace(effectId, params);
            }
        }
    }

}
