#ifndef GAME_MWMECHANICS_STAT_H
#define GAME_MWMECHANICS_STAT_H

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
            
            /// Change modified relatively.
            void modify (const T& diff)
            {
                mModified += diff;            
            }
    };
}

#endif

