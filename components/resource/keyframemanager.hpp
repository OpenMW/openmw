#ifndef OPENMW_COMPONENTS_KEYFRAMEMANAGER_H
#define OPENMW_COMPONENTS_KEYFRAMEMANAGER_H

#include <osg/ref_ptr>
#include <osgAnimation/BasicAnimationManager>
#include <string>

#include <components/sceneutil/keyframe.hpp>

#include "resourcemanager.hpp"

namespace Resource
{
    /// @brief extract animations to OpenMW's animation system
    class RetrieveAnimationsVisitor : public osg::NodeVisitor
    {
        public:
            RetrieveAnimationsVisitor(SceneUtil::KeyframeHolder& target, osg::ref_ptr<osgAnimation::BasicAnimationManager> animationManager,
                const std::string& normalized, const VFS::Manager* vfs);

            virtual void apply(osg::Node& node) override;

        private:

            std::string changeFileExtension(const std::string& file, const std::string& ext);
            std::string parseTextKey(const std::string& line);
            double parseTimeSignature(const std::string& line);

            SceneUtil::KeyframeHolder& mTarget;
            osg::ref_ptr<osgAnimation::BasicAnimationManager> mAnimationManager;
            std::string mNormalized;
            const VFS::Manager* mVFS;

    };
}

namespace Resource
{

    class SceneManager;

    /// @brief Managing of keyframe resources
    /// @note May be used from any thread.
    class KeyframeManager : public ResourceManager
    {
    public:
        KeyframeManager(const VFS::Manager* vfs, SceneManager* sceneManager);
        ~KeyframeManager();

        /// Retrieve a read-only keyframe resource by name (case-insensitive).
        /// @note Throws an exception if the resource is not found.
        osg::ref_ptr<const SceneUtil::KeyframeHolder> get(const std::string& name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;
    private:
        SceneManager* mSceneManager;
    };

}

#endif
