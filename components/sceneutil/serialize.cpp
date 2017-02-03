#include "serialize.hpp"

#include <osgDB/ObjectWrapper>
#include <osgDB/Registry>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/riggeometry.hpp>

namespace SceneUtil
{

template <class Cls>
static osg::Object* createInstanceFunc() { return new Cls; }

class PositionAttitudeTransformSerializer : public osgDB::ObjectWrapper
{
public:
    PositionAttitudeTransformSerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<SceneUtil::PositionAttitudeTransform>, "SceneUtil::PositionAttitudeTransform", "osg::Object osg::Node osg::Group osg::Transform SceneUtil::PositionAttitudeTransform")
    {
        addSerializer( new osgDB::PropByRefSerializer< SceneUtil::PositionAttitudeTransform, osg::Vec3f >(
            "position", osg::Vec3f(), &SceneUtil::PositionAttitudeTransform::getPosition, &SceneUtil::PositionAttitudeTransform::setPosition), osgDB::BaseSerializer::RW_VEC3F );
        addSerializer( new osgDB::PropByRefSerializer< SceneUtil::PositionAttitudeTransform, osg::Quat >(
            "attitude", osg::Quat(), &SceneUtil::PositionAttitudeTransform::getAttitude, &SceneUtil::PositionAttitudeTransform::setAttitude), osgDB::BaseSerializer::RW_QUAT );
        addSerializer( new osgDB::PropByRefSerializer< SceneUtil::PositionAttitudeTransform, osg::Vec3f >(
            "scale", osg::Vec3f(), &SceneUtil::PositionAttitudeTransform::getScale, &SceneUtil::PositionAttitudeTransform::setScale), osgDB::BaseSerializer::RW_VEC3F );
    }
};

class SkeletonSerializer : public osgDB::ObjectWrapper
{
public:
    SkeletonSerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<SceneUtil::Skeleton>, "SceneUtil::Skeleton", "osg::Object osg::Node osg::Group SceneUtil::Skeleton")
    {
    }
};

class FrameSwitchSerializer : public osgDB::ObjectWrapper
{
public:
    FrameSwitchSerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<osg::Group>, "NifOsg::FrameSwitch", "osg::Object osg::Node osg::Group NifOsg::FrameSwitch")
    {
    }
};

class RigGeometrySerializer : public osgDB::ObjectWrapper
{
public:
    RigGeometrySerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<SceneUtil::RigGeometry>, "SceneUtil::RigGeometry", "osg::Object osg::Node osg::Drawable osg::Geometry SceneUtil::RigGeometry")
    {
    }
};

class LightManagerSerializer : public osgDB::ObjectWrapper
{
public:
    LightManagerSerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<osg::Group>, "SceneUtil::LightManager", "osg::Object osg::Node osg::Group SceneUtil::LightManager")
    {
    }
};

class CameraRelativeTransformSerializer : public osgDB::ObjectWrapper
{
public:
    CameraRelativeTransformSerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<osg::Group>, "MWRender::CameraRelativeTransform", "osg::Object osg::Node osg::Group MWRender::CameraRelativeTransform")
    {
    }
};

osgDB::ObjectWrapper* makeDummySerializer(const std::string& classname)
{
    return new osgDB::ObjectWrapper(createInstanceFunc<osg::DummyObject>, classname, "osg::Object");
}


void registerSerializers()
{
    static bool done = false;
    if (!done)
    {
        osgDB::Registry::instance()->getObjectWrapperManager()->addWrapper(new PositionAttitudeTransformSerializer);
        osgDB::Registry::instance()->getObjectWrapperManager()->addWrapper(new SkeletonSerializer);
        osgDB::Registry::instance()->getObjectWrapperManager()->addWrapper(new FrameSwitchSerializer);
        osgDB::Registry::instance()->getObjectWrapperManager()->addWrapper(new RigGeometrySerializer);
        osgDB::Registry::instance()->getObjectWrapperManager()->addWrapper(new LightManagerSerializer);
        osgDB::Registry::instance()->getObjectWrapperManager()->addWrapper(new CameraRelativeTransformSerializer);

        // ignore the below for now to avoid warning spam
        const char* ignore[] = {
            "MWRender::PtrHolder",
            "Resource::TemplateRef",
            "SceneUtil::LightListCallback",
            "SceneUtil::LightManagerUpdateCallback",
            "SceneUtil::UpdateRigBounds",
            "SceneUtil::UpdateRigGeometry",
            "SceneUtil::LightSource",
            "SceneUtil::StateSetUpdater",
            "NifOsg::NodeUserData",
            "NifOsg::FlipController",
            "NifOsg::KeyframeController",
            "NifOsg::TextKeyMapHolder",
            "NifOsg::Emitter",
            "NifOsg::ParticleSystem",
            "NifOsg::GrowFadeAffector",
            "NifOsg::InverseWorldMatrix",
            "NifOsg::StaticBoundingBoxCallback",
            "NifOsg::GeomMorpherController",
            "NifOsg::UpdateMorphGeometry",
            "osgMyGUI::Drawable",
            "osg::DrawCallback",
            "osgOQ::ClearQueriesCallback",
            "osgOQ::RetrieveQueriesCallback",
            "osg::DummyObject"
        };
        for (size_t i=0; i<sizeof(ignore)/sizeof(ignore[0]); ++i)
        {
            osgDB::Registry::instance()->getObjectWrapperManager()->addWrapper(makeDummySerializer(ignore[i]));
        }


        done = true;
    }
}

}
