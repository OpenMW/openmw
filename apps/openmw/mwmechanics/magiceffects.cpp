
#include "magiceffects.hpp"

#include <cstdlib>

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

    void MagicEffects::add (const ESM::EffectList& list)
    {
        for (std::vector<ESM::ENAMstruct>::const_iterator iter (list.list.begin()); iter!=list.list.end();
            ++iter)
        {
            EffectParam param;

            if (iter->magnMin>=iter->magnMax)
                param.mMagnitude = iter->magnMin;
            else
                param.mMagnitude = static_cast<int> (
                    (iter->magnMax-iter->magnMin+1)*(std::rand() / RAND_MAX) + iter->magnMin);

            add (*iter, param);
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
            {
                result->second.mMagnitude += iter->second.mMagnitude;
            }
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

            if (other==prev.end())
            {
                result.add (iter->first, EffectParam() - iter->second);
            }
        }

        return result;
    }
}
