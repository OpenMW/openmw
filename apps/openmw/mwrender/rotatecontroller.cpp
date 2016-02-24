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

void RotateController::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
    if (!mEnabled)
    {
        traverse(node, nv);
        return;
    }
    osg::MatrixTransform* transform = static_cast<osg::MatrixTransform*>(node);
    osg::Matrix matrix = transform->getMatrix();
    osg::Quat worldOrient = getWorldOrientation(node);

    osg::Quat orient = worldOrient * mRotate * worldOrient.inverse() * matrix.getRotate();
    matrix.setRotate(orient);

    transform->setMatrix(matrix);

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
