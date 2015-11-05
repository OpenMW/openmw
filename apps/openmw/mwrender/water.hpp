#ifndef OPENMW_MWRENDER_WATER_H
#define OPENMW_MWRENDER_WATER_H

#include <memory>

#include <osg/ref_ptr>
#include <osg/Vec3f>

#include <components/settings/settings.hpp>

namespace osg
{
    class Group;
    class PositionAttitudeTransform;
    class Geode;
    class Node;
}

namespace osgUtil
{
    class IncrementalCompileOperation;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWWorld
{
    class Fallback;
    class CellStore;
    class Ptr;
}

namespace MWRender
{

    class Refraction;
    class Reflection;
    class RippleSimulation;

    /// Water rendering
    class Water
    {
        static const int CELL_SIZE = 8192;

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Group> mSceneRoot;
        osg::ref_ptr<osg::PositionAttitudeTransform> mWaterNode;
        osg::ref_ptr<osg::Geode> mWaterGeode;
        Resource::ResourceSystem* mResourceSystem;
        const MWWorld::Fallback* mFallback;
        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;

        std::auto_ptr<RippleSimulation> mSimulation;

        osg::ref_ptr<Refraction> mRefraction;
        osg::ref_ptr<Reflection> mReflection;

        const std::string mResourcePath;

        bool mEnabled;
        bool mToggled;
        float mTop;

        osg::Vec3f getSceneNodeCoordinates(int gridX, int gridY);
        void updateVisible();

        void createSimpleWaterStateSet(osg::Node* node, float alpha);

        /// @param reflection the reflection camera (required)
        /// @param refraction the refraction camera (optional)
        void createShaderWaterStateSet(osg::Node* node, Reflection* reflection, Refraction* refraction);

        void updateWaterMaterial();

    public:
        Water(osg::Group* parent, osg::Group* sceneRoot,
              Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico, const MWWorld::Fallback* fallback,
              const std::string& resourcePath);
        ~Water();

        void setEnabled(bool enabled);

        bool toggle();

        bool isUnderwater(const osg::Vec3f& pos) const;

        /// adds an emitter, position will be tracked automatically using its scene node
        void addEmitter (const MWWorld::Ptr& ptr, float scale = 1.f, float force = 1.f);
        void removeEmitter (const MWWorld::Ptr& ptr);
        void updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);
        void removeCell(const MWWorld::CellStore* store); ///< remove all emitters in this cell

        void clearRipples();

        void changeCell(const MWWorld::CellStore* store);
        void setHeight(const float height);

        void update(float dt);

        void processChangedSettings(const Settings::CategorySettingVector& settings);
    };

}

#endif
