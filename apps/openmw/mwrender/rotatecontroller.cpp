#include "rotatecontroller.hpp"

#include <osg/MatrixTransform>

namespace MWRender
{

RotateController::RotateController(osg::Node *relativeTo)
    : mEnabled(true)
    , mRelativeTo(relativeTo)
{

}

void RotateController::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

void RotateController::setRotate(const osg::Quat &rotate)
{
    mRotate = rotate;
}

void RotateController::setOffset(const osg::Vec3f& offset)
{
    mOffset = offset;
}

void RotateController::operator()(osg::MatrixTransform *node, osg::NodeVisitor *nv)
{
    if (!mEnabled)
    {
        traverse(node, nv);
        return;
    }
    osg::Matrix matrix = node->getMatrix();
    osg::Quat worldOrient = getWorldOrientation(node);
    osg::Quat worldOrientInverse = worldOrient.inverse();

    osg::Quat orient = worldOrient * mRotate * worldOrientInverse * matrix.getRotate();
    matrix.setRotate(orient);
    matrix.setTrans(matrix.getTrans() + worldOrientInverse * mOffset);

    node->setMatrix(matrix);

    traverse(node,nv);
}

osg::Quat RotateController::getWorldOrientation(osg::Node *node)
{
    // this could be optimized later, we just need the world orientation, not the full matrix
    osg::NodePathList nodepaths = node->getParentalNodePaths(mRelativeTo);
    osg::Quat worldOrient;
    if (!nodepaths.empty())
    {
        osg::Matrixf worldMat = osg::computeLocalToWorld(nodepaths[0]);
        worldOrient = worldMat.getRotate();
    }
    return worldOrient;
}

}
