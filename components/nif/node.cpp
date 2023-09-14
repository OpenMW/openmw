#include "node.hpp"

#include <cstdint>

#include <components/misc/strings/algorithm.hpp>

#include "data.hpp"
#include "exception.hpp"
#include "physics.hpp"
#include "property.hpp"

namespace Nif
{

    void NiBoundingVolume::read(NIFStream* nif)
    {
        nif->read(type);
        switch (type)
        {
            case BASE_BV:
                break;
            case SPHERE_BV:
            {
                nif->read(mSphere);
                break;
            }
            case BOX_BV:
            {
                nif->read(box.center);
                nif->read(box.axes);
                nif->read(box.extents);
                break;
            }
            case CAPSULE_BV:
            {
                nif->read(mCapsule.mCenter);
                nif->read(mCapsule.mAxis);
                nif->read(mCapsule.mExtent);
                nif->read(mCapsule.mRadius);
                break;
            }
            case LOZENGE_BV:
            {
                nif->read(mLozenge.mRadius);
                if (nif->getVersion() >= NIFStream::generateVersion(4, 2, 1, 0))
                {
                    nif->read(mLozenge.mExtent0);
                    nif->read(mLozenge.mExtent1);
                }
                nif->read(mLozenge.mCenter);
                nif->read(mLozenge.mAxis0);
                nif->read(mLozenge.mAxis1);
                break;
            }
            case UNION_BV:
            {
                mChildren.resize(nif->get<uint32_t>());
                for (NiBoundingVolume& child : mChildren)
                    child.read(nif);
                break;
            }
            case HALFSPACE_BV:
            {
                mHalfSpace.mPlane = osg::Plane(nif->get<osg::Vec4f>());
                if (nif->getVersion() >= NIFStream::generateVersion(4, 2, 1, 0))
                    nif->read(mHalfSpace.mOrigin);
                break;
            }
            default:
            {
                throw Nif::Exception(
                    "Unhandled NiBoundingVolume type: " + std::to_string(type), nif->getFile().getFilename());
            }
        }
    }

    void NiAVObject::read(NIFStream* nif)
    {
        NiObjectNET::read(nif);

        if (nif->getBethVersion() <= 26)
            mFlags = nif->get<uint16_t>();
        else
            nif->read(mFlags);
        nif->read(mTransform.mTranslation);
        nif->read(mTransform.mRotation);
        nif->read(mTransform.mScale);
        if (nif->getVersion() <= NIFStream::generateVersion(4, 2, 2, 0))
            nif->read(mVelocity);
        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
            readRecordList(nif, mProperties);
        if (nif->getVersion() <= NIFStream::generateVersion(4, 2, 2, 0) && nif->get<bool>())
            mBounds.read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
            mCollision.read(nif);
    }

    void NiAVObject::post(Reader& nif)
    {
        NiObjectNET::post(nif);

        postRecordList(nif, mProperties);
        mCollision.post(nif);
    }

    void NiAVObject::setBone()
    {
        mIsBone = true;
    }

    void NiNode::read(NIFStream* nif)
    {
        NiAVObject::read(nif);

        readRecordList(nif, mChildren);
        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            readRecordList(nif, mEffects);

        // FIXME: stopgap solution until we figure out what Oblivion does if it does anything
        if (nif->getVersion() > NIFFile::NIFVersion::VER_MW && nif->getVersion() < NIFFile::NIFVersion::VER_BGS)
            return;

        // Discard transformations for the root node, otherwise some meshes
        // occasionally get wrong orientation. Only for NiNode-s for now, but
        // can be expanded if needed.
        // FIXME: if node 0 is *not* the only root node, this must not happen.
        // FIXME: doing this here is awful.
        // We want to do this on world scene graph level rather than local scene graph level.
        if (recIndex == 0 && !Misc::StringUtils::ciEqual(mName, "bip01"))
        {
            mTransform = Nif::NiTransform::getIdentity();
        }
    }

    void NiNode::post(Reader& nif)
    {
        NiAVObject::post(nif);

        postRecordList(nif, mChildren);
        postRecordList(nif, mEffects);

        for (auto& child : mChildren)
        {
            // Why would a unique list of children contain empty refs?
            if (!child.empty())
                child->mParents.push_back(this);
        }
    }

