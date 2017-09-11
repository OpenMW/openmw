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

        // At this point compileGLObjects() remains unimplemented, hard to avoid race conditions
        // and there is limited value in compiling anyway since the data will change again for the next frame

        struct BoneInfluence
        {
            osg::Matrixf mInvBindMatrix;
            osg::BoundingSpheref mBoundSphere;
            // <vertex index, weight>
            std::map<unsigned short, float> mWeights;
        };

        struct InfluenceMap : public osg::Referenced
        {
            std::map<std::string, BoneInfluence> mMap;
        };

        void setInfluenceMap(osg::ref_ptr<InfluenceMap> influenceMap);

        /// Initialize this geometry from the source geometry.
        /// @note The source geometry will not be modified.
        void setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeom);

        osg::ref_ptr<osg::Geometry> getSourceGeometry();

        virtual void accept(osg::NodeVisitor &nv);
        virtual bool supports(const osg::PrimitiveFunctor&) const { return true; }
        virtual void accept(osg::PrimitiveFunctor&) const;

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

        typedef std::pair<Bone*, osg::Matrixf> BoneBindMatrixPair;

        typedef std::pair<BoneBindMatrixPair, float> BoneWeight;

        typedef std::vector<unsigned short> VertexList;

        typedef std::map<std::vector<BoneWeight>, VertexList> Bone2VertexMap;

        Bone2VertexMap mBone2VertexMap;

        typedef std::map<Bone*, osg::BoundingSpheref> BoneSphereMap;

        BoneSphereMap mBoneSphereMap;

        unsigned int mLastFrameNumber;
        bool mBoundsFirstFrame;

        bool initFromParentSkeleton(osg::NodeVisitor* nv);

        void updateGeomToSkelMatrix(const osg::NodePath& nodePath);
    };

}

#endif
