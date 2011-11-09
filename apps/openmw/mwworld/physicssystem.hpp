#ifndef GAME_MWWORLD_PHYSICSSYSTEM_H
#define GAME_MWWORLD_PHYSICSSYSTEM_H

#include <vector>
#include <openengine/ogre/renderer.hpp>
#include <openengine/bullet/physic.hpp>

namespace MWWorld
{

    class PhysicsSystem
    {
        public:
            PhysicsSystem (OEngine::Render::OgreRenderer &_rend , OEngine::Physic::PhysicEngine* physEng);
            ~PhysicsSystem ();

            std::vector< std::pair<std::string, Ogre::Vector3> > doPhysics (float duration,
                const std::vector<std::pair<std::string, Ogre::Vector3> >& actors);

            void addObject (const std::string& handle, const std::string& mesh,
                const Ogre::Quaternion& rotation, float scale, const Ogre::Vector3& position);

            void addActor (const std::string& handle, const std::string& mesh,
                const Ogre::Vector3& position);

            void removeObject (const std::string& handle);

            void moveObject (const std::string& handle, const Ogre::Vector3& position);

            void rotateObject (const std::string& handle, const Ogre::Quaternion& rotation);

            void scaleObject (const std::string& handle, float scale);

            bool toggleCollisionMode();
			 std::pair<std::string, float> getFacedHandle (MWWorld::World& world);

            void insertObjectPhysics(const std::string& handle);

              void insertActorPhysics(const std::string& handle);

        private:
            OEngine::Render::OgreRenderer &mRender;
            OEngine::Physic::PhysicEngine* mEngine;
            bool mFreeFly;

            PhysicsSystem (const PhysicsSystem&);
            PhysicsSystem& operator= (const PhysicsSystem&);
    };

}

#endif
