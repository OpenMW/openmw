#include "positionattitudetransform.hpp"

namespace SceneUtil
{

PositionAttitudeTransform::PositionAttitudeTransform():
    _scale(1.0,1.0,1.0)
{
}

bool PositionAttitudeTransform::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const
{
    if (_referenceFrame==RELATIVE_RF)
    {
        matrix.preMultTranslate(_position);
        matrix.preMultRotate(_attitude);
        matrix.preMultScale(_scale);
    }
    else // absolute
    {
        matrix.makeRotate(_attitude);
        matrix.postMultTranslate(_position);
        matrix.preMultScale(_scale);
    }
    return true;
}


bool PositionAttitudeTransform::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const
{
    if (_scale.x() == 0.0 || _scale.y() == 0.0 || _scale.z() == 0.0)
        return false;

    if (_referenceFrame==RELATIVE_RF)
    {
        matrix.postMultTranslate(-_position);
        matrix.postMultRotate(_attitude.inverse());
        matrix.postMultScale(osg::Vec3f(1.0/_scale.x(), 1.0/_scale.y(), 1.0/_scale.z()));
    }
    else // absolute
    {
        matrix.makeRotate(_attitude.inverse());
        matrix.preMultTranslate(-_position);
        matrix.postMultScale(osg::Vec3f(1.0/_scale.x(), 1.0/_scale.y(), 1.0/_scale.z()));
    }
    return true;
}

}
