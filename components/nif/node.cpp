#include "node.hpp"

#include <components/misc/stringops.hpp>

#include "data.hpp"
#include "physics.hpp"
#include "property.hpp"

namespace Nif
{
    void NiBoundingVolume::read(NIFStream* nif)
    {
        type = nif->getUInt();
        switch (type)
        {
            case BASE_BV:
                break;
            case SPHERE_BV:
            {
                sphere.center = nif->getVector3();
                sphere.radius = nif->getFloat();
                break;
            }
            case BOX_BV:
            {
                box.center = nif->getVector3();
                box.axes = nif->getMatrix3();
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
                if (nif->getVersion() >= NIFStream::generateVersion(4,2,1,0))
                {
                    lozenge.extent0 = nif->getFloat();
                    lozenge.extent1 = nif->getFloat();
                }
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
                halfSpace.plane = osg::Plane(nif->getVector4());
                if (nif->getVersion() >= NIFStream::generateVersion(4,2,1,0))
                    halfSpace.origin = nif->getVector3();
                break;
            }
            default:
            {
                nif->file->fail("Unhandled NiBoundingVolume type: " + std::to_string(type));
            }
        }
    }

    void Node::read(NIFStream *nif)
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
            collision.read(nif);

        parents.clear();

        isBone = false;
    }

    void Node::post(NIFFile *nif)
    {
        Named::post(nif);
        props.post(nif);
        collision.post(nif);
    }

    void Node::setBone()
    {
        isBone = true;
    }

    void NiNode::read(NIFStream *nif)
    {
        Node::read(nif);
        children.read(nif);
        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            effects.read(nif);

        // Discard transformations for the root node, otherwise some meshes
        // occasionally get wrong orientation. Only for NiNode-s for now, but
        // can be expanded if needed.
        // FIXME: if node 0 is *not* the only root node, this must not happen.
        if (0 == recIndex && !Misc::StringUtils::ciEqual(name, "bip01"))
        {
            trafo = Nif::Transformation::getIdentity();
        }
    }

    void NiNode::post(NIFFile *nif)
    {
        Node::post(nif);
        children.post(nif);
        effects.post(nif);

        for (size_t i = 0; i < children.length(); i++)
        {
            // Why would a unique list of children contain empty refs?
            if (!children[i].empty())
                children[i]->parents.push_back(this);
        }
    }

    void NiGeometry::MaterialData::read(NIFStream *nif)
    {
        if (nif->getVersion() <= NIFStream::generateVersion(10,0,1,0))
            return;
        unsigned int num = 0;
        if (nif->getVersion() <= NIFStream::generateVersion(20,1,0,3))
            num = nif->getBoolean(); // Has Shader
        else if (nif->getVersion() >= NIFStream::generateVersion(20,2,0,5))
            num = nif->getUInt();
        if (num)
        {
            nif->getStrings(names, num);
            nif->getInts(extra, num);
        }
        if (nif->getVersion() >= NIFStream::generateVersion(20,2,0,5))
            active = nif->getUInt();
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS)
            needsUpdate = nif->getBoolean();
    }

    void NiGeometry::read(NIFStream *nif)
    {
        Node::read(nif);
        data.read(nif);
        skin.read(nif);
        material.read(nif);
        if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
        {
            shaderprop.read(nif);
            alphaprop.read(nif);
        }
    }

    void NiGeometry::post(NIFFile *nif)
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
        shaderprop.post(nif);
        alphaprop.post(nif);
        if (recType != RC_NiParticles && !skin.empty())
            nif->setUseSkinning(true);
    }

    void BSLODTriShape::read(NIFStream *nif)
    {
        NiTriShape::read(nif);
        lod0 = nif->getUInt();
        lod1 = nif->getUInt();
        lod2 = nif->getUInt();
    }

    void NiCamera::Camera::read(NIFStream *nif)
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

    void NiCamera::read(NIFStream *nif)
    {
        Node::read(nif);

        cam.read(nif);

        nif->getInt(); // -1
        nif->getInt(); // 0
        if (nif->getVersion() >= NIFStream::generateVersion(4,2,1,0))
            nif->getInt(); // 0
    }

    void NiSwitchNode::read(NIFStream *nif)
    {
        NiNode::read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
            switchFlags = nif->getUShort();
        initialIndex = nif->getUInt();
    }

    void NiLODNode::read(NIFStream *nif)
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

    void NiFltAnimationNode::read(NIFStream *nif)
    {
        NiSwitchNode::read(nif);
        mDuration = nif->getFloat();
    }

    void NiSortAdjustNode::read(NIFStream *nif)
    {
        NiNode::read(nif);
        mMode = nif->getInt();
        if (nif->getVersion() <= NIFStream::generateVersion(20,0,0,3))
            mSubSorter.read(nif);
    }

    void NiSortAdjustNode::post(NIFFile *nif)
    {
        NiNode::post(nif);
        mSubSorter.post(nif);
    }
}
