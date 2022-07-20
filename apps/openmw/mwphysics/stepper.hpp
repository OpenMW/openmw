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

    public:
        Stepper(const btCollisionWorld *colWorld, const btCollisionObject *colObj);

        bool step(osg::Vec3f &position, osg::Vec3f &velocity, float &remainingTime, const bool & onGround, bool firstIteration);
    };
}

#endif
