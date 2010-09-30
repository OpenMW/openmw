
#include "magiceffects.hpp"

#include <stdexcept>

#include <components/esm/defs.hpp>

namespace MWMechanics
{
    EffectKey::EffectKey() : mId (0), mArg (-1) {}

    EffectKey::EffectKey (const ESM::ENAMstruct& effect)
    {
        mId = effect.effectID;
        mArg = -1;

        if (effect.skill!=-1)
            mArg = effect.skill;

        if (effect.attribute!=-1)
        {
            if (mArg!=-1)
                throw std::runtime_error (
                    "magic effect can't have both a skill and an attribute argument");

            mArg = effect.attribute;
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

    EffectParam::EffectParam() : mMagnitude (0) {}

    EffectParam& EffectParam::operator+= (const EffectParam& param)
    {
        mMagnitude += param.mMagnitude;
        return *this;
    }

    EffectParam& EffectParam::operator-= (const EffectParam& param)
    {
        mMagnitude -= param.mMagnitude;
        return *this;
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

    MagicEffects MagicEffects::diff (const MagicEffects& prev, const MagicEffects& now)
    {
        MagicEffects result;

        // adding/changing
        for (Collection::const_iterator iter (now.Begin()); iter!=now.End(); ++iter)
        {
            Collection::const_iterator other = prev.mCollection.find (iter->first);

            if (other==prev.End())
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
        for (Collection::const_iterator iter (prev.Begin()); iter!=prev.End(); ++iter)
        {
            Collection::const_iterator other = now.mCollection.find (iter->first);

            if (other==prev.End())
            {
                result.add (iter->first, EffectParam() - iter->second);
            }
        }

        return result;

    }
}
