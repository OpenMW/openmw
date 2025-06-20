#ifndef OPENMW_MWRENDER_EFFECTMANAGER_H
#define OPENMW_MWRENDER_EFFECTMANAGER_H

#include <memory>
#include <vector>

#include <osg/ref_ptr>

#include <components/vfs/pathutil.hpp>

namespace osg
{
    class Group;
    class Vec3f;
    class PositionAttitudeTransform;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    class EffectAnimationTime;

    // Note: effects attached to another object should be managed by MWRender::Animation::addEffect.
    // This class manages "free" effects, i.e. attached to a dedicated scene node in the world.
    class EffectManager
    {
    public:
        EffectManager(osg::ref_ptr<osg::Group> parent, Resource::ResourceSystem* resourceSystem);
        EffectManager(const EffectManager&) = delete;
        ~EffectManager();

        /// Add an effect. When it's finished playing, it will be removed automatically.
        void addEffect(VFS::Path::NormalizedView model, std::string_view textureOverride,
            const osg::Vec3f& worldPosition, float scale, bool isMagicVFX = true, bool useAmbientLight = true,
            std::string_view effectId = {}, bool loop = false);

        void removeEffect(std::string_view effectId);

        void update(float dt);

        /// Remove all effects
        void clear();

    private:
        struct Effect
        {
            std::string mEffectId;
            float mMaxControllerLength;
            bool mLoop;
            std::shared_ptr<EffectAnimationTime> mAnimTime;
            osg::ref_ptr<osg::PositionAttitudeTransform> mTransform;
        };

        std::mutex mEffectsMutex;
        std::vector<Effect> mEffects;

        osg::ref_ptr<osg::Group> mParentNode;
        Resource::ResourceSystem* mResourceSystem;
    };

}

#endif
