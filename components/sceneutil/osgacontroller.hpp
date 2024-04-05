#ifndef OPENMW_COMPONENTS_SCENEUTIL_OSGACONTROLLER_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_OSGACONTROLLER_HPP

#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/ref_ptr>
#include <osgAnimation/UpdateMatrixTransform>

#include "keyframe.hpp"
#include "nodecallback.hpp"

#include <components/resource/animation.hpp>

namespace SceneUtil
{
    struct EmulatedAnimation
    {
        float mStartTime;
        float mStopTime;
        std::string mName;
    };

    class LinkVisitor : public osg::NodeVisitor
    {
    public:
        LinkVisitor();

        virtual void link(osgAnimation::UpdateMatrixTransform* umt);

        virtual void setAnimation(Resource::Animation* animation);

        virtual void apply(osg::Node& node) override;

    protected:
        Resource::Animation* mAnimation;
    };

#ifdef _MSC_VER
#pragma warning(push)
/*
 * Warning C4250: 'SceneUtil::OsgAnimationController': inherits 'osg::Callback::osg::Callback::asCallback' via
 * dominance, there is no way to solved this if an object must inherit from both osg::Object and osg::Callback
 */
#pragma warning(disable : 4250)
#endif
    class OsgAnimationController : public SceneUtil::KeyframeController,
                                   public SceneUtil::NodeCallback<OsgAnimationController>
    {
    public:
        /// @brief Handles the animation for osgAnimation formats
        OsgAnimationController() = default;

        OsgAnimationController(const OsgAnimationController& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, OsgAnimationController)

        osg::Callback* getAsCallback() override { return this; }

        /// @brief Handles the location of the instance
        osg::Vec3f getTranslation(float time) const override;

        /// @brief Handles finding bone position in the animation
        osg::Matrixf getTransformForNode(float time, const std::string_view name) const;

        /// @brief Calls animation track update()
        void update(float time, const std::string& animationName);

        /// @brief Called every frame for osgAnimation
        void operator()(osg::Node*, osg::NodeVisitor*);

        /// @brief Sets details of the animations
        void setEmulatedAnimations(const std::vector<EmulatedAnimation>& emulatedAnimations);

        /// @brief Adds an animation track to a model
        void addMergedAnimationTrack(osg::ref_ptr<Resource::Animation> animationTrack);

    private:
        bool mNeedToLink = true;
        osg::ref_ptr<LinkVisitor> mLinker;
        std::vector<osg::ref_ptr<Resource::Animation>>
            mMergedAnimationTracks; // Used only by osgAnimation-based formats (e.g. dae)
        std::vector<EmulatedAnimation> mEmulatedAnimations;
    };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

#endif
