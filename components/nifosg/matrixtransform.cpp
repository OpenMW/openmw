#include "matrixtransform.hpp"

namespace NifOsg
{
    MatrixTransform::MatrixTransform(const Nif::Transformation &trafo)
        : osg::MatrixTransform(trafo.toMatrix())
        , mScale(trafo.scale)
        , mRotationScale(trafo.rotation)
    {
    }

    MatrixTransform::MatrixTransform(const MatrixTransform &copy, const osg::CopyOp &copyop)
        : osg::MatrixTransform(copy, copyop)
        , mScale(copy.mScale)
        , mRotationScale(copy.mRotationScale)
    {
    }

    void MatrixTransform::setScale(float scale)
    {
        if (mScale == scale)
            return;

        // Rescale the node using the known decomposed rotation.
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                _matrix(i,j) = mRotationScale.mValues[j][i] * scale; // NB: column/row major difference

        // Update the current decomposed scale.
        mScale = scale;

        _inverseDirty = true;
        dirtyBound();
    }

    void MatrixTransform::setRotation(const osg::Quat &rotation)
    {
        // First override the rotation ignoring the scale.
        _matrix.setRotate(rotation);
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                // Update the current decomposed rotation and restore the known scale.
                mRotationScale.mValues[j][i] = _matrix(i,j); // NB: column/row major difference
                _matrix(i,j) *= mScale;
            }
        }

        _inverseDirty = true;
        dirtyBound();
    }

    void MatrixTransform::setTranslation(const osg::Vec3f &translation)
    {
        // The translation is independent from the rotation and scale so we can apply it directly.
        _matrix.setTrans(translation);

        _inverseDirty = true;
        dirtyBound();
    }
}
