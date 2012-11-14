#ifndef AIESCORT_H
#define AIESCORT_H

#include "aipackage.hpp"
#include <iostream>
#include <vector>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{
class AiEscort : public AiPackage
{
    public:
     AiEscort(std::string ActorID,int Duration, float X, float Y, float Z, bool Reset = false);
    virtual AiEscort *clone() const;

    virtual bool execute (const MWWorld::Ptr& actor);
            ///< \return Package completed?

    virtual int getTypeId() const;
    float getX();
    float getY();
    float getZ();
    bool getReset();
    std::string getActorID();
    int getDuration();

    private:
    std::string mActorID;
    float mX;
    float mY;
    float mZ;
    int mDuration;
    bool mReset;
};
}
#endif // AIESCORT_H
