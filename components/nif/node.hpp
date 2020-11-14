#ifndef OPENMW_COMPONENTS_NIF_NODE_HPP
#define OPENMW_COMPONENTS_NIF_NODE_HPP

#include "controlled.hpp"
#include "extra.hpp"
#include "data.hpp"
#include "property.hpp"
#include "niftypes.hpp"
#include "controller.hpp"
#include "base.hpp"

#include <components/misc/stringops.hpp>

namespace Nif
{

struct NiNode;

struct NiBoundingVolume
{
    enum Type
    {
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
        Matrix3 axis;
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
        osg::Vec3f center, normal;
    };

    unsigned int type;
    NiSphereBV sphere;
    NiBoxBV box;
    NiCapsuleBV capsule;
    NiLozengeBV lozenge;
    std::vector<NiBoundingVolume> children;
    NiHalfSpaceBV plane;
    void read(NIFStream* nif)
    {
        type = nif->getUInt();
        switch (type)
        {
            case SPHERE_BV:
            {
                sphere.center = nif->getVector3();
                sphere.radius = nif->getFloat();
                break;
            }
            case BOX_BV:
            {
                box.center = nif->getVector3();
                box.axis = nif->getMatrix3();
                box.extents = nif->getVector3();
                break;
            }
            case CAPSULE_BV:
            {
                capsule.center = nif->getVector3();
                capsule.axis = nif->getVector3();
                capsule.extent = nif->getFloat();
                capsule.radius = nif->getFloat();
                break;
            }
            case LOZENGE_BV:
            {
                lozenge.radius = nif->getFloat();
                lozenge.extent0 = nif->getFloat();
                lozenge.extent1 = nif->getFloat();
                lozenge.center = nif->getVector3();
                lozenge.axis0 = nif->getVector3();
                lozenge.axis1 = nif->getVector3();
                break;
            }
            case UNION_BV:
            {
                unsigned int numChildren = nif->getUInt();
                if (numChildren == 0)
                    break;
                children.resize(numChildren);
                for (NiBoundingVolume& child : children)
                    child.read(nif);
                break;
            }
            case HALFSPACE_BV:
            {
                plane.center = nif->getVector3();
                plane.normal = nif->getVector3();
                break;
            }
            default:
            {
                std::stringstream error;
                error << "Unhandled NiBoundingVolume type: " << type;
                nif->file->fail(error.str());
            }
        }
    }
};

/** A Node is an object that's part of the main NIF tree. It has
    parent node (unless it's the root), and transformation (location
    and rotation) relative to it's parent.
 */
class Node : public Named
{
public:
    // Node flags. Interpretation depends somewhat on the type of node.
    unsigned int flags;
    Transformation trafo;
    osg::Vec3f velocity; // Unused? Might be a run-time game state
    PropertyList props;

    // Bounding box info
    bool hasBounds{false};
    NiBoundingVolume bounds;

    void read(NIFStream *nif) override
    {
        Named::read(nif);

        flags = nif->getBethVersion() <= 26 ? nif->getUShort() : nif->getUInt();
        trafo = nif->getTrafo();
        if (nif->getVersion() <= NIFStream::generateVersion(4,2,2,0))
            velocity = nif->getVector3();
        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
            props.read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(4,2,2,0))
            hasBounds = nif->getBoolean();
        if (hasBounds)
            bounds.read(nif);
        // Reference to the collision object in Gamebryo files.
        if (nif->getVersion() >= NIFStream::generateVersion(10,0,1,0))
            nif->skip(4);

        parent = nullptr;

        isBone = false;
    }

    void post(NIFFile *nif) override
    {
        Named::post(nif);
        props.post(nif);
    }

    // Parent node, or nullptr for the root node. As far as I'm aware, only
    // NiNodes (or types derived from NiNodes) can be parents.
    NiNode *parent;

    bool isBone;

    void setBone()
    {
        isBone = true;
    }
};

struct NiNode : Node
{
    NodeList children;
    NodeList effects;

    enum Flags {
        Flag_Hidden = 0x0001,
        Flag_MeshCollision = 0x0002,
        Flag_BBoxCollision = 0x0004,
        Flag_ActiveCollision = 0x0020
    };
    enum BSAnimFlags {
        AnimFlag_AutoPlay = 0x0020
    };
    enum BSParticleFlags {
        ParticleFlag_AutoPlay = 0x0020,
        ParticleFlag_LocalSpace = 0x0080
    };
    enum ControllerFlags {
        ControllerFlag_Active = 0x8
    };

    void read(NIFStream *nif) override
    {
        Node::read(nif);
        children.read(nif);
        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            effects.read(nif);

        // Discard transformations for the root node, otherwise some meshes
        // occasionally get wrong orientation. Only for NiNode-s for now, but
        // can be expanded if needed.
        if (0 == recIndex && !Misc::StringUtils::ciEqual(name, "bip01"))
        {
            static_cast<Nif::Node*>(this)->trafo = Nif::Transformation::getIdentity();
        }
    }

