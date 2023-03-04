#include "riggeometryosgaextension.hpp"

#include <osgAnimation/RigGeometry>

#include <osg/Drawable>
#include <osg/NodeVisitor>

#include <osgUtil/CullVisitor>

#include <components/debug/debuglog.hpp>
#include <components/resource/scenemanager.hpp>

namespace SceneUtil
{

OsgaRigGeometry::OsgaRigGeometry() : osgAnimation::RigGeometry()
{
    setDataVariance(osg::Object::STATIC);
}

OsgaRigGeometry::OsgaRigGeometry(const osgAnimation::RigGeometry& copy, const osg::CopyOp& copyop) : osgAnimation::RigGeometry(copy, copyop)
{
    setDataVariance(osg::Object::STATIC);
}

OsgaRigGeometry::OsgaRigGeometry(const OsgaRigGeometry& copy, const osg::CopyOp& copyop) :
    osgAnimation::RigGeometry(copy, copyop)
{
    setDataVariance(osg::Object::STATIC);
}

void OsgaRigGeometry::computeMatrixFromRootSkeleton(osg::MatrixList mtxList)
{
    if (!_root.valid())
    {
    	Log(Debug::Warning) << "Warning " << className() <<"::computeMatrixFromRootSkeleton if you have this message it means you miss to call buildTransformer(Skeleton* root), or your RigGeometry (" << getName() <<") is not attached to a Skeleton subgraph";
        return;
    }
    osg::Matrix notRoot = _root->getMatrix();
    _matrixFromSkeletonToGeometry = mtxList[0] * osg::Matrix::inverse(notRoot);
    _invMatrixFromSkeletonToGeometry = osg::Matrix::inverse(_matrixFromSkeletonToGeometry);
    _needToComputeMatrix = false;
}

RigGeometryHolder::RigGeometryHolder() :
	mBackToOrigin(nullptr),
    mLastFrameNumber(0),
	mIsBodyPart(false)
{
}

RigGeometryHolder::RigGeometryHolder(const RigGeometryHolder& copy, const osg::CopyOp& copyop) :
    Drawable(copy, copyop),
	mBackToOrigin(copy.mBackToOrigin),
    mLastFrameNumber(0),
	mIsBodyPart(copy.mIsBodyPart)
{
    setUseVertexBufferObjects(true);

    if (!copy.getSourceRigGeometry())
    {
        Log(Debug::Error) << "copy constructor of RigGeometryHolder partially failed (no source RigGeometry)";
        return;
    }

    osg::ref_ptr<OsgaRigGeometry> rigGeometry = new OsgaRigGeometry(*copy.getSourceRigGeometry(), copyop);
    setSourceRigGeometry(rigGeometry);
}

RigGeometryHolder::RigGeometryHolder(const osgAnimation::RigGeometry& copy, const osg::CopyOp& copyop) :
	mBackToOrigin(nullptr),
    mLastFrameNumber(0),
	mIsBodyPart(false)
{
    setUseVertexBufferObjects(true);

    osg::ref_ptr<OsgaRigGeometry> rigGeometry = new OsgaRigGeometry(copy, copyop);
    setSourceRigGeometry(rigGeometry);
}

void RigGeometryHolder::setSourceRigGeometry(osg::ref_ptr<OsgaRigGeometry> sourceRigGeometry)
{
    for (unsigned int i=0; i<2; ++i)
        mGeometry.at(i) = nullptr;

    mSourceRigGeometry = sourceRigGeometry;

    _boundingBox = mSourceRigGeometry->getComputeBoundingBoxCallback()->computeBound(*mSourceRigGeometry);
    _boundingSphere = osg::BoundingSphere(_boundingBox);

    for (unsigned int i=0; i<2; ++i)
    {
        const OsgaRigGeometry& from = *sourceRigGeometry;

        // DO NOT COPY AND PASTE THIS CODE. Cloning osg::Geometry without also cloning its contained Arrays is generally unsafe.
        // In this specific case the operation is safe under the following two assumptions:
        // - When Arrays are removed or replaced in the cloned geometry, the original Arrays in their place must outlive the cloned geometry regardless. (ensured by mSourceRigGeometry, possibly also RigGeometry._geometry)
        // - Arrays that we add or replace in the cloned geometry must be explicitely forbidden from reusing BufferObjects of the original geometry.
        mGeometry.at(i) = new OsgaRigGeometry(from, osg::CopyOp::SHALLOW_COPY);
        mGeometry.at(i)->getOrCreateUserDataContainer()->addUserObject(new Resource::TemplateRef(mSourceRigGeometry));

        OsgaRigGeometry& to = *mGeometry.at(i);
        to.setSupportsDisplayList(false);
        to.setUseVertexBufferObjects(true);
        to.setCullingActive(false); // make sure to disable culling since that's handled by this class

        to.setDataVariance(osg::Object::STATIC);
        to.setNeedToComputeMatrix(true);

        // vertices and normals are modified every frame, so we need to deep copy them.
        // assign a dedicated VBO to make sure that modifications don't interfere with source geometry's VBO.
        osg::ref_ptr<osg::VertexBufferObject> vbo (new osg::VertexBufferObject);
        vbo->setUsage(GL_DYNAMIC_DRAW_ARB);

        osg::ref_ptr<osg::Array> vertexArray = static_cast<osg::Array*>(from.getVertexArray()->clone(osg::CopyOp::DEEP_COPY_ALL));
        if (vertexArray)
        {
            vertexArray->setVertexBufferObject(vbo);
            to.setVertexArray(vertexArray);
        }

        if (const osg::Array* normals = from.getNormalArray())
        {
            osg::ref_ptr<osg::Array> normalArray = static_cast<osg::Array*>(normals->clone(osg::CopyOp::DEEP_COPY_ALL));
            if (normalArray)
            {
                normalArray->setVertexBufferObject(vbo);
                to.setNormalArray(normalArray, osg::Array::BIND_PER_VERTEX);
            }
        }

        if (const osg::Vec4Array* tangents = dynamic_cast<const osg::Vec4Array*>(from.getTexCoordArray(7)))
        {
            osg::ref_ptr<osg::Array> tangentArray = static_cast<osg::Array*>(tangents->clone(osg::CopyOp::DEEP_COPY_ALL));
            tangentArray->setVertexBufferObject(vbo);
            to.setTexCoordArray(7, tangentArray, osg::Array::BIND_PER_VERTEX);
        }
    }

}

osg::ref_ptr<OsgaRigGeometry> RigGeometryHolder::getSourceRigGeometry() const
{
    return mSourceRigGeometry;
}

void RigGeometryHolder::updateRigGeometry(OsgaRigGeometry* geom, osg::NodeVisitor *nv)
{
    if(!geom)
        return;

    if(!geom->getSkeleton() && !this->getParents().empty())
    {
        osgAnimation::RigGeometry::FindNearestParentSkeleton finder;
        if(this->getParents().size() > 1)
        Log(Debug::Warning) << "A RigGeometry should not have multi parent ( " << geom->getName() << " )";
        this->getParents()[0]->accept(finder);

        if(!finder._root.valid())
        {
        	Log(Debug::Warning) << "A RigGeometry did not find a parent skeleton for RigGeometry ( " << geom->getName() << " )";
            return;
        }
        geom->getRigTransformImplementation()->prepareData(*geom);
        geom->setSkeleton(finder._root.get());
    }

    if(!geom->getSkeleton())
        return;

    if(geom->getNeedToComputeMatrix())
    {
        osgAnimation::Skeleton* root = geom->getSkeleton();
        if (!root)
        {
            Log(Debug::Warning) << "Warning: if you have this message it means you miss to call buildTransformer(Skeleton* root), or your RigGeometry is not attached to a Skeleton subgraph";
            return;
        }
        osg::MatrixList mtxList = root->getWorldMatrices(root); //We always assume that RigGeometries have origin at their root
        geom->computeMatrixFromRootSkeleton(mtxList);

        if (mIsBodyPart && mBackToOrigin) updateBackToOriginTransform(geom);
    }

    if(geom->getSourceGeometry())
    {
        osg::Drawable::UpdateCallback * up = dynamic_cast<osg::Drawable::UpdateCallback*>(geom->getSourceGeometry()->getUpdateCallback());
        if(up)
        {
            up->update(nv, geom->getSourceGeometry());
        }
    }

    geom->update();
}

OsgaRigGeometry* RigGeometryHolder::getGeometry(int geometry)
{
    return mGeometry.at(geometry).get();
}


void RigGeometryHolder::updateBackToOriginTransform(OsgaRigGeometry* geometry)
{
    osgAnimation::Skeleton* skeleton = geometry->getSkeleton();
    if (skeleton)
    {
        osg::MatrixList mtxList = mBackToOrigin->getParents()[0]->getWorldMatrices(skeleton);
        osg::Matrix skeletonMatrix = skeleton->getMatrix();
        osg::Matrixf matrixFromSkeletonToGeometry = mtxList[0] * osg::Matrix::inverse(skeletonMatrix);
        osg::Matrixf invMatrixFromSkeletonToGeometry = osg::Matrix::inverse(matrixFromSkeletonToGeometry);
        mBackToOrigin->setMatrix(invMatrixFromSkeletonToGeometry);
    }
}

void RigGeometryHolder::accept(osg::NodeVisitor &nv)
{
    if (!nv.validNodeMask(*this))
        return;

    nv.pushOntoNodePath(this);

    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR && mSourceRigGeometry.get())
    {
        // The cull visitor won't be applied to the node itself,
        // but we want to use its state to render the child geometry.
        osg::StateSet* stateset = getStateSet();
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(&nv);
        if (stateset)
            cv->pushStateSet(stateset);

        unsigned int traversalNumber = nv.getTraversalNumber();
        if (mLastFrameNumber == traversalNumber)
        {
            OsgaRigGeometry& geom = *getRigGeometryPerFrame(mLastFrameNumber);

            nv.pushOntoNodePath(&geom);
            nv.apply(geom);
            nv.popFromNodePath();                
        }
        else
        {
            mLastFrameNumber = traversalNumber;

            OsgaRigGeometry& geom = *getRigGeometryPerFrame(mLastFrameNumber);
            
            if (mIsBodyPart)
            {
                if (mBackToOrigin) updateBackToOriginTransform(&geom);
                else
                {
                 	osg::MatrixTransform* matrixTransform = dynamic_cast<osg::MatrixTransform*> (this->getParents()[0]);
                 	if (matrixTransform)
                 	{
                 		mBackToOrigin = matrixTransform;
                     	updateBackToOriginTransform(&geom);
                	}
                }
            }
            
            updateRigGeometry(&geom, &nv);
            
            nv.pushOntoNodePath(&geom);
            nv.apply(geom);
            nv.popFromNodePath();
        }
        if (stateset)
            cv->popStateSet();
    }
    else if (nv.getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
    {
    }
    else
        nv.apply(*this);

    nv.popFromNodePath();
}


void RigGeometryHolder::accept(osg::PrimitiveFunctor& func) const
{
    getRigGeometryPerFrame(mLastFrameNumber)->accept(func);
}

OsgaRigGeometry* RigGeometryHolder::getRigGeometryPerFrame(unsigned int frame) const
{
    return mGeometry.at(frame%2).get();
}


}
