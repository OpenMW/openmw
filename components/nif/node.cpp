#include "node.hpp"

#include <cstdint>

#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <components/misc/convert.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/resource/bulletshape.hpp>

#include "data.hpp"
#include "exception.hpp"
#include "physics.hpp"
#include "property.hpp"

namespace
{

    void triBasedGeomToBtTriangleMesh(btTriangleMesh& mesh, const Nif::NiTriBasedGeomData& data)
    {
        // FIXME: copying vertices/indices individually is unreasonable
        const std::vector<osg::Vec3f>& vertices = data.mVertices;
        mesh.preallocateVertices(static_cast<int>(vertices.size()));
        for (const osg::Vec3f& vertex : vertices)
            mesh.findOrAddVertex(Misc::Convert::toBullet(vertex), false);

        mesh.preallocateIndices(static_cast<int>(data.mNumTriangles) * 3);
    }

    void trianglesToBtTriangleMesh(btTriangleMesh& mesh, const std::vector<unsigned short>& triangles)
    {
        for (std::size_t i = 0; i < triangles.size(); i += 3)
            mesh.addTriangleIndices(triangles[i + 0], triangles[i + 1], triangles[i + 2]);
    }

    void stripsToBtTriangleMesh(btTriangleMesh& mesh, const std::vector<std::vector<unsigned short>>& strips)
    {
        for (const auto& strip : strips)
        {
            if (strip.size() < 3)
                continue;

            unsigned short a;
            unsigned short b = strip[0];
            unsigned short c = strip[1];
            for (size_t i = 2; i < strip.size(); i++)
            {
                a = b;
                b = c;
                c = strip[i];
                if (a == b || b == c || a == c)
                    continue;
                if (i % 2 == 0)
                    mesh.addTriangleIndices(a, b, c);
                else
                    mesh.addTriangleIndices(a, c, b);
            }
        }
    }

}

namespace Nif
{

    void BoundingVolume::read(NIFStream* nif)
    {
        nif->read(mType);
        switch (mType)
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
                nif->read(mBox.mCenter);
                nif->read(mBox.mAxes);
                nif->read(mBox.mExtents);
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
                nif->readVectorOfRecords<uint32_t>(mChildren);
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
                    "Unhandled BoundingVolume type: " + std::to_string(mType), nif->getFile().getFilename());
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

