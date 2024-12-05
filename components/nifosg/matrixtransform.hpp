#ifndef OPENMW_COMPONENTS_NIFOSG_MATRIXTRANSFORM_H
#define OPENMW_COMPONENTS_NIFOSG_MATRIXTRANSFORM_H

#include <components/nif/niftypes.hpp>

#include <osg/MatrixTransform>

namespace NifOsg
{

    class MatrixTransform : public osg::MatrixTransform
    {
    public:
        MatrixTransform() = default;
        MatrixTransform(const Nif::NiTransform& transform);
        MatrixTransform(const MatrixTransform& copy, const osg::CopyOp& copyop);

        META_Node(NifOsg, MatrixTransform)

        // Hack: account for Transform differences between OSG and NIFs.
        // OSG uses a 4x4 matrix, NIF's use a 3x3 rotationScale, float scale, and vec3 position.
        // Decomposing the original components from the 4x4 matrix isn't possible, which causes
        // problems when a KeyframeController wants to change only one of these components. So
        // we store the scale and rotation components separately here.
        float mScale{ 0.f };
        Nif::Matrix3 mRotationScale;

        // Utility methods to transform the node and keep these components up-to-date.
        // The matrix's components should not be overridden manually or using preMult/postMult
        // unless you're sure you know what you are doing.
        void setScale(float scale);
        void setRotation(const osg::Quat& rotation);
        void setRotation(const Nif::Matrix3& rotation);
        void setTranslation(const osg::Vec3f& translation);
    };

}

#endif
