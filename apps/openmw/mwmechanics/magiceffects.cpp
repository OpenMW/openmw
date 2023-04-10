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
#include "../mwbase/world.hpp"

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
        , mArg(-1)
    {
    }

    EffectKey::EffectKey(const ESM::ENAMstruct& effect)
    {
        mId = effect.mEffectID;
        mArg = -1;

        if (effect.mSkill != -1)
            mArg = effect.mSkill;

        if (effect.mAttribute != -1)
        {
            if (mArg != -1)
                throw std::runtime_error("magic effect can't have both a skill and an attribute argument");

            mArg = effect.mAttribute;
        }
    }

    std::string EffectKey::toString() const
    {
        const ESM::MagicEffect* magicEffect
            = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().search(mId);
        return getMagicEffectString(*magicEffect, mArg, mArg);
    }

    bool operator<(const EffectKey& left, const EffectKey& right)
    {
        if (left.mId < right.mId)
            return true;

        if (left.mId > right.mId)
            return false;

        return left.mArg < right.mArg;
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

    void MagicEffects::remove(const EffectKey& key)
    {
        mCollection.erase(key);
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

    void MagicEffects::setModifiers(const MagicEffects& effects)
    {
        for (Collection::iterator it = mCollection.begin(); it != mCollection.end(); ++it)
        {
            it->second.setModifier(effects.get(it->first).getModifier());
        }

        for (Collection::const_iterator it = effects.begin(); it != effects.end(); ++it)
        {
            mCollection[it->first].setModifier(it->second.getModifier());
        }
    }

    EffectParam MagicEffects::get(const EffectKey& key) const
    {
        Collection::const_iterator iter = mCollection.find(key);

        if (iter == mCollection.end())
        {
            return EffectParam();
        }
        else
        {
            return iter->second;
        }
    }

    MagicEffects MagicEffects::diff(const MagicEffects& prev, const MagicEffects& now)
    {
        MagicEffects result;

        // adding/changing
        for (Collection::const_iterator iter(now.begin()); iter != now.end(); ++iter)
        {
            Collection::const_iterator other = prev.mCollection.find(iter->first);

            if (other == prev.end())
            {
                // adding
                result.add(iter->first, iter->second);
            }
            else
            {
                // changing
                result.add(iter->first, iter->second - other->second);
            }
        }

        // removing
        for (Collection::const_iterator iter(prev.begin()); iter != prev.end(); ++iter)
        {
            Collection::const_iterator other = now.mCollection.find(iter->first);
            if (other == now.end())
            {
                result.add(iter->first, EffectParam() - iter->second);
            }
        }

        return result;
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

    std::string getMagicEffectString(const ESM::MagicEffect& effect, int attributeArg, int skillArg)
    {
        const bool targetsSkill = effect.mData.mFlags & ESM::MagicEffect::TargetSkill && skillArg != -1;
        const bool targetsAttribute = effect.mData.mFlags & ESM::MagicEffect::TargetAttribute && attributeArg != -1;

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
            const std::string& effectIDStr = ESM::MagicEffect::effectIdToString(effect.mIndex);
            spellLine = windowManager->getGameSettingString(effectIDStr, {});
        }

        if (targetsSkill)
        {
            spellLine += ' ';
            spellLine += windowManager->getGameSettingString(ESM::Skill::sSkillNameIds[skillArg], {});
        }
        else if (targetsAttribute)
        {
            spellLine += ' ';
            spellLine += windowManager->getGameSettingString(ESM::Attribute::sGmstAttributeIds[attributeArg], {});
        }
        return spellLine;
    }
}
