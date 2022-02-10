#include "stat.hpp"

#include <components/esm3/statstate.hpp>

namespace MWMechanics
{
    template<typename T>
    Stat<T>::Stat() : mBase (0), mModifier (0) {}
    template<typename T>
    Stat<T>::Stat(T base, T modified) : mBase (base), mModifier (modified) {}

    template<typename T>
    T Stat<T>::getModified(bool capped) const
    {
        if(capped)
            return std::max({}, mModifier + mBase);
        return mModifier + mBase;
    }

    template<typename T>
    void Stat<T>::writeState (ESM::StatState<T>& state) const
    {
        state.mBase = mBase;
        state.mMod = mModifier;
    }
    template<typename T>
    void Stat<T>::readState (const ESM::StatState<T>& state)
    {
        mBase = state.mBase;
        mModifier = state.mMod;
    }


    template<typename T>
    DynamicStat<T>::DynamicStat() : mStatic(0, 0), mCurrent(0) {}
    template<typename T>
    DynamicStat<T>::DynamicStat(T base) : mStatic(base, 0), mCurrent(base) {}
    template<typename T>
    DynamicStat<T>::DynamicStat(T base, T modified, T current) : mStatic(base, modified), mCurrent (current) {}
    template<typename T>
    DynamicStat<T>::DynamicStat(const Stat<T> &stat, T current) : mStatic(stat), mCurrent (current) {}

    template<typename T>
    void DynamicStat<T>::setCurrent (const T& value, bool allowDecreaseBelowZero, bool allowIncreaseAboveModified)
    {
        if (value > mCurrent)
        {
            // increase
            if (value <= getModified() || allowIncreaseAboveModified)
                mCurrent = value;
            else if (mCurrent > getModified())
                return;
            else
                mCurrent = getModified();
        }
        else if (value > 0 || allowDecreaseBelowZero)
        {
            // allowed decrease
            mCurrent = value;
        }
        else if (mCurrent > 0)
        {
            // capped decrease
            mCurrent = 0;
        }
    }

    template<typename T>
    T DynamicStat<T>::getRatio(bool nanIsZero) const
    {
        T modified = getModified();
        if(modified == T{})
        {
            if(nanIsZero)
                return modified;
            return {1};
        }
        return getCurrent() / modified;
    }

    template<typename T>
    void DynamicStat<T>::writeState (ESM::StatState<T>& state) const
    {
        mStatic.writeState (state);
        state.mCurrent = mCurrent;
    }
    template<typename T>
    void DynamicStat<T>::readState (const ESM::StatState<T>& state)
    {
        mStatic.readState (state);
        mCurrent = state.mCurrent;
    }

    AttributeValue::AttributeValue() :
        mBase(0.f), mModifier(0.f), mDamage(0.f)
    {
    }

    float AttributeValue::getModified() const
    {
        return std::max(0.f, mBase - mDamage + mModifier);
    }
    float AttributeValue::getBase() const
    {
        return mBase;
    }
    float AttributeValue::getModifier() const
    {
        return mModifier;
    }

    void AttributeValue::setBase(float base, bool clearModifier)
    {
        mBase = base;
        if(clearModifier)
        {
            mModifier = 0.f;
            mDamage = 0.f;
        }
    }

    void AttributeValue::setModifier(float mod)
    {
        if(mod < 0)
        {
            mModifier = 0.f;
            mDamage -= mod;
        }
        else
            mModifier = mod;
    }

    void AttributeValue::damage(float damage)
    {
        mDamage += damage;
    }
    void AttributeValue::restore(float amount)
    {
        if (mDamage <= 0) return;

        mDamage -= std::min(mDamage, amount);
    }

    float AttributeValue::getDamage() const
    {
        return mDamage;
    }

    void AttributeValue::writeState (ESM::StatState<float>& state) const
    {
        state.mBase = mBase;
        state.mMod = mModifier;
        state.mDamage = mDamage;
    }

    void AttributeValue::readState (const ESM::StatState<float>& state)
    {
        mBase = state.mBase;
        mModifier = state.mMod;
        mDamage = state.mDamage;
    }

    SkillValue::SkillValue() :
        mProgress(0)
    {
    }

    float SkillValue::getProgress() const
    {
        return mProgress;
    }
    void SkillValue::setProgress(float progress)
    {
        mProgress = progress;
    }

    void SkillValue::writeState (ESM::StatState<float>& state) const
    {
        AttributeValue::writeState (state);
        state.mProgress = mProgress;
    }

    void SkillValue::readState (const ESM::StatState<float>& state)
    {
        AttributeValue::readState (state);
        mProgress = state.mProgress;
    }
}

template class MWMechanics::Stat<int>;
template class MWMechanics::Stat<float>;
template class MWMechanics::DynamicStat<int>;
template class MWMechanics::DynamicStat<float>;
