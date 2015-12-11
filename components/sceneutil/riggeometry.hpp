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
    class RigGeometry : public osg::Geometry
    {
    public:
        RigGeometry();
        RigGeometry(const RigGeometry& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, RigGeometry)

        typedef std::map<unsigned short, float> BoneWeightMap;
        struct BoneInfluence
        {
            osg::Matrixf mInvBindMatrix;
            osg::BoundingSpheref mBoundSphere;
            // <vertex index, weight>
            BoneWeightMap mWeights;
        };

        typedef std::map<std::string, BoneInfluence> InfluenceMapType;
        struct InfluenceMap : public osg::Referenced
        {
            InfluenceMapType mMap;
        };

        void setInfluenceMap(osg::ref_ptr<InfluenceMap> influenceMap);
        inline const osg::ref_ptr<const InfluenceMap> getInfluenceMap() const { return mInfluenceMap; }

        void setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeom);
        inline const osg::ref_ptr<const osg::Geometry> getSourceGeometry() const { return mSourceGeometry; }

        // Called automatically by our CullCallback
        void update(osg::NodeVisitor* nv);

        // Called automatically by our UpdateCallback
        void updateBounds(osg::NodeVisitor* nv);

    private:
        osg::ref_ptr<osg::Geometry> mSourceGeometry;
        Skeleton* mSkeleton;

        osg::NodePath mSkelToGeomPath;
        osg::Matrixf mGeomToSkelMatrix;

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

        void updateGeomToSkelMatrix(osg::NodeVisitor* nv);
    };

    class UpdateRigBounds : public osg::Drawable::UpdateCallback
    {
    public:
        UpdateRigBounds()
        {
        }

        UpdateRigBounds(const UpdateRigBounds& copy, const osg::CopyOp& copyop)
            : osg::Drawable::UpdateCallback(copy, copyop)
        {
        }

        META_Object(OpenMW, UpdateRigBounds)

        void update(osg::NodeVisitor* nv, osg::Drawable* drw);
    };

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

        META_Object(OpenMW, UpdateRigGeometry)

        virtual bool cull(osg::NodeVisitor* nv, osg::Drawable* drw, osg::State*) const;
    };
}

#endif
