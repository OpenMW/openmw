#include <osg/Version>

// OSG 3.2 offers this class, but apparently no serialization for it. :-(

// The initial commit of this capability was on Apr 29 2014 (OSG 3.3.2) in:
// https://github.com/openscenegraph/osg/commit/d7cf8d9395b29de38f1545c94ff3039aebe6bbde
// It was revised cosmetically on Mar 17 2015 (OSG 3.3.7) in:
// https://github.com/openscenegraph/osg/commit/f1d40b80339ff5e8925d4186a4e97664dc28e24b

// This approach back-ported from OSG 3.3.2 commit above.
// See: src/osgWrappers/serializers/osg/ComputeBoundingBoxCallback.cpp

// Unfortunately, it still doesn't work in OSG 3.2 because it appears that the changes
// from commit d7cf8d93 in include/osgAnimation/RigGeometry are required to actually use
// the callback.

#if OSG_VERSION_LESS_THAN(3,3,2)
#include <osg/Drawable>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>

#define SERIALIZER_DEBUG 1
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(ComputeBoundingBoxCallback,
                         new osg::Drawable::ComputeBoundingBoxCallback,
                         osg::Drawable::ComputeBoundingBoxCallback,
                         "osg::ComputeBoundingBoxCallback",
                         "osg::Object osg::ComputeBoundingBoxCallback") {
    SETUPMSG("osg::ComputeBoundingBoxCallback");
}
#endif
