#include "magiceffects.hpp"

#include <cmath>
#include <stdexcept>

#include <components/esm/attr.hpp>
#include <components/esm3/effectlist.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/magiceffects.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/esmstore.hpp"

namespace
{
    // Round value to prevent precision issues
    void truncate(float& value)
    {
        value = static_cast<int>(value * 1024.f) / 1024.f;
    }
}

namespace MWMechanics
{
    EffectKey::EffectKey()
        : mId(0)
    {
    }

    EffectKey::EffectKey(const ESM::ENAMstruct& effect)
    {
        mId = effect.mEffectID;
        mArg = ESM::Skill::indexToRefId(effect.mSkill);

        ESM::RefId attribute = ESM::Attribute::indexToRefId(effect.mAttribute);
        if (!attribute.empty())
        {
            if (!mArg.empty())
                throw std::runtime_error("magic effect can't have both a skill and an attribute argument");

            mArg = attribute;
        }
    }

    std::string EffectKey::toString() const
    {
        const auto& store = MWBase::Environment::get().getESMStore();
        const ESM::MagicEffect* magicEffect = store->get<ESM::MagicEffect>().find(mId);
        return getMagicEffectString(
            *magicEffect, store->get<ESM::Attribute>().search(mArg), store->get<ESM::Skill>().search(mArg));
    }

    bool operator<(const EffectKey& left, const EffectKey& right)
    {
        if (left.mId < right.mId)
            return true;

        if (left.mId > right.mId)
            return false;

        return left.mArg < right.mArg;
    }

    bool operator==(const EffectKey& left, const EffectKey& right)
    {
        return left.mId == right.mId && left.mArg == right.mArg;
    }

    float EffectParam::getMagnitude() const
    {
        return mBase + mModifier;
    }

    void EffectParam::modifyBase(int diff)
    {
        mBase += diff;
    }

    int EffectParam::getBase() const
    {
        return mBase;
    }

    void EffectParam::setBase(int base)
    {
        mBase = base;
    }

    void EffectParam::setModifier(float mod)
    {
        mModifier = mod;
    }

    float EffectParam::getModifier() const
    {
        return mModifier;
    }

    EffectParam::EffectParam()
        : mModifier(0)
        , mBase(0)
    {
    }

    EffectParam& EffectParam::operator+=(const EffectParam& param)
    {
        mModifier += param.mModifier;
        mBase += param.mBase;
        truncate(mModifier);
        return *this;
    }

    EffectParam& EffectParam::operator-=(const EffectParam& param)
    {
        mModifier -= param.mModifier;
        mBase -= param.mBase;
        truncate(mModifier);
        return *this;
    }

    void MagicEffects::add(const EffectKey& key, const EffectParam& param)
    {
        Collection::iterator iter = mCollection.find(key);

        if (iter == mCollection.end())
        {
            mCollection.insert(std::make_pair(key, param));
        }
        else
        {
            iter->second += param;
        }
    }

    void MagicEffects::modifyBase(const EffectKey& key, int diff)
    {
        mCollection[key].modifyBase(diff);
    }

    EffectParam MagicEffects::getOrDefault(const EffectKey& key) const
    {
        return get(key).value_or(EffectParam());
    }

    std::optional<EffectParam> MagicEffects::get(const EffectKey& key) const
    {
        Collection::const_iterator iter = mCollection.find(key);

        if (iter != mCollection.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    void MagicEffects::writeState(ESM::MagicEffects& state) const
    {
        for (const auto& [key, params] : mCollection)
        {
            if (params.getBase() != 0 || params.getModifier() != 0.f)
            {
                // Don't worry about mArg, never used by magic effect script instructions
                state.mEffects[key.mId] = { params.getBase(), params.getModifier() };
            }
        }
    }

    void MagicEffects::readState(const ESM::MagicEffects& state)
    {
        for (const auto& [key, params] : state.mEffects)
        {
            mCollection[EffectKey(key)].setBase(params.first);
            mCollection[EffectKey(key)].setModifier(params.second);
        }
    }

    std::string getMagicEffectString(
        const ESM::MagicEffect& effect, const ESM::Attribute* attribute, const ESM::Skill* skill)
    {
        const bool targetsSkill = effect.mData.mFlags & ESM::MagicEffect::TargetSkill && skill;
        const bool targetsAttribute = effect.mData.mFlags & ESM::MagicEffect::TargetAttribute && attribute;

        std::string spellLine;

        auto windowManager = MWBase::Environment::get().getWindowManager();

        if (targetsSkill || targetsAttribute)
        {
            switch (effect.mIndex)
            {
                case ESM::MagicEffect::AbsorbAttribute:
                case ESM::MagicEffect::AbsorbSkill:
                    spellLine = windowManager->getGameSettingString("sAbsorb", {});
                    break;
                case ESM::MagicEffect::DamageAttribute:
                case ESM::MagicEffect::DamageSkill:
                    spellLine = windowManager->getGameSettingString("sDamage", {});
                    break;
                case ESM::MagicEffect::DrainAttribute:
                case ESM::MagicEffect::DrainSkill:
                    spellLine = windowManager->getGameSettingString("sDrain", {});
                    break;
                case ESM::MagicEffect::FortifyAttribute:
                case ESM::MagicEffect::FortifySkill:
                    spellLine = windowManager->getGameSettingString("sFortify", {});
                    break;
                case ESM::MagicEffect::RestoreAttribute:
                case ESM::MagicEffect::RestoreSkill:
                    spellLine = windowManager->getGameSettingString("sRestore", {});
                    break;
            }
        }

        if (spellLine.empty())
        {
            auto& effectIDStr = ESM::MagicEffect::indexToGmstString(effect.mIndex);
            spellLine = windowManager->getGameSettingString(effectIDStr, {});
        }

        if (targetsSkill)
        {
            spellLine += ' ';
            spellLine += skill->mName;
        }
        else if (targetsAttribute)
        {
            spellLine += ' ';
            spellLine += attribute->mName;
        }
        return spellLine;
    }
}
