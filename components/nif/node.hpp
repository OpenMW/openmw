#ifndef OPENMW_COMPONENTS_NIF_NODE_HPP
#define OPENMW_COMPONENTS_NIF_NODE_HPP

#include <osg/Plane>

#include "base.hpp"

namespace Nif
{

struct NiNode;

struct NiBoundingVolume
{
    enum Type
    {
        BASE_BV = 0xFFFFFFFF,
        SPHERE_BV = 0,
        BOX_BV = 1,
        CAPSULE_BV = 2,
        LOZENGE_BV = 3,
        UNION_BV = 4,
        HALFSPACE_BV = 5
    };

    struct NiSphereBV
    {
        osg::Vec3f center;
        float radius{0.f};
    };

    struct NiBoxBV
    {
        osg::Vec3f center;
        Matrix3 axes;
        osg::Vec3f extents;
    };

    struct NiCapsuleBV
    {
        osg::Vec3f center, axis;
        float extent{0.f}, radius{0.f};
    };

    struct NiLozengeBV
    {
        float radius{0.f}, extent0{0.f}, extent1{0.f};
        osg::Vec3f center, axis0, axis1;
    };

    struct NiHalfSpaceBV
    {
        osg::Plane plane;
        osg::Vec3f origin;
    };

    unsigned int type;
    NiSphereBV sphere;
    NiBoxBV box;
    NiCapsuleBV capsule;
    NiLozengeBV lozenge;
    std::vector<NiBoundingVolume> children;
    NiHalfSpaceBV halfSpace;

    void read(NIFStream* nif);
};

/** A Node is an object that's part of the main NIF tree. It has
    parent node (unless it's the root), and transformation (location
    and rotation) relative to it's parent.
 */
struct Node : public Named
{
    enum Flags {
        Flag_Hidden = 0x0001,
        Flag_MeshCollision = 0x0002,
        Flag_BBoxCollision = 0x0004,
        Flag_ActiveCollision = 0x0020
    };

    // Node flags. Interpretation depends somewhat on the type of node.
    unsigned int flags;

    Transformation trafo;
    osg::Vec3f velocity; // Unused? Might be a run-time game state
    PropertyList props;

    // Bounding box info
    bool hasBounds{false};
    NiBoundingVolume bounds;

    // Collision object info
    NiCollisionObjectPtr collision;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;

    // Parent node, or nullptr for the root node. As far as I'm aware, only
    // NiNodes (or types derived from NiNodes) can be parents.
    std::vector<NiNode*> parents;

    bool isBone{false};

    void setBone();

    bool isHidden() const { return flags & Flag_Hidden; }
    bool hasMeshCollision() const { return flags & Flag_MeshCollision; }
    bool hasBBoxCollision() const { return flags & Flag_BBoxCollision; }
    bool collisionActive() const { return flags & Flag_ActiveCollision; }
};

struct NiNode : Node
{
    NodeList children;
    NodeList effects;

    enum BSAnimFlags {
        AnimFlag_AutoPlay = 0x0020
    };
    enum BSParticleFlags {
        ParticleFlag_AutoPlay = 0x0020,
        ParticleFlag_LocalSpace = 0x0080
    };

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiGeometry : Node
{
    /* Possible flags:
        0x40 - mesh has no vertex normals ?

        Only flags included in 0x47 (ie. 0x01, 0x02, 0x04 and 0x40) have
        been observed so far.
    */

    struct MaterialData
    {
        std::vector<std::string> names;
        std::vector<int> extra;
        unsigned int active{0};
        bool needsUpdate{false};
        void read(NIFStream *nif);
    };

    NiGeometryDataPtr data;
    NiSkinInstancePtr skin;
    MaterialData material;
    BSShaderPropertyPtr shaderprop;
    NiAlphaPropertyPtr alphaprop;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiTriShape : NiGeometry {};
struct BSLODTriShape : NiTriShape
{
    unsigned int lod0, lod1, lod2;
    void read(NIFStream *nif) override;
};
struct NiTriStrips : NiGeometry {};
struct NiLines : NiGeometry {};
struct NiParticles : NiGeometry { };

struct NiCamera : Node
{
    struct Camera
    {
        unsigned short cameraFlags{0};

        // Camera frustrum
        float left, right, top, bottom, nearDist, farDist;

        // Viewport
        float vleft, vright, vtop, vbottom;

        // Level of detail modifier
        float LOD;

        // Orthographic projection usage flag
        bool orthographic{false};

        void read(NIFStream *nif);
    };
    Camera cam;

    void read(NIFStream *nif) override;
};

// A node used as the base to switch between child nodes, such as for LOD levels.
struct NiSwitchNode : public NiNode
{
    unsigned int switchFlags{0};
    unsigned int initialIndex{0};

    void read(NIFStream *nif) override;
};

struct NiLODNode : public NiSwitchNode
{
    osg::Vec3f lodCenter;

    struct LODRange
    {
        float minRange;
        float maxRange;
    };
    std::vector<LODRange> lodLevels;

    void read(NIFStream *nif) override;
};

struct NiFltAnimationNode : public NiSwitchNode
{
    float mDuration;
    enum Flags
    {
        Flag_Swing = 0x40
    };

    void read(NIFStream *nif) override;

    bool swing() const { return flags & Flag_Swing; }
};

// Abstract
struct NiAccumulator : Record
{
    void read(NIFStream *nif) override {}
};

// Node children sorters
struct NiClusterAccumulator : NiAccumulator {};
struct NiAlphaAccumulator : NiClusterAccumulator {};

struct NiSortAdjustNode : NiNode
{
    enum SortingMode
    {
        SortingMode_Inherit,
        SortingMode_Off,
        SortingMode_Subsort
    };

    int mMode;
    NiAccumulatorPtr mSubSorter;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

} // Namespace
#endif
