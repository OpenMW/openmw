#ifndef AIFALLOW_H
#define AIFALLOW_H

#include "aipackage.hpp"
#include <string>
#include <iostream>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{

class AiFallow : AiPackage
{
    public:
    AiFallow(std::string ActorID,float duration, float X, float Y, float Z, bool Reset = false);
    virtual AiFallow *clone() const;

    virtual bool execute (const MWWorld::Ptr& actor);
            ///< \return Package completed?

    virtual int getTypeId() const;
            ///< 0: Wanter, 1 Travel, 2 Escort, 3 Follo
    private:
    float mDuration;
    float mX;
    float mY;
    float mZ;
    bool mReset;
    std::string mActorID;
};
}
#endif // AIFALLOW_H
