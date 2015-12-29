#ifndef OPENMW_COMPONENTS_NIFOSG_USERDATA_H
#define OPENMW_COMPONENTS_NIFOSG_USERDATA_H

#include <components/nif/niftypes.hpp>

#include <osg/Object>

namespace NifOsg
{

    // Note if you are copying a scene graph with this user data you should use the DEEP_COPY_USERDATA copyop.
    class NodeUserData : public osg::Object
    {
    public:
        NodeUserData(int index, float scale, const Nif::Matrix3& rotationScale)
            : mIndex(index), mScale(scale), mRotationScale(rotationScale)
        {
        }
        NodeUserData()
            : mIndex(0), mScale(0)
        {
        }
        NodeUserData(const NodeUserData& copy, const osg::CopyOp& copyop)
            : Object(copy, copyop)
            , mIndex(copy.mIndex)
            , mScale(copy.mScale)
            , mRotationScale(copy.mRotationScale)
        {
        }

        META_Object(NifOsg, NodeUserData)

        // NIF record index
        int mIndex;

        // Hack: account for Transform differences between OSG and NIFs.
        // OSG uses a 4x4 matrix, NIF's use a 3x3 rotationScale, float scale, and vec3 position.
        // Decomposing the original components from the 4x4 matrix isn't possible, which causes
        // problems when a KeyframeController wants to change only one of these components. So
        // we store the scale and rotation components separately here.
        // Note for a cleaner solution it would be possible to write a custom Transform node,
        // but then we have to fork osgAnimation :/
        float mScale;
        Nif::Matrix3 mRotationScale;
    };

}

#endif
