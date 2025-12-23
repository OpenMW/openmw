#include "effectlist.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <format>

#include <components/esm3/loadmgef.hpp>
#include <components/misc/concepts.hpp>

namespace ESM
{
    namespace
    {
        // ENAM format defined by Morrowind.esm
        struct EsmENAMstruct
        {
            int16_t mEffectID;
            signed char mSkill, mAttribute;
            int32_t mRange, mArea, mDuration, mMagnMin, mMagnMax;
        };

        void toBinary(const ENAMstruct& src, EsmENAMstruct& dst)
        {
            int16_t index = static_cast<int16_t>(ESM::MagicEffect::refIdToIndex(src.mEffectID));
            if (index < 0 || index >= ESM::MagicEffect::Length)
                throw std::runtime_error(std::format("Cannot serialize effect {}", src.mEffectID.toDebugString()));
            dst.mEffectID = index;
            dst.mSkill = src.mSkill;
            dst.mAttribute = src.mAttribute;
            dst.mRange = src.mRange;
            dst.mArea = src.mArea;
            dst.mDuration = src.mDuration;
            dst.mMagnMin = src.mMagnMin;
            dst.mMagnMax = src.mMagnMax;
        }

        void fromBinary(const EsmENAMstruct& src, ENAMstruct& dst)
        {
            int16_t index = src.mEffectID;
            if (index < 0 || index >= ESM::MagicEffect::Length)
                throw std::runtime_error(std::format("Cannot deserialize effect with index {}", index));
            dst.mEffectID = ESM::MagicEffect::indexToRefId(index);
            dst.mSkill = src.mSkill;
            dst.mAttribute = src.mAttribute;
            dst.mRange = src.mRange;
            dst.mArea = src.mArea;
            dst.mDuration = src.mDuration;
            dst.mMagnMin = src.mMagnMin;
            dst.mMagnMax = src.mMagnMax;
        }
    }

    template <Misc::SameAsWithoutCvref<EsmENAMstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mEffectID, v.mSkill, v.mAttribute, v.mRange, v.mArea, v.mDuration, v.mMagnMin, v.mMagnMax);
    }

    void EffectList::load(ESMReader& esm)
    {
        mList.clear();
        while (esm.isNextSub("ENAM"))
        {
            add(esm);
        }
    }

    void EffectList::populate(const std::vector<ENAMstruct>& effects)
    {
        mList.clear();
        for (size_t i = 0; i < effects.size(); i++)
            mList.push_back({ effects[i], static_cast<uint32_t>(i) });
    }

    void EffectList::updateIndexes()
    {
        for (size_t i = 0; i < mList.size(); i++)
            mList[i].mIndex = static_cast<uint32_t>(i);
    }

    void EffectList::add(ESMReader& esm)
    {
        EsmENAMstruct bin;
        esm.getSubComposite(bin);

        ENAMstruct s;
        fromBinary(bin, s);
        mList.push_back({ s, static_cast<uint32_t>(mList.size()) });
    }

    void EffectList::save(ESMWriter& esm) const
    {
        for (const IndexedENAMstruct& enam : mList)
        {
            EsmENAMstruct bin;
            toBinary(enam.mData, bin);
            esm.writeNamedComposite("ENAM", bin);
        }
    }

    bool IndexedENAMstruct::operator!=(const IndexedENAMstruct& rhs) const
    {
        return mData.mEffectID != rhs.mData.mEffectID || mData.mArea != rhs.mData.mArea
            || mData.mRange != rhs.mData.mRange || mData.mSkill != rhs.mData.mSkill
            || mData.mAttribute != rhs.mData.mAttribute || mData.mMagnMin != rhs.mData.mMagnMin
            || mData.mMagnMax != rhs.mData.mMagnMax || mData.mDuration != rhs.mData.mDuration;
    }

} // end namespace
