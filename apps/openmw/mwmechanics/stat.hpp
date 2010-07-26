#ifndef GAME_MWMECHANICS_STAT_H
#define GAME_MWMECHANICS_STAT_H

#include <limits>

namespace MWMechanics
{
    template<typename T>
    class Stat
    {
            T mBase;
            T mModified;
    
        public:
        
            Stat() : mBase (0), mModified (0) {}
        
            const T& getBase() const
            {
                return mBase;
            }
            
            const T& getModified() const
            {
                return mModified;
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
                else if (mBase>max-diff)
                {
                    value = max + (mModified - mBase);
                    diff = value - mModified;
                }
                
                mModified = value;
                mBase += diff;
            }
            
            /// Change modified relatively.
            void modify (const T& diff)
            {
                mModified += diff;            
            }
    };
}

#endif

