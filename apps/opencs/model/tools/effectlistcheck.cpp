#include "effectlistcheck.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/effectlist.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/world/universalid.hpp>

namespace CSMTools
{
    void effectListCheck(
        const std::vector<ESM::IndexedENAMstruct>& list, CSMDoc::Messages& messages, const CSMWorld::UniversalId& id)
    {
        if (list.empty())
        {
            messages.add(id, "No magic effects", "", CSMDoc::Message::Severity_Warning);
            return;
        }

        size_t i = 1;
        for (const ESM::IndexedENAMstruct& effect : list)
        {
            const std::string number = std::to_string(i);

            // At the time of writing this effects, attributes and skills are mostly hardcoded
            int effectIndex = ESM::MagicEffect::refIdToIndex(effect.mData.mEffectID);
            if (effectIndex < -1 || effectIndex >= ESM::MagicEffect::Length)
                messages.add(id, "Effect #" + number + ": invalid effect ID", "", CSMDoc::Message::Severity_Error);
            if (effect.mData.mSkill < -1 || effect.mData.mSkill >= ESM::Skill::Length)
                messages.add(id, "Effect #" + number + ": invalid skill", "", CSMDoc::Message::Severity_Error);
            if (effect.mData.mAttribute < -1 || effect.mData.mAttribute >= ESM::Attribute::Length)
                messages.add(id, "Effect #" + number + ": invalid attribute", "", CSMDoc::Message::Severity_Error);

            if (effect.mData.mRange < ESM::RT_Self || effect.mData.mRange > ESM::RT_Target)
                messages.add(id, "Effect #" + number + ": invalid range", "", CSMDoc::Message::Severity_Error);

            if (effect.mData.mArea < 0)
                messages.add(id, "Effect #" + number + ": negative area", "", CSMDoc::Message::Severity_Error);

            if (effect.mData.mDuration < 0)
                messages.add(id, "Effect #" + number + ": negative duration", "", CSMDoc::Message::Severity_Error);

            if (effect.mData.mMagnMin < 0)
                messages.add(
                    id, "Effect #" + number + ": negative minimum magnitude", "", CSMDoc::Message::Severity_Error);

            if (effect.mData.mMagnMax < 0)
                messages.add(
                    id, "Effect #" + number + ": negative maximum magnitude", "", CSMDoc::Message::Severity_Error);
            else if (effect.mData.mMagnMax == 0)
                messages.add(
                    id, "Effect #" + number + ": zero maximum magnitude", "", CSMDoc::Message::Severity_Warning);

            if (effect.mData.mMagnMin > effect.mData.mMagnMax)
                messages.add(id, "Effect #" + number + ": minimum magnitude is higher than maximum magnitude", "",
                    CSMDoc::Message::Severity_Error);

            ++i;
        }
    }

    void ingredientEffectListCheck(
        const ESM::Ingredient& ingredient, CSMDoc::Messages& messages, const CSMWorld::UniversalId& id)
    {
        bool hasEffects = false;

        for (size_t i = 0; i < 4; i++)
        {
            if (ingredient.mData.mEffectID[i].empty())
                continue;

            hasEffects = true;
            int effectIndex = ESM::MagicEffect::refIdToIndex(ingredient.mData.mEffectID[i]);

            const std::string number = std::to_string(i + 1);
            if (effectIndex < -1 || effectIndex >= ESM::MagicEffect::Length)
                messages.add(id, "Effect #" + number + ": invalid effect ID", "", CSMDoc::Message::Severity_Error);
            if (ingredient.mData.mSkills[i] < -1 || ingredient.mData.mSkills[i] >= ESM::Skill::Length)
                messages.add(id, "Effect #" + number + ": invalid skill", "", CSMDoc::Message::Severity_Error);
            if (ingredient.mData.mAttributes[i] < -1 || ingredient.mData.mAttributes[i] >= ESM::Attribute::Length)
                messages.add(id, "Effect #" + number + ": invalid attribute", "", CSMDoc::Message::Severity_Error);
        }

        if (!hasEffects)
            messages.add(id, "No magic effects", "", CSMDoc::Message::Severity_Warning);
    }
}
