#include "matrixtransform.hpp"

namespace NifOsg
{
    MatrixTransform::MatrixTransform()
        : osg::MatrixTransform()
    {
    }

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

    void MatrixTransform::applyCurrentRotation()
    {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                _matrix(j,i) = mRotationScale.mValues[i][j]; // NB column/row major difference
    }

    void MatrixTransform::applyCurrentScale()
    {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                _matrix(i,j) *= mScale;
    }

    void MatrixTransform::updateRotation(const osg::Quat& rotation)
    {
        _matrix.setRotate(rotation);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                mRotationScale.mValues[i][j] = _matrix(j,i); // NB column/row major difference
    }

    void MatrixTransform::updateScale(const float scale)
    {
        mScale = scale;
        applyCurrentScale();
    }

    void MatrixTransform::setTranslation(const osg::Vec3f& translation)
    {
        _matrix.setTrans(translation);
    }
}
