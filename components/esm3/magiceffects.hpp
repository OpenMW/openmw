#ifndef COMPONENTS_ESM_MAGICEFFECTS_H
#define COMPONENTS_ESM_MAGICEFFECTS_H

#include <compare>
#include <map>
#include <string>
#include <tuple>
#include <utility>

#include <components/esm/refid.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct MagicEffects
    {
        // <Effect Id, Base value, Modifier>
        std::map<int, std::pair<int, float>> mEffects;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };

    struct SummonKey
    {
        SummonKey(int effectId, const ESM::RefId& sourceId, int index)
            : mEffectId(effectId)
            , mSourceId(sourceId)
            , mEffectIndex(index)
        {
        }

        int mEffectId;
        ESM::RefId mSourceId;
        int mEffectIndex;
    };

    inline auto makeTupleRef(const SummonKey& value) noexcept
    {
        return std::tie(value.mEffectId, value.mSourceId, value.mEffectIndex);
    }

    inline bool operator==(const SummonKey& l, const SummonKey& r) noexcept
    {
        return makeTupleRef(l) == makeTupleRef(r);
    }

    inline bool operator<(const SummonKey& l, const SummonKey& r) noexcept
    {
        return makeTupleRef(l) < makeTupleRef(r);
    }
}

#endif
