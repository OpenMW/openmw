#include <components/nifosg/controller.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <osg/FrameStamp>
#include <osg/StateSet>

TEST(NifOsgUVControllerTest, updatesEveryTextureUnit)
{
    Nif::NiUVData data;
    data.mKeyList[0] = std::make_shared<Nif::FloatKeyMap>();
    data.mKeyList[0]->mKeys = { { 0.f, { 0.5f, 0.f, 0.f } } };
    osg::ref_ptr<NifOsg::UVController> controller = new NifOsg::UVController(&data, { 0, 3, 7 });
    controller->setSource(std::make_shared<SceneUtil::FrameTimeSource>());
    osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
    controller->setDefaults(stateset);
    osg::NodeVisitor visitor;
    visitor.setFrameStamp(new osg::FrameStamp);
    controller->apply(stateset, &visitor);
    for (const char* name : { "texMat0", "texMat3", "texMat7" })
    {
        osg::Matrixf matrix;
        ASSERT_TRUE(stateset->getUniform(name)->get(matrix)) << name;
        EXPECT_EQ(matrix, osg::Matrixf::translate(-0.5f, 0.f, 0.f)) << name;
    }
}
