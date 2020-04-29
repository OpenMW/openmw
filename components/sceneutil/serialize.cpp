#include "serialize.hpp"

#include <osgDB/ObjectWrapper>
#include <osgDB/Registry>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/riggeometry.hpp>
#include <components/sceneutil/morphgeometry.hpp>

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

class RigGeometrySerializer : public osgDB::ObjectWrapper
{
public:
    RigGeometrySerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<SceneUtil::RigGeometry>, "SceneUtil::RigGeometry", "osg::Object osg::Node osg::Drawable SceneUtil::RigGeometry")
    {
    }
};

class MorphGeometrySerializer : public osgDB::ObjectWrapper
{
public:
    MorphGeometrySerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<SceneUtil::MorphGeometry>, "SceneUtil::MorphGeometry", "osg::Object osg::Node osg::Drawable SceneUtil::MorphGeometry")
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

class GeometrySerializer : public osgDB::ObjectWrapper
{
public:
    GeometrySerializer()
        : osgDB::ObjectWrapper(createInstanceFunc<osg::Drawable>, "osg::Geometry", "osg::Object osg::Drawable osg::Geometry")
    {
    }
};

void registerSerializers()
{
    static bool done = false;
    if (!done)
    {
        osgDB::ObjectWrapperManager* mgr = osgDB::Registry::instance()->getObjectWrapperManager();
        mgr->addWrapper(new PositionAttitudeTransformSerializer);
        mgr->addWrapper(new SkeletonSerializer);
        mgr->addWrapper(new RigGeometrySerializer);
        mgr->addWrapper(new MorphGeometrySerializer);
        mgr->addWrapper(new LightManagerSerializer);
        mgr->addWrapper(new CameraRelativeTransformSerializer);

        // Don't serialize Geometry data as we are more interested in the overall structure rather than tons of vertex data that would make the file large and hard to read.
        mgr->removeWrapper(mgr->findWrapper("osg::Geometry"));
        mgr->addWrapper(new GeometrySerializer);

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
            "NifOsg::CollisionSwitch",
            "osgMyGUI::Drawable",
            "osg::DrawCallback",
            "osgOQ::ClearQueriesCallback",
            "osgOQ::RetrieveQueriesCallback",
            "osg::DummyObject"
        };
        for (size_t i=0; i<sizeof(ignore)/sizeof(ignore[0]); ++i)
        {
            mgr->addWrapper(makeDummySerializer(ignore[i]));
        }


        done = true;
    }
}

}
