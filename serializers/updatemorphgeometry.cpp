#include <osg/Version>
#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// The only member in the UpdateMorphGeometry callback is the last frame number which is computed
// dynamically in cull().  The callback is usually created in handleMorphGeometry, so we'll need to
// either serialize it, or create it in the GeomMorphController constructor?

// The associated classes for osg::Drawable::CullCallback has apparently changed between OSG 3.2 and
// OSG 3.3.  It's apparently serialized as simply osg::Callback, that's not documented in the OSG
// 3.2.0 Doxygen docmentation. See src/osgWrappers/serializers/sdg/Drawable.cpp for an example.

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
#define ASSOCIATES "osg::Object osg::Callback OpenMW::UpdateMorphGeometry"
#else
#define ASSOCIATES "osg::Object OpenMW::UpdateMorphGeometry"
#endif

REGISTER_OBJECT_WRAPPER2(NifOsg_UpdateMorphGeometry_Serializer,
                         new NifOsg::UpdateMorphGeometry,
                         NifOsg::UpdateMorphGeometry,
                         "OpenMW::UpdateMorphGeometry",
                         ASSOCIATES)
{
    SETUPMSG("OpenMW::UpdateMorphGeometry");
    // No serialization is required for mLastFrameNumber
}
