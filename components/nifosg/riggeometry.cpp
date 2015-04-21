#include "riggeometry.hpp"

#include <stdexcept>
#include <iostream>

#include <cstdlib>

#include <osg/MatrixTransform>

#include "skeleton.hpp"

#include <osg/io_utils>

namespace NifOsg
{

// TODO: make threadsafe for multiple cull threads
class UpdateRigGeometry : public osg::Drawable::CullCallback
{
public:
    UpdateRigGeometry()
    {
    }

    UpdateRigGeometry(const UpdateRigGeometry& copy, const osg::CopyOp& copyop)
        : osg::Drawable::CullCallback(copy, copyop)
    {
    }

    META_Object(NifOsg, UpdateRigGeometry)

    virtual bool cull(osg::NodeVisitor* nv, osg::Drawable* drw, osg::State*) const
    {
        RigGeometry* geom = static_cast<RigGeometry*>(drw);
        geom->update(nv);
        return false;
    }
};

RigGeometry::RigGeometry()
{
    setCullCallback(new UpdateRigGeometry);
    setSupportsDisplayList(false);
}

RigGeometry::RigGeometry(const RigGeometry &copy, const osg::CopyOp &copyop)
    : osg::Geometry(copy, copyop)
    , mInfluenceMap(copy.mInfluenceMap)
{
    setSourceGeometry(copy.mSourceGeometry);
}

void RigGeometry::setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeometry)
{
    mSourceGeometry = sourceGeometry;

    osg::Geometry& from = *sourceGeometry;

    if (from.getStateSet())
        setStateSet(from.getStateSet());

    // copy over primitive sets.
    getPrimitiveSetList() = from.getPrimitiveSetList();

    if (from.getColorArray())
        setColorArray(from.getColorArray());

    if (from.getSecondaryColorArray())
        setSecondaryColorArray(from.getSecondaryColorArray());

    if (from.getFogCoordArray())
        setFogCoordArray(from.getFogCoordArray());

    for(unsigned int ti=0;ti<from.getNumTexCoordArrays();++ti)
    {
        if (from.getTexCoordArray(ti))
            setTexCoordArray(ti,from.getTexCoordArray(ti));
    }

    osg::Geometry::ArrayList& arrayList = from.getVertexAttribArrayList();
    for(unsigned int vi=0;vi< arrayList.size();++vi)
    {
        osg::Array* array = arrayList[vi].get();
        if (array)
            setVertexAttribArray(vi,array);
    }

    setVertexArray(dynamic_cast<osg::Array*>(from.getVertexArray()->clone(osg::CopyOp::DEEP_COPY_ALL)));
    setNormalArray(dynamic_cast<osg::Array*>(from.getNormalArray()->clone(osg::CopyOp::DEEP_COPY_ALL)), osg::Array::BIND_PER_VERTEX);
}

bool RigGeometry::initFromParentSkeleton(osg::NodeVisitor* nv)
{
    const osg::NodePath& path = nv->getNodePath();
    for (osg::NodePath::const_reverse_iterator it = path.rbegin(); it != path.rend(); ++it)
    {
        osg::Node* node = *it;
        if (Skeleton* skel = dynamic_cast<Skeleton*>(node))
        {
            mSkeleton = skel;
            break;
        }
    }

    if (!mSkeleton)
    {
        std::cerr << "A RigGeometry did not find its parent skeleton" << std::endl;
        return false;
    }

    if (!mInfluenceMap)
    {
        std::cerr << "No InfluenceMap set on RigGeometry" << std::endl;
        return false;
    }

    for (std::map<std::string, BoneInfluence>::const_iterator it = mInfluenceMap->mMap.begin(); it != mInfluenceMap->mMap.end(); ++it)
    {
        Bone* b = mSkeleton->getBone(it->first);
        if (!b)
        {
            std::cerr << "RigGeometry did not find bone " << it->first << std::endl;
        }

        mResolvedInfluenceMap[b] = it->second;
    }
    return true;
}

void RigGeometry::update(osg::NodeVisitor* nv)
{
    if (!mSkeleton)
    {
        if (!initFromParentSkeleton(nv))
            return;
    }

    mSkeleton->updateBoneMatrices(nv);

    osg::NodePath path;
    bool foundSkel = false;
    for (osg::NodePath::const_iterator it = nv->getNodePath().begin(); it != nv->getNodePath().end(); ++it)
    {
        if (!foundSkel)
        {
            if (*it == mSkeleton)
                foundSkel = true;
        }
        else
            path.push_back(*it);
    }
    osg::Matrix geomToSkel = osg::computeWorldToLocal(path);

    // skinning
    osg::Vec3Array* positionSrc = static_cast<osg::Vec3Array*>(mSourceGeometry->getVertexArray());
    osg::Vec3Array* normalSrc = static_cast<osg::Vec3Array*>(mSourceGeometry->getNormalArray());

    osg::Vec3Array* positionDst = static_cast<osg::Vec3Array*>(getVertexArray());
    osg::Vec3Array* normalDst = static_cast<osg::Vec3Array*>(getNormalArray());

    for (unsigned int i=0; i<positionDst->size(); ++i)
        (*positionDst)[i] = osg::Vec3f(0,0,0);
    for (unsigned int i=0; i<positionDst->size(); ++i)
        (*normalDst)[i] = osg::Vec3f(0,0,0);

    for (ResolvedInfluenceMap::const_iterator it = mResolvedInfluenceMap.begin(); it != mResolvedInfluenceMap.end(); ++it)
    {
        const BoneInfluence& bi = it->second;
        Bone* bone = it->first;

        // Here we could cache the (weighted) matrix for each combination of bone weights

        osg::Matrixf finalMatrix = bi.mInvBindMatrix * bone->mMatrixInSkeletonSpace * geomToSkel;

        for (std::map<short, float>::const_iterator weightIt = bi.mWeights.begin(); weightIt != bi.mWeights.end(); ++weightIt)
        {
            short vertex = weightIt->first;
            float weight = weightIt->second;

            osg::Vec3f a = (*positionSrc)[vertex];

            (*positionDst)[vertex] += finalMatrix.preMult(a) * weight;
            (*normalDst)[vertex] += osg::Matrix::transform3x3((*normalSrc)[vertex], finalMatrix) * weight;
        }
    }

    positionDst->dirty();
    normalDst->dirty();
}

void RigGeometry::setInfluenceMap(osg::ref_ptr<InfluenceMap> influenceMap)
{
    mInfluenceMap = influenceMap;
}


}
