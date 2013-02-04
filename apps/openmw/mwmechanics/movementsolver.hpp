#ifndef  GAME_MWMECHANICS_MOVEMENTSOLVER_H
#define  GAME_MWMECHANICS_MOVEMENTSOLVER_H

#include <OgreVector3.h>

namespace MWWorld
{
    class Ptr;
}

namespace OEngine
{
    namespace Physic
    {
        class PhysicEngine;
        class PhysicActor;
    }
}

namespace MWMechanics
{
    class MovementSolver
    {
        public:
            MovementSolver();
            virtual ~MovementSolver();

            Ogre::Vector3 move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time, const Ogre::Vector3 &halfExtents);

        private:
            bool stepMove(Ogre::Vector3& position, const Ogre::Vector3 &velocity, float remainingTime, float verticalRotation, const Ogre::Vector3 &halfExtents, bool isInterior);

            void clipVelocity(const Ogre::Vector3& in, const Ogre::Vector3& normal, Ogre::Vector3& out, const float overbounce);
            void projectVelocity(Ogre::Vector3& velocity, const Ogre::Vector3& direction);

            float getSlope(const Ogre::Vector3 &normal);

            OEngine::Physic::PhysicEngine *mEngine;
            OEngine::Physic::PhysicActor *mPhysicActor;

            float verticalVelocity;
    };
}

#endif /* GAME_MWMECHANICS_MOVEMENTSOLVER_H */
