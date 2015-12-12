#include <osg/Version>
#include <components/sceneutil/riggeometry.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// There are no members in the UpdateRigGeometry callback.  It just needs to be created, which the
// RigGeometry constructor does automatically, so there's no purpose to this serializer except to
// silence wanrings from the serialization library.   Perhaps there's a better way to handle this?

// The associated classes for osg::Drawable::CullCallback has apparently changed between OSG 3.2 and
// OSG 3.3.  It's apparently serialized as simply osg::Callback, that's not documented in the OSG
// 3.2.0 Doxygen docmentation. See src/osgWrappers/serializers/sdg/Drawable.cpp for an example.

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
#define ASSOCIATES "osg::Object osg::Callback OpenMW::UpdateRigGeometry"
#else
#define ASSOCIATES "osg::Object OpenMW::UpdateRigGeometry"
#endif

REGISTER_OBJECT_WRAPPER2(SceneUtil_UpdateRigGeometry_Serializer,
                         new SceneUtil::UpdateRigGeometry,
                         SceneUtil::UpdateRigGeometry,
                         "OpenMW::UpdateRigGeometry",
                         ASSOCIATES)
{
    SETUPMSG("OpenMW::UpdateRigGeometry");
}