    void NiGeometry::MaterialData::read(NIFStream* nif)
    {
        if (nif->getVersion() < NIFStream::generateVersion(10, 0, 1, 0))
            return;
        if (nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 5))
            mNames.resize(nif->get<uint32_t>());
        else if (nif->getVersion() <= NIFStream::generateVersion(20, 1, 0, 3))
            mNames.resize(nif->get<bool>());
        nif->readVector(mNames, mNames.size());
        nif->readVector(mExtra, mNames.size());
        if (nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 5))
            nif->read(mActive);
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS)
            nif->read(mNeedsUpdate);
    }

    void NiGeometry::read(NIFStream* nif)
    {
        NiAVObject::read(nif);

        data.read(nif);
        skin.read(nif);
        mMaterial.read(nif);
        if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS
            && nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
        {
            shaderprop.read(nif);
            alphaprop.read(nif);
        }
    }

    void NiGeometry::post(Reader& nif)
    {
        NiAVObject::post(nif);

        data.post(nif);
        skin.post(nif);
        shaderprop.post(nif);
        alphaprop.post(nif);
        if (recType != RC_NiParticles && !skin.empty())
            nif.setUseSkinning(true);
    }

    void BSLODTriShape::read(NIFStream* nif)
    {
        NiTriShape::read(nif);

        nif->readArray(mLOD);
    }

    void NiCamera::read(NIFStream* nif)
    {
        NiAVObject::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mCameraFlags);
        nif->read(mLeft);
        nif->read(mRight);
        nif->read(mTop);
        nif->read(mBottom);
        nif->read(mNearDist);
        nif->read(mFarDist);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mOrthographic);
        nif->read(mVLeft);
        nif->read(mVRight);
        nif->read(mVTop);
        nif->read(mVBottom);
        nif->read(mLODAdjust);
        mScene.read(nif);
        nif->skip(4); // Unused
        if (nif->getVersion() >= NIFStream::generateVersion(4, 2, 1, 0))
            nif->skip(4); // Unused
    }

    void NiCamera::post(Reader& nif)
    {
        NiAVObject::post(nif);

        mScene.post(nif);
    }

    void NiSwitchNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(switchFlags);
        nif->read(initialIndex);
    }

    void NiLODNode::read(NIFStream* nif)
    {
        NiSwitchNode::read(nif);

        if (nif->getVersion() > NIFStream::generateVersion(10, 0, 1, 0))
        {
            nif->skip(4); // NiLODData, unsupported at the moment
            return;
        }

        if (nif->getVersion() >= NIFFile::NIFVersion::VER_MW)
            nif->read(lodCenter);

        lodLevels.resize(nif->get<uint32_t>());
        for (LODRange& level : lodLevels)
        {
            nif->read(level.minRange);
            nif->read(level.maxRange);
        }
    }

    void NiFltAnimationNode::read(NIFStream* nif)
    {
        NiSwitchNode::read(nif);

        nif->read(mDuration);
    }

    void NiSortAdjustNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        mMode = static_cast<SortingMode>(nif->get<uint32_t>());
        if (nif->getVersion() <= NIFStream::generateVersion(20, 0, 0, 3))
            mSubSorter.read(nif);
    }

    void NiSortAdjustNode::post(Reader& nif)
    {
        NiNode::post(nif);

        mSubSorter.post(nif);
    }

    void NiBillboardNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mMode = nif->get<uint16_t>() & 0x7;
        else
            mMode = (mFlags >> 5) & 0x3;
    }

    void NiDefaultAVObjectPalette::read(NIFStream* nif)
    {
        mScene.read(nif);
        uint32_t numObjects;
        nif->read(numObjects);
        for (uint32_t i = 0; i < numObjects; i++)
            mObjects[nif->getSizedString()].read(nif);
    }

    void NiDefaultAVObjectPalette::post(Reader& nif)
    {
        mScene.post(nif);
        for (auto& object : mObjects)
            object.second.post(nif);
    }

    void BSTreeNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        readRecordList(nif, mBones1);
        readRecordList(nif, mBones2);
    }

    void BSTreeNode::post(Reader& nif)
    {
        NiNode::post(nif);

        postRecordList(nif, mBones1);
        postRecordList(nif, mBones2);
    }

    void BSMultiBoundNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        mMultiBound.read(nif);
        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_SKY)
            nif->read(mType);
    }

    void BSMultiBoundNode::post(Reader& nif)
    {
        NiNode::post(nif);

        mMultiBound.post(nif);
    }

    void BSTriShape::read(NIFStream* nif)
    {
        NiAVObject::read(nif);

        nif->read(mBoundingSphere);
        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            nif->readArray(mBoundMinMax);

        mSkin.read(nif);
        mShaderProperty.read(nif);
        mAlphaProperty.read(nif);
        mVertDesc.read(nif);

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
            mTriangles.resize(nif->get<uint32_t>() * 3);
        else
            mTriangles.resize(nif->get<uint16_t>() * 3);
        mVertData.resize(nif->get<uint16_t>());
        nif->read(mDataSize);
        if (mDataSize > 0)
        {
            for (auto& vertex : mVertData)
                vertex.read(nif, mVertDesc.mFlags);
            nif->readVector(mTriangles, mTriangles.size());
        }

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_SSE)
        {
            nif->read(mParticleDataSize);
            if (mParticleDataSize > 0)
            {
                nif->readVector(mParticleVerts, mVertData.size() * 3);
                nif->readVector(mParticleNormals, mVertData.size() * 3);
                nif->readVector(mParticleTriangles, mTriangles.size());
            }
        }
    }

    void BSTriShape::post(Reader& nif)
    {
        NiAVObject::post(nif);

        mSkin.post(nif);
        mShaderProperty.post(nif);
        mAlphaProperty.post(nif);
    }

    void BSVertexDesc::read(NIFStream* nif)
    {
        uint64_t data;
        nif->read(data);
        mVertexDataSize = (data & 0xF) >> 0x00;
        mDynamicVertexSize = (data & 0xF0) >> 0x04;
        mUV1Offset = (data & 0xF00) >> 0x08;
        mUV2Offset = (data & 0xF000) >> 0x0C;
        mNormalOffset = (data & 0xF0000) >> 0x10;
        mTangentOffset = (data & 0xF00000) >> 0x14;
        mColorOffset = (data & 0xF000000) >> 0x18;
        mSkinningDataOffset = (data & 0xF0000000) >> 0x1C;
        mLandscapeDataOffset = (data & 0xF00000000) >> 0x20;
        mEyeDataOffset = (data & 0xF000000000) >> 0x24;
        mFlags = (data & 0xFFF00000000000) >> 0x2C;
    }

    void BSVertexData::read(NIFStream* nif, uint16_t flags)
    {
        uint16_t vertexFlag = flags & BSVertexDesc::VertexAttribute::Vertex;
        uint16_t tangentsFlag = flags & BSVertexDesc::VertexAttribute::Tangents;
        uint16_t UVsFlag = flags & BSVertexDesc::VertexAttribute::UVs;
        uint16_t normalsFlag = flags & BSVertexDesc::VertexAttribute::Normals;

        if (vertexFlag == BSVertexDesc::VertexAttribute::Vertex)
        {
            nif->read(mVertex);
        }

        if ((vertexFlag | tangentsFlag)
            == (BSVertexDesc::VertexAttribute::Vertex | BSVertexDesc::VertexAttribute::Tangents))
        {
            nif->read(mBitangentX);
        }

        if ((vertexFlag | tangentsFlag) == BSVertexDesc::VertexAttribute::Vertex)
        {
            nif->read(mUnusedW);
        }

        if (UVsFlag == BSVertexDesc::VertexAttribute::UVs)
        {
            nif->readArray(mUV);
        }

        if (normalsFlag)
        {
            nif->readArray(mNormal);
            nif->read(mBitangentY);
        }

        if ((normalsFlag | tangentsFlag)
            == (BSVertexDesc::VertexAttribute::Normals | BSVertexDesc::VertexAttribute::Tangents))
        {
            nif->readArray(mTangent);
            nif->read(mBitangentZ);
        }

        if (flags & BSVertexDesc::VertexAttribute::Vertex_Colors)
        {
            nif->readArray(mVertColors);
        }

        if (flags & BSVertexDesc::VertexAttribute::Skinned)
        {
            nif->readArray(mBoneWeights);
            nif->readArray(mBoneIndices);
        }

        if (flags & BSVertexDesc::VertexAttribute::Eye_Data)
        {
            nif->read(mEyeData);
        }
    }

    void BSValueNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        nif->read(mValue);
        nif->read(mValueFlags);
    }

    void BSOrderedNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        nif->read(mAlphaSortBound);
        nif->read(mStaticBound);
    }

    void BSRangeNode::read(NIFStream* nif)
    {
        NiNode::read(nif);

        nif->read(mMin);
        nif->read(mMax);
        nif->read(mCurrent);
    }

}
