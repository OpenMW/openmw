#include <osg/Version>
#include <components/sceneutil/riggeometry.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// osg::CullCallback was refactored to include osg::Callback as a parent on Jun 5 2014 (OSG 3.3.2) in:
// https://github.com/openscenegraph/osg/commit/e967420323bb6e500425144cb305cf8060c1c515

// osg::Cullback is serialized in src/osgWrappers/serializers/osg/Drawable.cpp as simply osg::Callback
// The is no src/osgWrappers/serializers/osg/CullCallback.cpp
// It's unclear if this correct, but we can't associate osg::CullCallback as a result.

// See updaterigbounds.cpp for a history of the related osg::UpdateCallback parentage.

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
