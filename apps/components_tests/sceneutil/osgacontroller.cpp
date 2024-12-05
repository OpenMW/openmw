#include <components/sceneutil/osgacontroller.hpp>

#include <gtest/gtest.h>
#include <osgAnimation/Channel>

#include <filesystem>
#include <fstream>

namespace
{
    using namespace SceneUtil;

    static const std::string ROOT_BONE_NAME = "bip01";

    // Creates a merged anim track with a single root channel with two start/end matrix transforms
    osg::ref_ptr<Resource::Animation> createMergedAnimationTrack(std::string name, osg::Matrixf startTransform,
        osg::Matrixf endTransform, float startTime = 0.0f, float endTime = 1.0f)
    {
        osg::ref_ptr<Resource::Animation> mergedAnimationTrack = new Resource::Animation;
        mergedAnimationTrack->setName(name);

        osgAnimation::MatrixKeyframeContainer* cbCntr = new osgAnimation::MatrixKeyframeContainer;
        cbCntr->push_back(osgAnimation::MatrixKeyframe(startTime, startTransform));
        cbCntr->push_back(osgAnimation::MatrixKeyframe(endTime, endTransform));

        osg::ref_ptr<osgAnimation::MatrixLinearChannel> rootChannel = new osgAnimation::MatrixLinearChannel;
        rootChannel->setName("transform");
        rootChannel->setTargetName(ROOT_BONE_NAME);
        rootChannel->getOrCreateSampler()->setKeyframeContainer(cbCntr);
        mergedAnimationTrack->addChannel(rootChannel);
        return mergedAnimationTrack;
    }

    TEST(OsgAnimationControllerTest, getTranslationShouldReturnSampledChannelTranslationForBip01)
    {
        std::vector<EmulatedAnimation> emulatedAnimations;
        emulatedAnimations.push_back({ 0.0f, 1.0f, "test1" }); // should sample this
        emulatedAnimations.push_back({ 1.1f, 2.0f, "test2" }); // should ignore this

        OsgAnimationController controller;
        controller.setEmulatedAnimations(emulatedAnimations);

        osg::Matrixf startTransform = osg::Matrixf::identity();
        osg::Matrixf endTransform = osg::Matrixf::identity();
        osg::Matrixf endTransform2 = osg::Matrixf::identity();
        endTransform.setTrans(1.0f, 1.0f, 1.0f);
        controller.addMergedAnimationTrack(createMergedAnimationTrack("test1", startTransform, endTransform));
        endTransform2.setTrans(2.0f, 2.0f, 2.0f);
        controller.addMergedAnimationTrack(
            createMergedAnimationTrack("test2", endTransform, endTransform2, 0.1f, 0.9f));

        // should be halfway between 0,0,0 and 1,1,1
        osg::Vec3f translation = controller.getTranslation(0.5f);
        EXPECT_EQ(translation, osg::Vec3f(0.5f, 0.5f, 0.5f));
    }

    TEST(OsgAnimationControllerTest, getTranslationShouldReturnZeroVectorIfNotFound)
    {
        std::vector<EmulatedAnimation> emulatedAnimations;
        emulatedAnimations.push_back({ 0.0f, 1.0f, "test1" });

        OsgAnimationController controller;
        controller.setEmulatedAnimations(emulatedAnimations);

        osg::Matrixf startTransform = osg::Matrixf::identity();
        osg::Matrixf endTransform = osg::Matrixf::identity();
        endTransform.setTrans(1.0f, 1.0f, 1.0f);
        controller.addMergedAnimationTrack(createMergedAnimationTrack("test1", startTransform, endTransform));

        // Has no emulated animation at time so will return 0,0,0
        osg::Vec3f translation = controller.getTranslation(100.0f);
        EXPECT_EQ(translation, osg::Vec3f(0.0f, 0.0f, 0.0f));
    }

    TEST(OsgAnimationControllerTest, getTranslationShouldReturnZeroVectorIfNoMergedTracks)
    {
        std::vector<EmulatedAnimation> emulatedAnimations;
        emulatedAnimations.push_back({ 0.0f, 1.0f, "test1" });

        OsgAnimationController controller;
        controller.setEmulatedAnimations(emulatedAnimations);

        // Has no merged tracks so will return 0,0,0
        osg::Vec3f translation = controller.getTranslation(0.5);
        EXPECT_EQ(translation, osg::Vec3f(0.0f, 0.0f, 0.0f));
    }

    TEST(OsgAnimationControllerTest, getTransformShouldReturnIdentityIfNotFound)
    {
        std::vector<EmulatedAnimation> emulatedAnimations;
        emulatedAnimations.push_back({ 0.0f, 1.0f, "test1" });

        OsgAnimationController controller;
        controller.setEmulatedAnimations(emulatedAnimations);

        osg::Matrixf startTransform = osg::Matrixf::identity();
        osg::Matrixf endTransform = osg::Matrixf::identity();
        endTransform.setTrans(1.0f, 1.0f, 1.0f);
        controller.addMergedAnimationTrack(createMergedAnimationTrack("test1", startTransform, endTransform));

        // Has no emulated animation at time so will return identity
        EXPECT_EQ(controller.getTransformForNode(100.0f, ROOT_BONE_NAME), osg::Matrixf::identity());

        // Has no bone animation at time so will return identity
        EXPECT_EQ(controller.getTransformForNode(0.5f, "wrongbone"), osg::Matrixf::identity());
    }

    TEST(OsgAnimationControllerTest, getTransformShouldReturnSampledAnimMatrixAtTime)
    {
        std::vector<EmulatedAnimation> emulatedAnimations;
        emulatedAnimations.push_back({ 0.0f, 1.0f, "test1" }); // should sample this
        emulatedAnimations.push_back({ 1.1f, 2.0f, "test2" }); // should ignore this

        OsgAnimationController controller;
        controller.setEmulatedAnimations(emulatedAnimations);

        osg::Matrixf startTransform = osg::Matrixf::identity();
        osg::Matrixf endTransform = osg::Matrixf::identity();
        endTransform.setTrans(1.0f, 1.0f, 1.0f);
        controller.addMergedAnimationTrack(createMergedAnimationTrack("test1", startTransform, endTransform));
        osg::Matrixf endTransform2 = osg::Matrixf::identity();
        endTransform2.setTrans(2.0f, 2.0f, 2.0f);
        controller.addMergedAnimationTrack(
            createMergedAnimationTrack("test2", endTransform, endTransform2, 0.1f, 0.9f));

        EXPECT_EQ(controller.getTransformForNode(0.0f, ROOT_BONE_NAME), startTransform); // start of test1
        EXPECT_EQ(controller.getTransformForNode(1.0f, ROOT_BONE_NAME), endTransform); // end of test1
        EXPECT_EQ(controller.getTransformForNode(1.1f, ROOT_BONE_NAME), endTransform); // start of test2
        EXPECT_EQ(controller.getTransformForNode(2.0f, ROOT_BONE_NAME), endTransform2); // end of test2
    }
}
