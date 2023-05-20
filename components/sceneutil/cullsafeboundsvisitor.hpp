#ifndef OPENMW_COMPONENTS_SCENEUTIL_CULLSAFEBOUNDSVISITOR_H
#define OPENMW_COMPONENTS_SCENEUTIL_CULLSAFEBOUNDSVISITOR_H

#include <vector>

#include <osg/BoundingBox>
#include <osg/Matrix>
#include <osg/NodeVisitor>

#include <osg/Drawable>
#include <osg/Transform>

namespace SceneUtil
{
    // Computes local bounding box of a node without dirtying itself or any of its children
    struct CullSafeBoundsVisitor : osg::NodeVisitor
    {
        CullSafeBoundsVisitor(osg::NodeVisitor::TraversalMode traversalMode = TRAVERSE_ALL_CHILDREN)
            : osg::NodeVisitor(traversalMode)
        {
        }

        void reset() override
        {
            mMatrixStack.clear();
            mBoundingBox.init();
        }

        void apply(osg::Drawable& drawable)
        {
            osg::BoundingBox bbox = drawable.getInitialBound();
            bbox.expandBy(drawable.computeBoundingBox());
            applyBoundingBox(bbox);
        }

        void apply(osg::Transform& transform)
        {
            osg::Matrix matrix;
            if (!mMatrixStack.empty())
                matrix = mMatrixStack.back();

            mMatrixStack.push_back(matrix);
            traverse(transform);
            mMatrixStack.pop_back();
        }

        void applyBoundingBox(const osg::BoundingBox& bbox)
        {
            if (mMatrixStack.empty())
            {
                mBoundingBox.expandBy(bbox);
            }
            else if (bbox.valid())
            {
                for (int i = 0; i < 8; ++i)
                    mBoundingBox.expandBy(bbox.corner(i) * mMatrixStack.back());
            }
        }

        osg::BoundingBox mBoundingBox;
        std::vector<osg::Matrix> mMatrixStack;
    };
}
#endif // OPENMW_COMPONENTS_SCENEUTIL_CULLSAFEBOUNDSVISITOR_H