#ifndef OPENMW_MWPHYSICS_PTRHOLDER_H
#define OPENMW_MWPHYSICS_PTRHOLDER_H

#include <list>
#include <memory>
#include <mutex>
#include <utility>

#include <osg/Vec3d>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "../mwworld/ptr.hpp"

namespace MWPhysics
{
    struct Movement
    {
        osg::Vec3f mVelocity = osg::Vec3f();
        float mSimulationTimeStart = 0.f; // The time at which this movement begun
        float mSimulationTimeStop = 0.f; // The time at which this movement finished
        bool mJump = false;
    };

    class PtrHolder
    {
    public:
        explicit PtrHolder(const MWWorld::Ptr& ptr, const osg::Vec3f& position)
            : mPtr(ptr)
            , mSimulationPosition(position)
            , mPosition(position)
            , mPreviousPosition(position)
        {
        }

        virtual ~PtrHolder() = default;

        void updatePtr(const MWWorld::Ptr& updated) { mPtr = updated; }

        MWWorld::Ptr getPtr() const { return mPtr; }

        btCollisionObject* getCollisionObject() const { return mCollisionObject.get(); }

        void clearMovement() { mMovement = {}; }
        void queueMovement(osg::Vec3f velocity, float simulationTimeStart, float simulationTimeStop, bool jump = false)
        {
            mMovement.push_back(Movement{ velocity, simulationTimeStart, simulationTimeStop, jump });
        }

        void eraseMovementIf(const auto& predicate) { std::erase_if(mMovement, predicate); }

        void setSimulationPosition(const osg::Vec3f& position) { mSimulationPosition = position; }

        osg::Vec3f getSimulationPosition() const { return mSimulationPosition; }

        void setPosition(const osg::Vec3f& position)
        {
            mPreviousPosition = mPosition;
            mPosition = position;
        }

        osg::Vec3d getPosition() const { return mPosition; }

        osg::Vec3d getPreviousPosition() const { return mPreviousPosition; }

    protected:
        MWWorld::Ptr mPtr;
        std::unique_ptr<btCollisionObject> mCollisionObject;
        std::list<Movement> mMovement;
        osg::Vec3f mSimulationPosition;
        osg::Vec3d mPosition;
        osg::Vec3d mPreviousPosition;
    };
}

#endif
