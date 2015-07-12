#ifndef OPENMW_MWRENDER_RIPPLESIMULATION_H
#define OPENMW_MWRENDER_RIPPLESIMULATION_H

#include <osg/ref_ptr>

#include "../mwworld/ptr.hpp"

namespace osg
{
    class Group;
}

namespace osgParticle
{
    class ParticleSystem;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWWorld
{
    class Fallback;
}

namespace MWRender
{

    struct Emitter
    {
        MWWorld::Ptr mPtr;
        osg::Vec3f mLastEmitPosition;
        float mScale;
        float mForce;
    };

    class RippleSimulation
    {
    public:
        RippleSimulation(osg::Group* parent, Resource::ResourceSystem* resourceSystem, const MWWorld::Fallback* fallback);
        ~RippleSimulation();

        /// @param dt Time since the last frame
        void update(float dt);

        /// adds an emitter, position will be tracked automatically
        void addEmitter (const MWWorld::Ptr& ptr, float scale = 1.f, float force = 1.f);
        void removeEmitter (const MWWorld::Ptr& ptr);
        void updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);
        void removeCell(const MWWorld::CellStore* store);

        /// Change the height of the water surface, thus moving all ripples with it
        void setWaterHeight(float height);

        /// Remove all active ripples
        void clear();

    private:
        osg::ref_ptr<osg::Group> mParent;

        osg::ref_ptr<osgParticle::ParticleSystem> mParticleSystem;
        osg::ref_ptr<osg::PositionAttitudeTransform> mParticleNode;

        std::vector<Emitter> mEmitters;
    };

}

#endif
