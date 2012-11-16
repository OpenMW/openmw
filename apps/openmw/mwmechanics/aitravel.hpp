#ifndef GAME_MWMECHANICS_AITRAVEL_H
#define GAME_MWMECHANICS_AITRAVEL_H

#include "aipackage.hpp"

namespace MWMechanics
{
    class AiTravel : public AiPackage
    {
        public:
            AiTravel(float x, float y, float z);
            virtual AiTravel *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor);
                    ///< \return Package completed?

            virtual int getTypeId() const;
            float getX();
            float getY();
            float getZ();


        private:
            float mX;
            float mY;
            float mZ;

    };
}

#endif
