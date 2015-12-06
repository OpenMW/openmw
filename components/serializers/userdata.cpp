//#define SERIALIZER_DEBUG

#ifdef SERIALIZER_DEBUG
#include <iostream>
#endif

#include <boost/foreach.hpp>

#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osgDB/Serializer>
#include "fixes.hpp"

#include <components/nifosg/nifloader.hpp>
#include <components/nifosg/userdata.hpp>

static bool checkRotationScale(const NifOsg::NodeUserData& node) {
    // Return true if the serializer should read/write the value.

    // Don't write the identity matrix.
    if (node.mRotationScale.isIdentity()) return false;
    // But do write all others.
    return true;
}
static bool writeRotationScale(osgDB::OutputStream& os,
                               const NifOsg::NodeUserData& node) {
    // Write nine floats for the matrix.
    os << os.BEGIN_BRACKET << std::endl;
    for (int i=0; i<3; ++i) {
        for (int j=0; j<3; ++j) {
            os << node.mRotationScale.mValues[i][j];
#ifdef SERIALIZER_DEBUG
            std::cout << "Wrote OpenMW::NodeUserData.mRotationScale["
                      << i << "][" << j << "] = "
                      << node.mRotationScale.mValues[i][j]<< std::endl;
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
    // Read nine floats for the matrix.
    is >> is.BEGIN_BRACKET;
    for (int i=0; i<3; ++i) {
        for (int j=0; j<3; ++j) {
            is >> node.mRotationScale.mValues[i][j];
#ifdef SERIALIZER_DEBUG
            std::cout << "Read OpenMW::NodeUserData.mRotationScale ["
                      << i << "][" << j << "] = "
                      << node.mRotationScale.mValues[i][j]<< std::endl;
#endif
        }
    }
    is >> is.END_BRACKET;
    // Return true if serialization succeeded.
    return true;
}

#define MyClass NifOsg::NodeUserData
REGISTER_OBJECT_WRAPPER(NifOsg_NodeUserData_Serializer,
                        new NifOsg::NodeUserData,
                        "OpenMW::NodeUserData",
                        "osg::Object OpenMW::NodeUserData")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up OpenMW::NodeUserData serializer..." << std::endl;
#endif
    
    ADD_INT_SERIALIZER(Index, 0);
    ADD_FLOAT_SERIALIZER(Scale, 1.0);
    ADD_USER_SERIALIZER(RotationScale);
}
