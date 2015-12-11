#include <osg/Version>
#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// TODO: Test that OSG 3.2 and OSG 3.3 serializations are identical (already in correct order).

// The osg::Drawable::ComputeBoundingBoxCallback is serialized (with an empty serializer) as
// osg::ComputeBoundingBoxCallback.  Or at least that's the beahvior in OSG 3.3.  See the file:
// src/osgWrappers/serializers/osg/ComputeBoundingBoxCallback.cpp for an example.

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
#else
static bool checkBoundingBox(const NifOsg::StaticBoundingBoxCallback& node) {
    CHECKMSG("BoundingBox");
    // if (node.mBoundingBox == osg::BoundingBox()) return false;
    return true;
}

static bool writeBoundingBox(osgDB::OutputStream& os,
                             const NifOsg::StaticBoundingBoxCallback& node) {
    WRITEMSG("BoundingBox");
    const osg::BoundingBox& bbox = node.getBoundingBox();
    os << bbox.xMin() << bbox.yMin() << bbox.zMin();
    os << bbox.xMax() << bbox.yMax() << bbox.zMax() << std::endl;
    return true;
}

static bool readBoundingBox(osgDB::InputStream& is,
                            NifOsg::StaticBoundingBoxCallback& node) {
    READMSG("BoundingBox");
    float xmin, ymin, zmin, xmax, ymax, zmax;
    is >> xmin >> ymin >> zmin >> xmax >> ymax >> zmax;
    osg::BoundingBox bbox = osg::BoundingBox(xmin, ymin, zmin, xmax, ymax, zmax);
    node.setBoundingBox(bbox);
    return true;
}
#endif

REGISTER_OBJECT_WRAPPER2(NifOsg_StaticBoundingBoxCallback_Serializer,
                         new NifOsg::StaticBoundingBoxCallback,
                         NifOsg::StaticBoundingBoxCallback,
                         "OpenMW::StaticBoundingBoxCallback",
                         "osg::Object osg::ComputeBoundingBoxCallback OpenMW::StaticBoundingBoxCallback")
{
    SETUPMSG("OpenMW::StaticBoundingBoxCallback");
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
    ADD_BOUNDINGBOXF_SERIALIZER(BoundingBox, osg::BoundingBoxf());
#else
    ADD_USER_SERIALIZER(BoundingBox);
#endif
}
