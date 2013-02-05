#include "movementsolver.hpp"

#include "libs/openengine/bullet/trace.h"
#include "libs/openengine/bullet/physic.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include <cmath>


namespace MWMechanics
{

MovementSolver::MovementSolver()
  : mEngine(MWBase::Environment::get().getWorld()->getPhysicEngine())
{
}

MovementSolver::~MovementSolver()
{
    // nothing to do
}

void MovementSolver::clipVelocity(const Ogre::Vector3& in, const Ogre::Vector3& normal, Ogre::Vector3& out, const float overbounce)
{
    //Math stuff. Basically just project the velocity vector onto the plane represented by the normal.
    //More specifically, it projects velocity onto the normal, takes that result, multiplies it by overbounce and then subtracts it from velocity.
    float backoff;

    backoff = in.dotProduct(normal);
    if(backoff < 0.0f)
        backoff *= overbounce;
    else
        backoff /= overbounce;

    out = in - (normal*backoff);
}

void MovementSolver::projectVelocity(Ogre::Vector3& velocity, const Ogre::Vector3& direction)
{
    Ogre::Vector3 normalizedDirection(direction);
    normalizedDirection.normalise();

    // no divide by normalizedDirection.length necessary because it's normalized
    velocity = normalizedDirection * velocity.dotProduct(normalizedDirection);
}

bool MovementSolver::stepMove(Ogre::Vector3& position, const Ogre::Vector3 &velocity, float remainingTime, float verticalRotation, const Ogre::Vector3 &halfExtents, bool isInterior)
{
    static const float maxslope = 45.0f;
    traceResults trace; // no initialization needed

    newtrace(&trace, position+Ogre::Vector3(0.0f,0.0f,STEPSIZE),
                     position+Ogre::Vector3(0.0f,0.0f,STEPSIZE)+velocity*remainingTime,
             halfExtents, verticalRotation, isInterior, mEngine);
    if(trace.fraction == 0.0f || (trace.fraction != 1.0f && getSlope(trace.planenormal) > maxslope))
        return false;

    newtrace(&trace, trace.endpos, trace.endpos-Ogre::Vector3(0,0,STEPSIZE), halfExtents, verticalRotation, isInterior, mEngine);
    if(getSlope(trace.planenormal) < maxslope)
    {
        // only step down onto semi-horizontal surfaces. don't step down onto the side of a house or a wall.
        position = trace.endpos;
        return true;
    }

    return false;
}

float MovementSolver::getSlope(const Ogre::Vector3 &normal)
{
    return normal.angleBetween(Ogre::Vector3(0.0f,0.0f,1.0f)).valueDegrees();
}


Ogre::Vector3 MovementSolver::move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time)
{
    Ogre::Vector3 position(ptr.getRefData().getPosition().pos);

    /* Anything to collide with? */
    mPhysicActor = mEngine->getCharacter(ptr.getRefData().getHandle());
    if(!mPhysicActor || !mPhysicActor->getCollisionMode())
        return position + movement;

    traceResults trace; //no initialization needed
    int iterations=0, maxIterations=50; //arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    float maxslope=45;

    float verticalVelocity = mPhysicActor->getVerticalForce();
    Ogre::Vector3 horizontalVelocity = movement/time;
    Ogre::Vector3 velocity(horizontalVelocity.x, horizontalVelocity.y, verticalVelocity); // we need a copy of the velocity before we start clipping it for steps
    Ogre::Vector3 clippedVelocity(horizontalVelocity.x, horizontalVelocity.y, verticalVelocity);

    float remainingTime = time;
    bool isInterior = !ptr.getCell()->isExterior();
    float verticalRotation = mPhysicActor->getRotation().getYaw().valueDegrees();
    Ogre::Vector3 halfExtents = mPhysicActor->getHalfExtents();

    Ogre::Vector3 lastNormal(0.0f);
    Ogre::Vector3 currentNormal(0.0f);
    Ogre::Vector3 up(0.0f, 0.0f, 1.0f);
    Ogre::Vector3 newPosition = position;

    newtrace(&trace, position, position+Ogre::Vector3(0,0,-10), halfExtents, verticalRotation, isInterior, mEngine);
    if(trace.fraction < 1.0f)
    {
        if(getSlope(trace.planenormal) > maxslope)
        {
            // if we're on a really steep slope, don't listen to user input
            clippedVelocity.x = clippedVelocity.y = 0.0f;
        }
        else
        {
            // if we're within 10 units of the ground, force velocity to track the ground
            clipVelocity(clippedVelocity, trace.planenormal, clippedVelocity, 1.0f);
        }
    }

    do {
        // trace to where character would go if there were no obstructions
        newtrace(&trace, newPosition, newPosition+clippedVelocity*remainingTime, halfExtents, verticalRotation, isInterior, mEngine);
        newPosition = trace.endpos;
        currentNormal = trace.planenormal;
        remainingTime = remainingTime * (1.0f-trace.fraction);

        // check for obstructions
        if(trace.fraction != 1.0f)
        {
            //std::cout<<"angle: "<<getSlope(trace.planenormal)<<"\n";
            if(getSlope(currentNormal) > maxslope || currentNormal == lastNormal)
            {
                if(stepMove(newPosition, velocity, remainingTime, verticalRotation, halfExtents, mEngine))
                    std::cout<< "stepped" <<std::endl;
                else
                {
                    Ogre::Vector3 resultantDirection = currentNormal.crossProduct(up);
                    resultantDirection.normalise();
                    clippedVelocity = velocity;
                    projectVelocity(clippedVelocity, resultantDirection);

                    // just this isn't enough sometimes. It's the same problem that causes steps to be necessary on even uphill terrain.
                    clippedVelocity += currentNormal*clippedVelocity.length()/50.0f;
                    std::cout<< "clipped velocity: "<<clippedVelocity <<std::endl;
                }
            }
            else
                clipVelocity(clippedVelocity, currentNormal, clippedVelocity, 1.0f);
        }

        lastNormal = currentNormal;

        iterations++;
    } while(iterations < maxIterations && remainingTime != 0.0f);

    verticalVelocity = clippedVelocity.z;
    verticalVelocity -= time*400;
    mPhysicActor->setVerticalForce(verticalVelocity);

    return newPosition;
}

}
