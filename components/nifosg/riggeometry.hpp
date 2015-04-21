#ifndef OPENMW_COMPONENTS_NIFOSG_RIGGEOMETRY_H
#define OPENMW_COMPONENTS_NIFOSG_RIGGEOMETRY_H

#include <osg/Geometry>
#include <osg/Matrixf>

namespace NifOsg
{

    class Skeleton;
    class Bone;

    class RigGeometry : public osg::Geometry
    {
    public:
        RigGeometry();
        RigGeometry(const RigGeometry& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, RigGeometry)

        struct BoneInfluence
        {
            osg::Matrixf mInvBindMatrix;
            // <vertex index, weight>
            std::map<short, float> mWeights;
        };


        struct InfluenceMap : public osg::Referenced
        {
            std::map<std::string, BoneInfluence> mMap;
        };

        void setInfluenceMap(osg::ref_ptr<InfluenceMap> influenceMap);

        void setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeom);

        // Called automatically by our CullCallback
        void update(osg::NodeVisitor* nv);


    private:
        osg::ref_ptr<osg::Geometry> mSourceGeometry;
        osg::ref_ptr<Skeleton> mSkeleton;

        osg::ref_ptr<InfluenceMap> mInfluenceMap;

        typedef std::map<Bone*, BoneInfluence> ResolvedInfluenceMap;
        ResolvedInfluenceMap mResolvedInfluenceMap;

        bool initFromParentSkeleton(osg::NodeVisitor* nv);
    };

}

#endif
