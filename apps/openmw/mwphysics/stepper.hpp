#ifndef OPENMW_MWPHYSICS_STEPPER_H
#define OPENMW_MWPHYSICS_STEPPER_H

#include "trace.h"

class btCollisionObject;
class btCollisionWorld;

namespace osg
{
    class Vec3f;
}

namespace MWPhysics
{
    class Stepper
    {
    private:
        const btCollisionWorld *mColWorld;
        const btCollisionObject *mColObj;

        ActorTracer mTracer, mUpStepper, mDownStepper;
        bool mHaveMoved;

    public:
        Stepper(const btCollisionWorld *colWorld, const btCollisionObject *colObj);

        bool step(osg::Vec3f &position, const osg::Vec3f &toMove, float &remainingTime);
    };
}

#endif
