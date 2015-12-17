#include <osg/Version>
#include <components/sceneutil/riggeometry.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// The parent osg::Object was first added on Oct 8 2003 (OSG 1.9) in:
// https://github.com/openscenegraph/osg/commit/41cdaf817b8e48bc40568fb6e9ec692fae343fb2
// The initial serialization capability for osg::UpdateCallback was added on Apr 29 2014 (OSG 3.3.2) in:
// https://github.com/openscenegraph/osg/commit/d7cf8d9395b29de38f1545c94ff3039aebe6bbde
// UpdateCallback was refactored to include osg::Callback as a parent on Jun 5 2014 (OSG 3.3.2) in:
// https://github.com/openscenegraph/osg/commit/e967420323bb6e500425144cb305cf8060c1c515
// The parent osg::Callback was added to the serializer associates on Mar 17 2015 (OSG 3.3.7) in:
// https://github.com/openscenegraph/osg/commit/f1d40b80339ff5e8925d4186a4e97664dc28e24b

// UpdateCallback is defined in include/osg/Drawable
// UpdateCallback is serialized in src/osgWrappers/serializers/osg/UpdateCallback.cpp

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
// Correct the defect in 3.3.2 that was only really fixed upstream in 3.3.7?
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
