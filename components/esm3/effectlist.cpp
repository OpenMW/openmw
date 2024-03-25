#include "effectlist.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<ENAMstruct> T>
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
            mList[i].mIndex = i;
    }

    void EffectList::add(ESMReader& esm)
    {
        ENAMstruct s;
        esm.getSubComposite(s);
        mList.push_back({ s, static_cast<uint32_t>(mList.size()) });
    }

    void EffectList::save(ESMWriter& esm) const
    {
        for (const IndexedENAMstruct& enam : mList)
        {
            esm.writeNamedComposite("ENAM", enam.mData);
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
