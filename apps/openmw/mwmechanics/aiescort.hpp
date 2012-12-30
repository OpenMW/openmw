#ifndef GAME_MWMECHANICS_AIESCORT_H
#define GAME_MWMECHANICS_AIESCORT_H

#include "aipackage.hpp"
#include <string>

namespace MWMechanics
{
    class AiEscort : public AiPackage
    {
        public:
            AiEscort(const std::string &actorId,int duration, float x, float y, float z);
            virtual AiEscort *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor);
            ///< \return Package completed?

            virtual int getTypeId() const;

        private:
            std::string mActorId;
            float mX;
            float mY;
            float mZ;
            int mDuration;

    };
}
#endif
