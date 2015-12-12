#include <osg/Version>
#include <components/sceneutil/riggeometry.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// There are no members in the UpdateRigGeometry callback.  It just needs to be created, which the
// RigGeometry constructor does automatically, so there's no purpose to this serializer except to
// silence wanrings from the serialization library.   Perhaps there's a better way to handle this?

// The associated classes for osg::Drawable::UpdateCallback has apparently changed between OSG 3.2
// and OSG 3.3.  It's apparently serialized as osg::UpdateCallback, and has a parent of
// osg::Callback that's not documented in the OSG 3.2.0 Doxygen docmentation. See
// src/osgWrappers/serializers/sdg/UpdateCallback.cpp for an example.

// There appears to be a problem with osg::ComputeBoundingBoxCallback serialization in OSG 3.2.  The
// real class names is osg::Drawable::ComputeBoundingBoxCallaback.

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
#define ASSOCIATES "osg::Object osg::Callback osg::UpdateCallback OpenMW::UpdateRigBounds"
#else
#define ASSOCIATES "osg::Object OpenMW::UpdateRigBounds"
#endif

REGISTER_OBJECT_WRAPPER2(SceneUtil_UpdateRigBounds_Serializer,
                         new SceneUtil::UpdateRigBounds,
                         SceneUtil::UpdateRigBounds,
                         "OpenMW::UpdateRigBounds",
                         ASSOCIATES)
{
    SETUPMSG("OpenMW::UpdateRigBounds");
}
