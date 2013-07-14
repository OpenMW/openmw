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
            
            virtual bool execute (const MWWorld::Ptr& actor) = 0;
            ///< \return Package completed?
            
            virtual int getTypeId() const = 0;
            ///< 0: Wanter, 1 Travel, 2 Escort, 3 Follow, 4 Activate
    };
}

#endif

