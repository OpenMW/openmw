#ifndef OPENMW_COMPONENTS_SCENEUTIL_OSGACONTROLLER_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_OSGACONTROLLER_HPP

#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/StateSet>
#include <osg/ref_ptr>
#include <osgAnimation/Animation>
#include <osgAnimation/AnimationUpdateCallback>
#include <osgAnimation/Channel>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/StackedTransform>
#include <osgAnimation/UpdateMatrixTransform>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/keyframe.hpp>
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

            virtual void handle_stateset(osg::StateSet* stateset);

            virtual void setAnimation(Resource::Animation* animation);

            virtual void apply(osg::Node& node) override;

            virtual void apply(osg::Geode& node) override;

        protected:
            Resource::Animation* mAnimation;
    };

    class OsgAnimationController : public SceneUtil::KeyframeController
    {
    public:
        /// @brief Handles the animation for osgAnimation formats
        OsgAnimationController() {};

        OsgAnimationController(const OsgAnimationController& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, OsgAnimationController)

        /// @brief Handles the location of the instance
        osg::Vec3f getTranslation(float time) const override;

        /// @brief Calls animation track update()
        void update(float time, const std::string& animationName);

        /// @brief Called every frame for osgAnimation
        void operator() (osg::Node*, osg::NodeVisitor*) override;

        /// @brief Sets details of the animations
        void setEmulatedAnimations(const std::vector<EmulatedAnimation>& emulatedAnimations);

        /// @brief Adds an animation track to a model
        void addMergedAnimationTrack(osg::ref_ptr<Resource::Animation> animationTrack);

    private:
        bool mNeedToLink = true;
        osg::ref_ptr<LinkVisitor> mLinker;
        std::vector<osg::ref_ptr<Resource::Animation>> mMergedAnimationTracks; // Used only by osgAnimation-based formats (e.g. dae)
        std::vector<EmulatedAnimation> mEmulatedAnimations;
    };
}

#endif
