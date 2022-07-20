#ifndef GAME_MWMECHANICS_STAT_H
#define GAME_MWMECHANICS_STAT_H

#include <algorithm>
#include <limits>

namespace ESM
{
    template<typename T>
    struct StatState;
}

namespace MWMechanics
{
    template<typename T>
    class Stat
    {
            T mBase;
            T mModifier;

        public:
            typedef T Type;

            Stat();
            Stat(T base, T modified);

            const T& getBase() const { return mBase; };

            T getModified(bool capped = true) const;
            T getModifier() const { return mModifier; };

            void setBase(const T& value) { mBase = value; };

            void setModifier(const T& modifier) { mModifier = modifier; };

            void writeState (ESM::StatState<T>& state) const;
            void readState (const ESM::StatState<T>& state);
    };

    template<typename T>
    inline bool operator== (const Stat<T>& left, const Stat<T>& right)
    {
        return left.getBase()==right.getBase() &&
            left.getModifier()==right.getModifier();
    }

    template<typename T>
    inline bool operator!= (const Stat<T>& left, const Stat<T>& right)
    {
        return !(left==right);
    }

    template<typename T>
    class DynamicStat
    {
            Stat<T> mStatic;
            T mCurrent;

        public:
            typedef T Type;

            DynamicStat();
            DynamicStat(T base);
            DynamicStat(T base, T modified, T current);
            DynamicStat(const Stat<T> &stat, T current);

            const T& getBase() const { return mStatic.getBase(); };
            T getModified(bool capped = true) const { return mStatic.getModified(capped); };
            const T& getCurrent() const { return mCurrent; };
            T getRatio(bool nanIsZero = true) const;

            /// Set base and adjust current accordingly.
            void setBase(const T& value) { mStatic.setBase(value); };

            void setCurrent (const T& value, bool allowDecreaseBelowZero = false, bool allowIncreaseAboveModified = false);

            T getModifier() const { return mStatic.getModifier(); }
            void setModifier(T value) { mStatic.setModifier(value); }

            void writeState (ESM::StatState<T>& state) const;
            void readState (const ESM::StatState<T>& state);
    };

    template<typename T>
    inline bool operator== (const DynamicStat<T>& left, const DynamicStat<T>& right)
    {
        return left.getBase()==right.getBase() &&
            left.getModifier()==right.getModifier() &&
            left.getCurrent()==right.getCurrent();
    }

    template<typename T>
    inline bool operator!= (const DynamicStat<T>& left, const DynamicStat<T>& right)
    {
        return !(left==right);
    }

    class AttributeValue
    {
        float mBase;
        float mModifier;
        float mDamage; // needs to be float to allow continuous damage

    public:
        AttributeValue();

        float getModified() const;
        float getBase() const;
        float getModifier() const;

        void setBase(float base, bool clearModifier = false);

        void setModifier(float mod);

        // Maximum attribute damage is limited to the modified value.
        // Note: MW applies damage directly to mModified, however it does track how much
        // a damaged attribute that has been fortified beyond its base can be restored.
        // Getting rid of mDamage would require calculating its value by ignoring active effects when restoring
        void damage(float damage);
        void restore(float amount);

        float getDamage() const;

        void writeState (ESM::StatState<float>& state) const;
        void readState (const ESM::StatState<float>& state);
    };

    class SkillValue : public AttributeValue
    {
        float mProgress;
    public:
        SkillValue();
        float getProgress() const;
        void setProgress(float progress);

        void writeState (ESM::StatState<float>& state) const;
        void readState (const ESM::StatState<float>& state);
    };

    inline bool operator== (const AttributeValue& left, const AttributeValue& right)
    {
        return left.getBase() == right.getBase()
                && left.getModifier() == right.getModifier()
                && left.getDamage() == right.getDamage();
    }
    inline bool operator!= (const AttributeValue& left, const AttributeValue& right)
    {
        return !(left == right);
    }

    inline bool operator== (const SkillValue& left, const SkillValue& right)
    {
        return left.getBase() == right.getBase()
                && left.getModifier() == right.getModifier()
                && left.getDamage() == right.getDamage()
                && left.getProgress() == right.getProgress();
    }
    inline bool operator!= (const SkillValue& left, const SkillValue& right)
    {
        return !(left == right);
    }
}

#endif
