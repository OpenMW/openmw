#ifndef OPENMW_COMPONENTS_KEYFRAMEMANAGER_H
#define OPENMW_COMPONENTS_KEYFRAMEMANAGER_H

#include <osg/ref_ptr>
#include <osgAnimation/BasicAnimationManager>
#include <string>

#include <components/sceneutil/keyframe.hpp>

#include "resourcemanager.hpp"

namespace ToUTF8
{
    class StatelessUtf8Encoder;
}

namespace Resource
{
    /// @brief extract animations from OSG formats to OpenMW's animation system
    class RetrieveAnimationsVisitor : public osg::NodeVisitor
    {
    public:
        RetrieveAnimationsVisitor(SceneUtil::KeyframeHolder& target,
            osg::ref_ptr<osgAnimation::BasicAnimationManager> animationManager, const std::string& normalized,
            const VFS::Manager* vfs);

        bool belongsToLeftUpperExtremity(const std::string& name);
        bool belongsToRightUpperExtremity(const std::string& name);
        bool belongsToTorso(const std::string& name);

        void addKeyframeController(const std::string& name, const osg::Node& node);
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
        explicit KeyframeManager(const VFS::Manager* vfs, SceneManager* sceneManager, double expiryDelay,
            const ToUTF8::StatelessUtf8Encoder* encoder);
        ~KeyframeManager() = default;

        /// Retrieve a read-only keyframe resource by name (case-insensitive).
        /// @note Throws an exception if the resource is not found.
        osg::ref_ptr<const SceneUtil::KeyframeHolder> get(const std::string& name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        SceneManager* mSceneManager;
        const ToUTF8::StatelessUtf8Encoder* mEncoder;
    };

}

#endif
