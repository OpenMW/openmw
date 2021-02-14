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
            T mModified;
            T mCurrentModified;

        public:
            typedef T Type;

            Stat();
            Stat(T base);
            Stat(T base, T modified);

            const T& getBase() const;

            T getModified(bool capped = true) const;
            T getCurrentModified() const;
            T getModifier() const;
            T getCurrentModifier() const;

            /// Set base and modified to \a value.
            void set (const T& value);

            /// Set base and adjust modified accordingly.
            void setBase (const T& value);

            /// Set modified value and adjust base accordingly.
            void setModified (T value, const T& min, const T& max = std::numeric_limits<T>::max());

            /// Set "current modified," used for drain and fortify. Unlike the regular modifier
            /// this just adds and subtracts from the current value without changing the maximum.
            void setCurrentModified(T value);

            void setModifier (const T& modifier);
            void setCurrentModifier (const T& modifier);

            void writeState (ESM::StatState<T>& state) const;
            void readState (const ESM::StatState<T>& state);
    };

    template<typename T>
    inline bool operator== (const Stat<T>& left, const Stat<T>& right)
    {
        return left.getBase()==right.getBase() &&
            left.getModified()==right.getModified();
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

            const T& getBase() const;
            T getModified() const;
            T getCurrentModified() const;
            const T& getCurrent() const;

            /// Set base, modified and current to \a value.
            void set (const T& value);

            /// Set base and adjust modified accordingly.
            void setBase (const T& value);

            /// Set modified value and adjust base accordingly.
            void setModified (T value, const T& min, const T& max = std::numeric_limits<T>::max());

            /// Set "current modified," used for drain and fortify. Unlike the regular modifier
            /// this just adds and subtracts from the current value without changing the maximum.
            void setCurrentModified(T value);

            void setCurrent (const T& value, bool allowDecreaseBelowZero = false, bool allowIncreaseAboveModified = false);
            void setModifier (const T& modifier, bool allowCurrentToDecreaseBelowZero=false);
            void setCurrentModifier (const T& modifier, bool allowCurrentToDecreaseBelowZero = false);

            void writeState (ESM::StatState<T>& state) const;
            void readState (const ESM::StatState<T>& state);
    };

    template<typename T>
    inline bool operator== (const DynamicStat<T>& left, const DynamicStat<T>& right)
    {
        return left.getBase()==right.getBase() &&
            left.getModified()==right.getModified() &&
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

        void setBase(float base);

        void setModifier(float mod);

        // Maximum attribute damage is limited to the modified value.
        // Note: I think MW applies damage directly to mModified, since you can also
        // "restore" drained attributes. We need to rewrite the magic effect system to support this.
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
