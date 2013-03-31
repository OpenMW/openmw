#ifndef GAME_MWMECHANICS_AITRAVEL_H
#define GAME_MWMECHANICS_AITRAVEL_H

#include "aipackage.hpp"
#include <components/esm/loadpgrd.hpp>
#include <list>

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

        private:
            float mX;
            float mY;
            float mZ;

            int cellX;
            int cellY;

            bool isPathConstructed;
            std::list<ESM::Pathgrid::Point> mPath;

    };
}

#endif
