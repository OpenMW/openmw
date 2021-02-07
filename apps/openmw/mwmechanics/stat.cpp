#include "stat.hpp"

#include <components/esm/statstate.hpp>

namespace MWMechanics
{
    template<typename T>
    Stat<T>::Stat() : mBase (0), mModified (0), mCurrentModified (0) {}
    template<typename T>
    Stat<T>::Stat(T base) : mBase (base), mModified (base), mCurrentModified (base) {}
    template<typename T>
    Stat<T>::Stat(T base, T modified) : mBase (base), mModified (modified), mCurrentModified (modified) {}

    template<typename T>
    const T& Stat<T>::getBase() const
    {
        return mBase;
    }

    template<typename T>
    T Stat<T>::getModified(bool capped) const
    {
        if(!capped)
            return mModified;
        return std::max(static_cast<T>(0), mModified);
    }

    template<typename T>
    T Stat<T>::getCurrentModified() const
    {
        return mCurrentModified;
    }

    template<typename T>
    T Stat<T>::getModifier() const
    {
        return mModified-mBase;
    }

    template<typename T>
    T Stat<T>::getCurrentModifier() const
    {
        return mCurrentModified - mModified;
    }

    template<typename T>
    void Stat<T>::set (const T& value)
    {
        T diff = value - mBase;
        mBase = mModified = value;
        mCurrentModified += diff;
    }

    template<typename T>
    void Stat<T>::setBase (const T& value)
    {
        T diff = value - mBase;
        mBase = value;
        mModified += diff;
        mCurrentModified += diff;
    }

    template<typename T>
    void Stat<T>::setModified (T value, const T& min, const T& max)
    {
        T diff = value - mModified;

        if (mBase+diff<min)
        {
            value = min + (mModified - mBase);
            diff = value - mModified;
        }
        else if (mBase+diff>max)
        {
            value = max + (mModified - mBase);
            diff = value - mModified;
        }

        mModified = value;
        mBase += diff;
        mCurrentModified += diff;
    }

    template<typename T>
    void Stat<T>::setCurrentModified(T value)
    {
        mCurrentModified = value;
    }

    template<typename T>
    void Stat<T>::setModifier (const T& modifier)
    {
        mModified = mBase + modifier;
    }

    template<typename T>
    void Stat<T>::setCurrentModifier(const T& modifier)
    {
        mCurrentModified = mModified + modifier;
    }

    template<typename T>
    void Stat<T>::writeState (ESM::StatState<T>& state) const
    {
        state.mBase = mBase;
        state.mMod = mCurrentModified;
    }
    template<typename T>
    void Stat<T>::readState (const ESM::StatState<T>& state)
    {
        mBase = state.mBase;
        mModified = state.mBase;
        mCurrentModified = state.mMod;
    }


    template<typename T>
    DynamicStat<T>::DynamicStat() : mStatic (0), mCurrent (0) {}
    template<typename T>
    DynamicStat<T>::DynamicStat(T base) : mStatic (base), mCurrent (base) {}
    template<typename T>
    DynamicStat<T>::DynamicStat(T base, T modified, T current) : mStatic(base, modified), mCurrent (current) {}
    template<typename T>
    DynamicStat<T>::DynamicStat(const Stat<T> &stat, T current) : mStatic(stat), mCurrent (current) {}


    template<typename T>
    const T& DynamicStat<T>::getBase() const
    {
        return mStatic.getBase();
    }
    template<typename T>
    T DynamicStat<T>::getModified() const
    {
        return mStatic.getModified();
    }
    template<typename T>
    T DynamicStat<T>::getCurrentModified() const
    {
        return mStatic.getCurrentModified();
    }

    template<typename T>
    const T& DynamicStat<T>::getCurrent() const
    {
        return mCurrent;
    }

    template<typename T>
    void DynamicStat<T>::set (const T& value)
    {
        mStatic.set (value);
        mCurrent = value;
    }
    template<typename T>
    void DynamicStat<T>::setBase (const T& value)
    {
        mStatic.setBase (value);

        if (mCurrent>getModified())
            mCurrent = getModified();
    }
    template<typename T>
    void DynamicStat<T>::setModified (T value, const T& min, const T& max)
    {
        mStatic.setModified (value, min, max);

        if (mCurrent>getModified())
            mCurrent = getModified();
    }
    template<typename T>
    void DynamicStat<T>::setCurrentModified(T value)
    {
        mStatic.setCurrentModified(value);
    }
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
    void DynamicStat<T>::setModifier (const T& modifier, bool allowCurrentDecreaseBelowZero)
    {
        T diff =  modifier - mStatic.getModifier();
        mStatic.setModifier (modifier);
        setCurrent (getCurrent()+diff, allowCurrentDecreaseBelowZero);
    }

    template<typename T>
    void DynamicStat<T>::setCurrentModifier(const T& modifier, bool allowCurrentDecreaseBelowZero)
    {
        T diff = modifier - mStatic.getCurrentModifier();
        mStatic.setCurrentModifier(modifier);

        // The (modifier > 0) check here allows increase over modified only if the modifier is positive (a fortify effect is active).
        setCurrent (getCurrent() + diff, allowCurrentDecreaseBelowZero, (modifier > 0));
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

    void AttributeValue::setBase(float base)
    {
        mBase = base;
    }

    void AttributeValue::setModifier(float mod)
    {
        mModifier = mod;
    }

    void AttributeValue::damage(float damage)
    {
        float threshold = mBase + mModifier;

        if (mDamage + damage > threshold)
            mDamage = threshold;
        else
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
