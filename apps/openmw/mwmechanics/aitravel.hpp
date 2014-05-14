#ifndef GAME_MWMECHANICS_AITRAVEL_H
#define GAME_MWMECHANICS_AITRAVEL_H

#include "aipackage.hpp"

#include "pathfinding.hpp"

namespace MWMechanics
{
    /// \brief Causes the AI to travel to the specified point
    class AiTravel : public AiPackage
    {
        public:
            /// Default constructor
            AiTravel(float x, float y, float z);
            virtual AiTravel *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor,float duration);

            virtual int getTypeId() const;

        private:
            float mX;
            float mY;
            float mZ;

            int mCellX;
            int mCellY;

            PathFinder mPathFinder;
    };
}

#endif