    void post(NIFFile *nif) override
    {
        Node::post(nif);
        children.post(nif);
        effects.post(nif);

        for(size_t i = 0;i < children.length();i++)
        {
            // Why would a unique list of children contain empty refs?
            if(!children[i].empty())
                children[i]->parent = this;
        }
    }
};

struct NiGeometry : Node
{
    struct MaterialData
    {
        std::vector<std::string> materialNames;
        std::vector<int> materialExtraData;
        unsigned int activeMaterial{0};
        bool materialNeedsUpdate{false};
        void read(NIFStream *nif)
        {
            if (nif->getVersion() <= NIFStream::generateVersion(10,0,1,0))
                return;
            unsigned int numMaterials = 0;
            if (nif->getVersion() <= NIFStream::generateVersion(20,1,0,3))
                numMaterials = nif->getBoolean(); // Has Shader
            else if (nif->getVersion() >= NIFStream::generateVersion(20,2,0,5))
                numMaterials = nif->getUInt();
            if (numMaterials)
            {
                nif->getStrings(materialNames, numMaterials);
                nif->getInts(materialExtraData, numMaterials);
            }
            if (nif->getVersion() >= NIFStream::generateVersion(20,2,0,5))
                activeMaterial = nif->getUInt();
            if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS)
            {
                materialNeedsUpdate = nif->getBoolean();
                if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
                    nif->skip(8);
            }
        }
    };

    NiSkinInstancePtr skin;
    MaterialData materialData;
};

struct NiTriShape : NiGeometry
{
    /* Possible flags:
        0x40 - mesh has no vertex normals ?

        Only flags included in 0x47 (ie. 0x01, 0x02, 0x04 and 0x40) have
        been observed so far.
    */

    NiTriShapeDataPtr data;

    void read(NIFStream *nif) override
    {
        Node::read(nif);
        data.read(nif);
        skin.read(nif);
        materialData.read(nif);
    }

    void post(NIFFile *nif) override
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
        if (!skin.empty())
            nif->setUseSkinning(true);
    }
};

struct NiTriStrips : NiGeometry
{
    NiTriStripsDataPtr data;

    void read(NIFStream *nif) override
    {
        Node::read(nif);
        data.read(nif);
        skin.read(nif);
        materialData.read(nif);
    }

    void post(NIFFile *nif) override
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
        if (!skin.empty())
            nif->setUseSkinning(true);
    }
};

struct NiLines : NiGeometry
{
    NiLinesDataPtr data;

    void read(NIFStream *nif) override
    {
        Node::read(nif);
        data.read(nif);
        skin.read(nif);
    }

    void post(NIFFile *nif) override
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
        if (!skin.empty())
            nif->setUseSkinning(true);
    }
};

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

        void read(NIFStream *nif)
        {
            if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
                cameraFlags = nif->getUShort();
            left = nif->getFloat();
            right = nif->getFloat();
            top = nif->getFloat();
            bottom = nif->getFloat();
            nearDist = nif->getFloat();
            farDist = nif->getFloat();
            if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
                orthographic = nif->getBoolean();
            vleft = nif->getFloat();
            vright = nif->getFloat();
            vtop = nif->getFloat();
            vbottom = nif->getFloat();

            LOD = nif->getFloat();
        }
    };
    Camera cam;

    void read(NIFStream *nif) override
    {
        Node::read(nif);

        cam.read(nif);

        nif->getInt(); // -1
        nif->getInt(); // 0
        if (nif->getVersion() >= NIFStream::generateVersion(4,2,1,0))
            nif->getInt(); // 0
    }
};

struct NiAutoNormalParticles : Node
{
    NiAutoNormalParticlesDataPtr data;

    void read(NIFStream *nif) override
    {
        Node::read(nif);
        data.read(nif);
        nif->getInt(); // -1
    }

    void post(NIFFile *nif) override
    {
        Node::post(nif);
        data.post(nif);
    }
};

struct NiRotatingParticles : Node
{
    NiRotatingParticlesDataPtr data;

    void read(NIFStream *nif) override
    {
        Node::read(nif);
        data.read(nif);
        nif->getInt(); // -1
    }

    void post(NIFFile *nif) override
    {
        Node::post(nif);
        data.post(nif);
    }
};

// A node used as the base to switch between child nodes, such as for LOD levels.
struct NiSwitchNode : public NiNode
{
    unsigned int switchFlags{0};
    unsigned int initialIndex;

    void read(NIFStream *nif) override
    {
        NiNode::read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
            switchFlags = nif->getUShort();
        initialIndex = nif->getUInt();
    }
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

    void read(NIFStream *nif) override
    {
        NiSwitchNode::read(nif);
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_MW && nif->getVersion() <= NIFStream::generateVersion(10,0,1,0))
            lodCenter = nif->getVector3();
        else if (nif->getVersion() > NIFStream::generateVersion(10,0,1,0))
        {
            nif->skip(4); // NiLODData, unsupported at the moment
            return;
        }

        unsigned int numLodLevels = nif->getUInt();
        for (unsigned int i=0; i<numLodLevels; ++i)
        {
            LODRange r;
            r.minRange = nif->getFloat();
            r.maxRange = nif->getFloat();
            lodLevels.push_back(r);
        }
    }
};

} // Namespace
#endif
