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
            enum TypeId {
                TypeIdNone = -1,
                TypeIdWander = 0,
                TypeIdTravel = 1,
                TypeIdEscort = 2,
                TypeIdFollow = 3,
                TypeIdActivate = 4,
                TypeIdCombat = 5
            };

            virtual ~AiPackage();
      
            virtual AiPackage *clone() const = 0;
            
            virtual bool execute (const MWWorld::Ptr& actor,float duration) = 0;
            ///< \return Package completed?
            
            virtual int getTypeId() const = 0;
            ///< @see enum TypeId

            virtual unsigned int getPriority() const {return 0;}
            ///< higher number is higher priority (0 beeing the lowest)
    };
}

#endif

