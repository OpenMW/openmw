#ifndef GAME_MWMECHANICS_AIACTIVATE_H
#define GAME_MWMECHANICS_AIACTIVATE_H

#include "aipackage.hpp"
#include <string>

namespace MWMechanics
{

    class AiActivate : public AiPackage
    {
        public:
            AiActivate(const std::string &objectId);
            virtual AiActivate *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor);
                    ///< \return Package completed?
            virtual int getTypeId() const;

        private:
            std::string mObjectId;
    };
}
#endif // GAME_MWMECHANICS_AIACTIVATE_H
