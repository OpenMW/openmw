#include <osg/Version>
#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

// OSG 3.2 provides osg::BoundingBoxf, but no way to serialize it.

// The initial commit of this capability was on Feb 10 2014 (OSG 3.3.2) in:
// https://github.com/openscenegraph/osg/commit/8b485f0b588e6311a24b90711ae548e49e0468ae

// This implementation adapted from OSG 3.3.2 in:
//  src/osgDB/InputStream.cpp
//  src/osgDB/OutputStream.cpp

#if OSG_VERSION_LESS_THAN(3,3,2)
static bool checkBoundingBox(const NifOsg::StaticBoundingBoxCallback& node) {
    CHECKMSG("BoundingBox");
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
#if OSG_VERSION_LESS_THAN(3,3,2)
    ADD_USER_SERIALIZER(BoundingBox);
#else
    ADD_BOUNDINGBOXF_SERIALIZER(BoundingBox, osg::BoundingBoxf());
#endif
}
