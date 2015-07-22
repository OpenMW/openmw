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

        public:
            typedef T Type;

            Stat();
            Stat(T base);
            Stat(T base, T modified);

            const T& getBase() const;

            T getModified() const;
            T getModifier() const;

            /// Set base and modified to \a value.
            void set (const T& value);
            void modify(const T& diff);

            /// Set base and adjust modified accordingly.
            void setBase (const T& value);

            /// Set modified value an adjust base accordingly.
            void setModified (T value, const T& min, const T& max = std::numeric_limits<T>::max());
            void setModifier (const T& modifier);

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
            const T& getCurrent() const;

            /// Set base, modified and current to \a value.
            void set (const T& value);

            /// Set base and adjust modified accordingly.
            void setBase (const T& value);

            /// Set modified value an adjust base accordingly.
            void setModified (T value, const T& min, const T& max = std::numeric_limits<T>::max());

            /// Change modified relatively.
            void modify (const T& diff, bool allowCurrentDecreaseBelowZero=false);

            void setCurrent (const T& value, bool allowDecreaseBelowZero = false);
            void setModifier (const T& modifier, bool allowCurrentDecreaseBelowZero=false);

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
        int mBase;
        int mModifier;
        float mDamage; // needs to be float to allow continuous damage

    public:
        AttributeValue();

        int getModified() const;
        int getBase() const;
        int getModifier() const;

        void setBase(int base);

        void setModifier(int mod);

        // Maximum attribute damage is limited to the modified value.
        // Note: I think MW applies damage directly to mModified, since you can also
        // "restore" drained attributes. We need to rewrite the magic effect system to support this.
        void damage(float damage);
        void restore(float amount);

        float getDamage() const;

        void writeState (ESM::StatState<int>& state) const;
        void readState (const ESM::StatState<int>& state);
    };

    class SkillValue : public AttributeValue
    {
        float mProgress;
    public:
        SkillValue();
        float getProgress() const;
        void setProgress(float progress);

        void writeState (ESM::StatState<int>& state) const;
        void readState (const ESM::StatState<int>& state);
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
