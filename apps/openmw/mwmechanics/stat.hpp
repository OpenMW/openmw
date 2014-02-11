#ifndef GAME_MWMECHANICS_STAT_H
#define GAME_MWMECHANICS_STAT_H

#undef min
#undef max

#include <limits>

namespace MWMechanics
{
    template<typename T>
    class Stat
    {
            T mBase;
            T mModified;

        public:
            typedef T Type;

            Stat() : mBase (0), mModified (0) {}
            Stat(T base) : mBase (base), mModified (base) {}
            Stat(T base, T modified) : mBase (base), mModified (modified) {}

            const T& getBase() const
            {
                return mBase;
            }

            T getModified() const
            {
                return std::max(static_cast<T>(0), mModified);
            }

            T getModifier() const
            {
                return mModified-mBase;
            }

            /// Set base and modified to \a value.
            void set (const T& value)
            {
                mBase = mModified = value;
            }

            void modify(const T& diff)
            {
                mBase += diff;
                if(mBase >= static_cast<T>(0))
                    mModified += diff;
                else
                {
                    mModified += diff - mBase;
                    mBase = static_cast<T>(0);
                }
            }

            /// Set base and adjust modified accordingly.
            void setBase (const T& value)
            {
                T diff = value - mBase;
                mBase = value;
                mModified += diff;
            }

            /// Set modified value an adjust base accordingly.
            void setModified (T value, const T& min, const T& max = std::numeric_limits<T>::max())
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
            }

            void setModifier (const T& modifier)
            {
                mModified = mBase + modifier;
            }
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

            DynamicStat() : mStatic (0), mCurrent (0) {}
            DynamicStat(T base) : mStatic (base), mCurrent (base) {}
            DynamicStat(T base, T modified, T current) : mStatic(base, modified), mCurrent (current) {}
            DynamicStat(const Stat<T> &stat, T current) : mStatic(stat), mCurrent (current) {}

            const T& getBase() const
            {
                return mStatic.getBase();
            }

            T getModified() const
            {
                return mStatic.getModified();
            }

            const T& getCurrent() const
            {
                return mCurrent;
            }

            /// Set base, modified and current to \a value.
            void set (const T& value)
            {
                mStatic.set (value);
                mCurrent = value;
            }

            /// Set base and adjust modified accordingly.
            void setBase (const T& value)
            {
                mStatic.setBase (value);

                if (mCurrent>getModified())
                    mCurrent = getModified();
            }

            /// Set modified value an adjust base accordingly.
            void setModified (T value, const T& min, const T& max = std::numeric_limits<T>::max())
            {
                mStatic.setModified (value, min, max);

                if (mCurrent>getModified())
                    mCurrent = getModified();
            }

            /// Change modified relatively.
            void modify (const T& diff)
            {
                mStatic.modify (diff);
                setCurrent (getCurrent()+diff);
            }

            void setCurrent (const T& value, bool allowDecreaseBelowZero = false)
            {
                if (value > mCurrent)
                {
                    // increase
                    mCurrent = value;

                    if (mCurrent > getModified())
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

            void setModifier (const T& modifier)
            {
                T diff =  modifier - mStatic.getModifier();
                mStatic.setModifier (modifier);
                setCurrent (getCurrent()+diff);
            }
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
        int mDamage;

    public:
        AttributeValue() : mBase(0), mModifier(0), mDamage(0) {}

        int getModified() const { return std::max(0, mBase - mDamage + mModifier); }
        int getBase() const { return mBase; }
        int getModifier() const {  return mModifier; }

        void setBase(int base) { mBase = std::max(0, base); }
        void setModifier(int mod) { mModifier = mod; }

        void damage(int damage) { mDamage += damage; }
        void restore(int amount) { mDamage -= std::min(mDamage, amount); }
        int getDamage() const { return mDamage; }
    };

    class SkillValue : public AttributeValue
    {
        float mProgress;
    public:
        SkillValue() : mProgress(0) {}
        float getProgress() const { return mProgress; }
        void setProgress(float progress) { mProgress = progress; }
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
