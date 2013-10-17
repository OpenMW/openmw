#ifndef GAME_MWMECHANICS_AIFALLOW_H
#define GAME_MWMECHANICS_AIFALLOW_H

#include "aipackage.hpp"
#include <string>

namespace MWMechanics
{

    class AiFollow : public AiPackage
    {
        public:
            AiFollow(const std::string &ActorId,float duration, float X, float Y, float Z);
            AiFollow(const std::string &ActorId,const std::string &CellId,float duration, float X, float Y, float Z);
            virtual AiFollow *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor);
                    ///< \return Package completed?
            virtual int getTypeId() const;

        private:
            float mDuration;
            float mX;
            float mY;
            float mZ;
            std::string mActorId;
            std::string mCellId;
    };
}
#endif
