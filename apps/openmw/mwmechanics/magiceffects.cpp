#include "magiceffects.hpp"

#include <stdexcept>

#include <components/esm/effectlist.hpp>
#include <components/esm/magiceffects.hpp>

namespace MWMechanics
{
    EffectKey::EffectKey() : mId (0), mArg (-1) {}

    EffectKey::EffectKey (const ESM::ENAMstruct& effect)
    {
        mId = effect.mEffectID;
        mArg = -1;

        if (effect.mSkill!=-1)
            mArg = effect.mSkill;

        if (effect.mAttribute!=-1)
        {
            if (mArg!=-1)
                throw std::runtime_error (
                    "magic effect can't have both a skill and an attribute argument");

            mArg = effect.mAttribute;
        }
    }

    bool operator< (const EffectKey& left, const EffectKey& right)
    {
        if (left.mId<right.mId)
            return true;

        if (left.mId>right.mId)
            return false;

        return left.mArg<right.mArg;
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

    EffectParam::EffectParam() : mModifier (0), mBase(0) {}

    EffectParam& EffectParam::operator+= (const EffectParam& param)
    {
        mModifier += param.mModifier;
        mBase += param.mBase;
        return *this;
    }

    EffectParam& EffectParam::operator-= (const EffectParam& param)
    {
        mModifier -= param.mModifier;
        mBase -= param.mBase;
        return *this;
    }

    void MagicEffects::remove(const EffectKey &key)
    {
        mCollection.erase(key);
    }

    void MagicEffects::add (const EffectKey& key, const EffectParam& param)
    {
        Collection::iterator iter = mCollection.find (key);

        if (iter==mCollection.end())
        {
            mCollection.insert (std::make_pair (key, param));
        }
        else
        {
            iter->second += param;
        }
    }

    void MagicEffects::modifyBase(const EffectKey &key, int diff)
    {
        mCollection[key].modifyBase(diff);
    }

    void MagicEffects::setModifiers(const MagicEffects &effects)
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

    MagicEffects& MagicEffects::operator+= (const MagicEffects& effects)
    {
        if (this==&effects)
        {
            MagicEffects temp (effects);
            *this += temp;
            return *this;
        }

        for (Collection::const_iterator iter (effects.begin()); iter!=effects.end(); ++iter)
        {
            Collection::iterator result = mCollection.find (iter->first);

            if (result!=mCollection.end())
                result->second += iter->second;
            else
                mCollection.insert (*iter);
        }

        return *this;
    }

    EffectParam MagicEffects::get (const EffectKey& key) const
    {
        Collection::const_iterator iter = mCollection.find (key);

        if (iter==mCollection.end())
        {
            return EffectParam();
        }
        else
        {
            return iter->second;
        }
    }

    MagicEffects MagicEffects::diff (const MagicEffects& prev, const MagicEffects& now)
    {
        MagicEffects result;

        // adding/changing
        for (Collection::const_iterator iter (now.begin()); iter!=now.end(); ++iter)
        {
            Collection::const_iterator other = prev.mCollection.find (iter->first);

            if (other==prev.end())
            {
                // adding
                result.add (iter->first, iter->second);
            }
            else
            {
                // changing
                result.add (iter->first, iter->second - other->second);
            }
        }

        // removing
        for (Collection::const_iterator iter (prev.begin()); iter!=prev.end(); ++iter)
        {
            Collection::const_iterator other = now.mCollection.find (iter->first);
            if (other==now.end())
            {
                result.add (iter->first, EffectParam() - iter->second);
            }
        }

        return result;
    }

    void MagicEffects::writeState(ESM::MagicEffects &state) const
    {
        // Don't need to save Modifiers, they are recalculated every frame anyway.
        for (Collection::const_iterator iter (begin()); iter!=end(); ++iter)
        {
            if (iter->second.getBase() != 0)
            {
                // Don't worry about mArg, never used by magic effect script instructions
                state.mEffects.insert(std::make_pair(iter->first.mId, iter->second.getBase()));
            }
        }
    }

    void MagicEffects::readState(const ESM::MagicEffects &state)
    {
        for (std::map<int, int>::const_iterator it = state.mEffects.begin(); it != state.mEffects.end(); ++it)
        {
            mCollection[EffectKey(it->first)].setBase(it->second);
        }
    }
}
