#ifndef OPENMW_COMPONENTS_NIFOSG_RIGGEOMETRY_H
#define OPENMW_COMPONENTS_NIFOSG_RIGGEOMETRY_H

#include <osg/Geometry>
#include <osg/Matrixf>

namespace SceneUtil
{
    class Skeleton;
    class Bone;

    /// @brief Mesh skinning implementation.
    /// @note A RigGeometry may be attached directly to a Skeleton, or somewhere below a Skeleton.
    /// Note though that the RigGeometry ignores any transforms below the Skeleton, so the attachment point is not that important.
    /// @note The internal Geometry used for rendering is double buffered, this allows updates to be done in a thread safe way while
    /// not compromising rendering performance. This is crucial when using osg's default threading model of DrawThreadPerContext.
    class RigGeometry : public osg::Drawable
    {
    public:
        RigGeometry();
        RigGeometry(const RigGeometry& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, RigGeometry)

        // Currently empty as this is difficult to implement. Technically we would need to compile both internal geometries in separate frames but this method is only called once. Alternatively we could compile just the static parts of the model.
        virtual void compileGLObjects(osg::RenderInfo& renderInfo) const {}

        struct BoneInfluence
        {
            osg::Matrixf mInvBindMatrix;
            osg::BoundingSpheref mBoundSphere;
            // <vertex index, weight>
            std::vector<std::pair<unsigned short, float>> mWeights;
        };

        struct InfluenceMap : public osg::Referenced
        {
            std::vector<std::pair<std::string, BoneInfluence>> mData;
        };

        void setInfluenceMap(osg::ref_ptr<InfluenceMap> influenceMap);

        /// Initialize this geometry from the source geometry.
        /// @note The source geometry will not be modified.
        void setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeom);

        osg::ref_ptr<osg::Geometry> getSourceGeometry();

        virtual void accept(osg::NodeVisitor &nv);
        virtual bool supports(const osg::PrimitiveFunctor&) const { return true; }
        virtual void accept(osg::PrimitiveFunctor&) const;

        struct CopyBoundingBoxCallback : osg::Drawable::ComputeBoundingBoxCallback
        {
            osg::BoundingBox boundingBox;

            virtual osg::BoundingBox computeBound(const osg::Drawable&) const override { return boundingBox; }
        };

        struct CopyBoundingSphereCallback : osg::Node::ComputeBoundingSphereCallback
        {
            osg::BoundingSphere boundingSphere;

            virtual osg::BoundingSphere computeBound(const osg::Node&) const override { return boundingSphere; }
        };

    private:
        void cull(osg::NodeVisitor* nv);
        void updateBounds(osg::NodeVisitor* nv);

        osg::ref_ptr<osg::Geometry> mGeometry[2];
        osg::Geometry* getGeometry(unsigned int frame) const;

        osg::ref_ptr<osg::Geometry> mSourceGeometry;
        osg::ref_ptr<const osg::Vec4Array> mSourceTangents;
        Skeleton* mSkeleton;

        osg::ref_ptr<osg::RefMatrix> mGeomToSkelMatrix;

        osg::ref_ptr<InfluenceMap> mInfluenceMap;

        typedef std::pair<std::string, osg::Matrixf> BoneBindMatrixPair;

        typedef std::pair<BoneBindMatrixPair, float> BoneWeight;

        typedef std::vector<unsigned short> VertexList;

        typedef std::map<std::vector<BoneWeight>, VertexList> Bone2VertexMap;

        struct Bone2VertexVector : public osg::Referenced
        {
            std::vector<std::pair<std::vector<BoneWeight>, VertexList>> mData;
        };
        osg::ref_ptr<Bone2VertexVector> mBone2VertexVector;

        struct BoneSphereVector : public osg::Referenced
        {
            std::vector<std::pair<std::string, osg::BoundingSpheref>> mData;
        };
        osg::ref_ptr<BoneSphereVector> mBoneSphereVector;
        std::vector<Bone*> mBoneNodesVector;

        unsigned int mLastFrameNumber;
        bool mBoundsFirstFrame;

        bool initFromParentSkeleton(osg::NodeVisitor* nv);

        void updateGeomToSkelMatrix(const osg::NodePath& nodePath);
    };

}

#endif
