#include <components/nifosg/userdata.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

static bool checkRotationScale(const NifOsg::NodeUserData& node) {
    CHECKMSG("RotationScale");
    // Return true if the serializer should read/write the value.

    // Don't write the identity matrix.
    if (node.mRotationScale.isIdentity()) return false;
    // But do write all others.
    return true;
}
static bool writeRotationScale(osgDB::OutputStream& os,
                               const NifOsg::NodeUserData& node) {
    WRITEMSG("RotationScale");
    // Write nine floats for the matrix.
    os << os.BEGIN_BRACKET << std::endl;
    for (int i=0; i<3; ++i) {
        for (int j=0; j<3; ++j) {
            os << node.mRotationScale.mValues[i][j];
#if SERIALIZER_DEBUG==3
            WRITEVALUE << "RotationScale[" << i << "][" << j
                       << "] = " << node.mRotationScale.mValues[i][j] << std::endl;
#endif
        }
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    // Return true if serialization succeeded.
    return true;
}
static bool readRotationScale(osgDB::InputStream& is,
                              NifOsg::NodeUserData& node) {
    READMSG("RotationScale");
    // Read nine floats for the matrix.
    is >> is.BEGIN_BRACKET;
    for (int i=0; i<3; ++i) {
        for (int j=0; j<3; ++j) {
            is >> node.mRotationScale.mValues[i][j];
#if SERIALIZER_DEBUG==3
            READVALUE << "RotationScale[" << i << "][" << j
                      << "] = " << node.mRotationScale.mValues[i][j] << std::endl;
#endif
        }
    }
    is >> is.END_BRACKET;
    // Return true if serialization succeeded.
    return true;
}

REGISTER_OBJECT_WRAPPER2(NifOsg_NodeUserData_Serializer,
                         new NifOsg::NodeUserData,
                         NifOsg::NodeUserData,
                         "OpenMW::NodeUserData",
                         "osg::Object OpenMW::NodeUserData")
{
    SETUPMSG("OpenMW::NodeUserData");
    ADD_INT_SERIALIZER(Index, 0);
    ADD_FLOAT_SERIALIZER(Scale, 1.0);
    ADD_USER_SERIALIZER(RotationScale);
}
