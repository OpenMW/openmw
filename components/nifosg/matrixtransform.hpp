#ifndef OPENMW_COMPONENTS_NIFOSG_MATRIXTRANSFORM_H
#define OPENMW_COMPONENTS_NIFOSG_MATRIXTRANSFORM_H

#include <components/nif/niftypes.hpp>

#include <osg/MatrixTransform>

namespace NifOsg
{

    class MatrixTransform : public osg::MatrixTransform
    {
    public:
        MatrixTransform();
        MatrixTransform(const Nif::Transformation &trafo);
        MatrixTransform(const MatrixTransform &copy, const osg::CopyOp &copyop);

        META_Node(NifOsg, MatrixTransform)

        // Apply the current NIF rotation or scale to OSG matrix.
        void applyCurrentRotation();
        void applyCurrentScale();

        // Apply the given rotation to OSG matrix directly and update NIF rotation matrix.
        void updateRotation(const osg::Quat& rotation);
        // Update current NIF scale and apply it to OSG matrix.
        void updateScale(const float scale);

        // Apply the given translation to OSG matrix.
        void setTranslation(const osg::Vec3f& translation);

    private:
        // Hack: account for Transform differences between OSG and NIFs.
        // OSG uses a 4x4 matrix, NIF's use a 3x3 rotationScale, float scale, and vec3 position.
        // Decomposing the original components from the 4x4 matrix isn't possible, which causes
        // problems when a KeyframeController wants to change only one of these components. So
        // we store the scale and rotation components separately here.
        float mScale{0.f};
        Nif::Matrix3 mRotationScale;
    };

}

#endif