        mData.read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            mSkin.read(nif);
        mMaterial.read(nif);
        if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS
            && nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
        {
            mShaderProperty.read(nif);
            mAlphaProperty.read(nif);
        }
    }

    void NiGeometry::post(Reader& nif)
    {
        NiAVObject::post(nif);

        mData.post(nif);
        mSkin.post(nif);
        mShaderProperty.post(nif);
        mAlphaProperty.post(nif);
        if (recType != RC_NiParticles && !mSkin.empty())
            nif.setUseSkinning(true);

        if (!mData.empty())
        {
            switch (recType)
            {
                case RC_NiTriShape:
                case RC_BSLODTriShape:
                case RC_BSSegmentedTriShape:
                    if (mData->recType != RC_NiTriShapeData)
                        mData = NiGeometryDataPtr(nullptr);
                    break;
                case RC_NiTriStrips:
                    if (mData->recType != RC_NiTriStripsData)
                        mData = NiGeometryDataPtr(nullptr);
                    break;
                case RC_NiParticles:
                    if (mData->recType != RC_NiParticlesData)
                        mData = NiGeometryDataPtr(nullptr);
                    break;
                case RC_NiLines:
                    if (mData->recType != RC_NiLinesData)
                        mData = NiGeometryDataPtr(nullptr);
                    break;
                default:
                    break;
            }
        }
    }

    std::unique_ptr<btCollisionShape> NiTriShape::getCollisionShape() const
    {
        if (mData.empty() || mData->mVertices.empty())
            return nullptr;

        std::vector<const std::vector<unsigned short>*> triangleLists;
        std::vector<const std::vector<std::vector<unsigned short>>*> stripsLists;
        auto data = static_cast<const NiTriShapeData*>(mData.getPtr());
        const Nif::NiSkinPartition* partitions = nullptr;
        if (!mSkin.empty())
            partitions = mSkin->getPartitions();

        if (partitions)
        {
            triangleLists.reserve(partitions->mPartitions.size());
            stripsLists.reserve(partitions->mPartitions.size());
            for (auto& partition : partitions->mPartitions)
            {
                triangleLists.push_back(&partition.mTrueTriangles);
                stripsLists.push_back(&partition.mTrueStrips);
            }
        }
        else if (data->mNumTriangles != 0)
            triangleLists.push_back(&data->mTriangles);

        // This makes a perhaps dangerous assumption that NiSkinPartition will never have more than 65536 triangles.
        auto mesh = std::make_unique<btTriangleMesh>();
        triBasedGeomToBtTriangleMesh(*mesh, *data);
        for (const auto triangles : triangleLists)
            trianglesToBtTriangleMesh(*mesh, *triangles);
        for (const auto strips : stripsLists)
            stripsToBtTriangleMesh(*mesh, *strips);

        if (mesh->getNumTriangles() == 0)
            return nullptr;

        auto shape = std::make_unique<Resource::TriangleMeshShape>(mesh.get(), true);
        std::ignore = mesh.release();

        return shape;
    }

    std::unique_ptr<btCollisionShape> NiTriStrips::getCollisionShape() const
    {
        if (mData.empty() || mData->mVertices.empty())
            return nullptr;

        std::vector<const std::vector<unsigned short>*> triangleLists;
        std::vector<const std::vector<std::vector<unsigned short>>*> stripsLists;
        auto data = static_cast<const NiTriStripsData*>(mData.getPtr());
        const Nif::NiSkinPartition* partitions = nullptr;
        if (!mSkin.empty())
            partitions = mSkin->getPartitions();

        if (partitions)
        {
            triangleLists.reserve(partitions->mPartitions.size());
            stripsLists.reserve(partitions->mPartitions.size());
            for (auto& partition : partitions->mPartitions)
            {
                triangleLists.push_back(&partition.mTrueTriangles);
                stripsLists.push_back(&partition.mTrueStrips);
            }
        }
        else if (data->mNumTriangles != 0)
            stripsLists.push_back(&data->mStrips);

        auto mesh = std::make_unique<btTriangleMesh>();
        triBasedGeomToBtTriangleMesh(*mesh, *data);
        for (const auto triangles : triangleLists)
            trianglesToBtTriangleMesh(*mesh, *triangles);
        for (const auto strips : stripsLists)
            stripsToBtTriangleMesh(*mesh, *strips);

        if (mesh->getNumTriangles() == 0)
            return nullptr;

        auto shape = std::make_unique<Resource::TriangleMeshShape>(mesh.get(), true);
        std::ignore = mesh.release();

        return shape;
    }

    std::unique_ptr<btCollisionShape> NiLines::getCollisionShape() const
    {
        return nullptr;
    }

    std::unique_ptr<btCollisionShape> NiParticles::getCollisionShape() const
    {
        return nullptr;
    }

    void BSSegmentedTriShape::SegmentData::read(NIFStream* nif)
    {
        nif->read(mFlags);
        nif->read(mStartIndex);
        nif->read(mNumTriangles);
    }

    void BSSegmentedTriShape::read(NIFStream* nif)
    {
        NiTriShape::read(nif);

        mSegments.resize(nif->get<uint32_t>());
        for (SegmentData& segment : mSegments)
            segment.read(nif);
    }

    void BSLODTriShape::read(NIFStream* nif)
    {
        NiTriBasedGeom::read(nif);

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
            nif->read(mSwitchFlags);
        nif->read(mInitialIndex);
    }

    void NiLODNode::LODRange::read(NIFStream* nif)
    {
        nif->read(mMinRange);
        nif->read(mMaxRange);
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
            nif->read(mLODCenter);

        nif->readVectorOfRecords<uint32_t>(mLODLevels);
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
            mCullingType = static_cast<BSCPCullingType>(nif->get<uint32_t>());
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
        if (!mSkin.empty())
            nif.setUseSkinning(true);
    }

    void BSDynamicTriShape::read(NIFStream* nif)
    {
        BSTriShape::read(nif);

        nif->read(mDynamicDataSize);
        // nifly style.
        // Consider complaining if mDynamicDataSize * 16 != mVertData.size()?
        nif->readVector(mDynamicData, mVertData.size());
    }

    void BSMeshLODTriShape::read(NIFStream* nif)
    {
        BSTriShape::read(nif);

        nif->readArray(mLOD);
    }

    void BSSubIndexTriShape::SubSegment::read(NIFStream* nif)
    {
        nif->read(mStartIndex);
        nif->read(mNumPrimitives);
        nif->read(mArrayIndex);
        nif->skip(4); // Unknown
    }

    void BSSubIndexTriShape::Segment::read(NIFStream* nif)
    {
        nif->read(mStartIndex);
        nif->read(mNumPrimitives);
        nif->read(mParentArrayIndex);
        mSubSegments.resize(nif->get<uint32_t>());
        for (SubSegment& subsegment : mSubSegments)
            subsegment.read(nif);
    }

    void BSSubIndexTriShape::SubSegmentDataRecord::read(NIFStream* nif)
    {
        nif->read(mUserSlotID);
        nif->read(mMaterial);
        nif->readVector(mExtraData, nif->get<uint32_t>());
    }

    void BSSubIndexTriShape::SubSegmentData::read(NIFStream* nif)
    {
        uint32_t numArrayIndices;
        nif->read(numArrayIndices);
        mDataRecords.resize(nif->get<uint32_t>());
        nif->readVector(mArrayIndices, numArrayIndices);
        for (SubSegmentDataRecord& dataRecord : mDataRecords)
            dataRecord.read(nif);
        mSSFFile = nif->getSizedString(nif->get<uint16_t>());
    }

    void BSSubIndexTriShape::Segmentation::read(NIFStream* nif)
    {
        nif->read(mNumPrimitives);
        mSegments.resize(nif->get<uint32_t>());
        nif->read(mNumTotalSegments);
        for (Segment& segment : mSegments)
            segment.read(nif);

        if (mSegments.size() < mNumTotalSegments)
            mSubSegmentData.read(nif);
    }

    void BSSubIndexTriShape::read(NIFStream* nif)
    {
        BSTriShape::read(nif);

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_SSE)
        {
            mSegments.resize(nif->get<uint32_t>());
            for (BSSegmentedTriShape::SegmentData& segment : mSegments)
                segment.read(nif);
        }
        else if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4 && mDataSize > 0)
            mSegmentation.read(nif);
    }

    void BSVertexDesc::read(NIFStream* nif)
    {
        uint64_t data;
        nif->read(data);
        mVertexDataSize = (data & 0xF) >> 0x00;
        mDynamicVertexSize = (data & 0xF0) >> 0x04;
        mUV1Offset = (data & 0xF00) >> 0x08;
        mUV2Offset = (data & 0xF000) >> 0x0C;
        mNormalOffset = static_cast<uint8_t>((data & 0xF0000) >> 0x10);
        mTangentOffset = static_cast<uint8_t>((data & 0xF00000) >> 0x14);
        mColorOffset = (data & 0xF000000) >> 0x18;
        mSkinningDataOffset = (data & 0xF0000000) >> 0x1C;
        mLandscapeDataOffset = static_cast<uint8_t>((data & 0xF00000000) >> 0x20);
        mEyeDataOffset = static_cast<uint8_t>((data & 0xF000000000) >> 0x24);
        mFlags = static_cast<uint16_t>((data & 0xFFF00000000000) >> 0x2C);
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_SSE)
            mFlags |= BSVertexDesc::VertexAttribute::Full_Precision;
    }

    void BSVertexData::read(NIFStream* nif, uint16_t flags)
    {
        bool fullPrecision = flags & BSVertexDesc::VertexAttribute::Full_Precision;
        bool hasVertex = flags & BSVertexDesc::VertexAttribute::Vertex;
        bool hasTangent = flags & BSVertexDesc::VertexAttribute::Tangents;
        bool hasUV = flags & BSVertexDesc::VertexAttribute::UVs;
        bool hasNormal = flags & BSVertexDesc::VertexAttribute::Normals;
        bool hasVertexColor = flags & BSVertexDesc::VertexAttribute::Vertex_Colors;
        bool hasSkinData = flags & BSVertexDesc::VertexAttribute::Skinned;
        bool hasEyeData = flags & BSVertexDesc::VertexAttribute::Eye_Data;

        if (hasVertex)
        {
            if (fullPrecision)
                nif->read(mVertex);
            else
                nif->readArray(mHalfVertex);
        }

        if (hasUV)
            nif->readArray(mUV);

        if (hasNormal)
        {
            nif->readArray(mNormal);
            if (hasTangent)
                nif->readArray(mTangent);
        }

        if (hasVertexColor)
            nif->readArray(mVertColor);

        if (hasSkinData)
        {
            nif->readArray(mBoneWeights);
            nif->readArray(mBoneIndices);
        }

        if (hasEyeData)
            nif->read(mEyeData);
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

    void BSResourceID::read(NIFStream* nif)
    {
        nif->read(mFileHash);
        nif->readArray(mExtension);
        nif->read(mDirectoryHash);
    }

    void BSDistantObjectInstance::read(NIFStream* nif)
    {
        mResourceID.read(nif);
        nif->skip(12 * nif->get<uint32_t>()); // Unknown data
        mTransforms.resize(nif->get<uint32_t>());
        for (osg::Matrixf& transform : mTransforms)
        {
            std::array<float, 16> mat;
            nif->readArray(mat);
            transform.set(mat.data());
        }
    }

    void BSShaderTextureArray::read(NIFStream* nif)
    {
        nif->skip(1); // Unknown
        mTextureArrays.resize(nif->get<uint32_t>());
        for (std::vector<std::string>& textureArray : mTextureArrays)
            nif->getSizedStrings(textureArray, nif->get<uint32_t>());
    }

    void BSDistantObjectInstancedNode::read(NIFStream* nif)
    {
        BSMultiBoundNode::read(nif);

        mInstances.resize(nif->get<uint32_t>());
        for (BSDistantObjectInstance& instance : mInstances)
            instance.read(nif);
        for (BSShaderTextureArray& textureArray : mShaderTextureArrays)
            textureArray.read(nif);
    }

}
