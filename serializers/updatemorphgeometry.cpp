#include <osg/Version>
#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// See updateriggeometry.cpp for a history of the osg::CullCallback parentage.
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
