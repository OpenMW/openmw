#ifndef OPENMW_MWRENDER_WATER_H
#define OPENMW_MWRENDER_WATER_H

#include <memory>
#include <vector>

#include <osg/ref_ptr>
#include <osg/Vec3f>
#include <osg/Uniform>
#include <osg/Camera>

#include <components/settings/settings.hpp>

namespace osg
{
    class Group;
    class PositionAttitudeTransform;
    class Geometry;
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
    class CellStore;
    class Ptr;
}

namespace Fallback
{
    class Map;
}

namespace MWRender
{

    class Refraction;
    class Reflection;
    class RippleSimulation;

    /// Water rendering
    class Water
    {
        osg::ref_ptr<osg::Uniform> mRainIntensityUniform;

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Group> mSceneRoot;
        osg::ref_ptr<osg::PositionAttitudeTransform> mWaterNode;
        osg::ref_ptr<osg::Geometry> mWaterGeom;
        Resource::ResourceSystem* mResourceSystem;
        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;

        std::unique_ptr<RippleSimulation> mSimulation;

        osg::ref_ptr<Refraction> mRefraction;
        osg::ref_ptr<Reflection> mReflection;

        const std::string mResourcePath;

        bool mEnabled;
        bool mToggled;
        float mTop;
        bool mInterior;

        osg::Callback* mCullCallback;

        osg::Vec3f getSceneNodeCoordinates(int gridX, int gridY);
        void updateVisible();

        void createSimpleWaterStateSet(osg::Node* node, float alpha);

        /// @param reflection the reflection camera (required)
        /// @param refraction the refraction camera (optional)
        void createShaderWaterStateSet(osg::Node* node, Reflection* reflection, Refraction* refraction);

        void updateWaterMaterial();

    public:
        Water(osg::Group* parent, osg::Group* sceneRoot,
              Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
              const std::string& resourcePath);
        ~Water();

        void setCullCallback(osg::Callback* callback);

        void listAssetsToPreload(std::vector<std::string>& textures);

        void setEnabled(bool enabled);

        bool toggle();

        bool isUnderwater(const osg::Vec3f& pos) const;

        /// adds an emitter, position will be tracked automatically using its scene node
        void addEmitter (const MWWorld::Ptr& ptr, float scale = 1.f, float force = 1.f);
        void removeEmitter (const MWWorld::Ptr& ptr);
        void updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr);
        void emitRipple(const osg::Vec3f& pos);

        void removeCell(const MWWorld::CellStore* store); ///< remove all emitters in this cell

        void clearRipples();

        void changeCell(const MWWorld::CellStore* store);
        void setHeight(const float height);

        void update(float dt);

        osg::Camera *getReflectionCamera();
        osg::Camera *getRefractionCamera();

        void processChangedSettings(const Settings::CategorySettingVector& settings);

        osg::Uniform *getRainIntensityUniform();
    };

}

#endif
