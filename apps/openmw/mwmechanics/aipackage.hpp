#ifndef GAME_MWMECHANICS_AIPACKAGE_H
#define GAME_MWMECHANICS_AIPACKAGE_H

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    /// \brief Base class for AI packages
    class AiPackage
    {
        public:
    
            virtual ~AiPackage();
      
            virtual AiPackage *clone() const = 0;
            
            virtual bool execute (const MWWorld::Ptr& actor,float duration) = 0;
            ///< \return Package completed?
            
            virtual int getTypeId() const = 0;
            ///< 0: Wanter, 1 Travel, 2 Escort, 3 Follow, 4 Activate

            virtual unsigned int getPriority() const {return 0;}
            ///< higher number is higher priority (0 beeing the lowest)
    };
}

#endif

