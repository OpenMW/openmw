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

            DynamicStat() : mCurrent (0) {}
            DynamicStat(T current) : mCurrent (current) {}
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

            void setCurrent (const T& value)
            {
                mCurrent = value;

                if (mCurrent<0)
                    mCurrent = 0;
                else if (mCurrent>getModified())
                    mCurrent = getModified();
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
}

#endif
