#ifndef GAME_MWMECHANICS_AIACTIVATE_H
#define GAME_MWMECHANICS_AIACTIVATE_H

#include "aipackage.hpp"
#include <string>

namespace MWMechanics
{

    class AiActivate : AiPackage
    {
        public:
            AiActivate(const std::string &objectID);
            virtual AiActivate *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor);
                    ///< \return Package completed?
            virtual int getTypeId() const;

        private:
            std::string mObjectID;
    };
}
#endif // GAME_MWMECHANICS_AIACTIVATE_H
