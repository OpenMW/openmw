#include "bulletnifloader.hpp"

#include <cassert>
#include <sstream>
#include <tuple>
#include <variant>
#include <vector>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <components/debug/debuglog.hpp>

#include <components/misc/convert.hpp>

#include <components/misc/strings/algorithm.hpp>

#include <components/nif/data.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/nifstream.hpp>
#include <components/nif/node.hpp>
#include <components/nif/parent.hpp>

#include <components/files/conversion.hpp>

namespace
{

    bool pathFileNameStartsWithX(const std::string& path)
    {
        const std::size_t slashpos = path.find_last_of("/\\");
        const std::size_t letterPos = slashpos == std::string::npos ? 0 : slashpos + 1;
        return letterPos < path.size() && (path[letterPos] == 'x' || path[letterPos] == 'X');
    }

    bool isTypeNiGeometry(int type)
    {
        switch (type)
        {
            case Nif::RC_NiTriShape:
            case Nif::RC_NiTriStrips:
            case Nif::RC_BSLODTriShape:
            case Nif::RC_BSSegmentedTriShape:
                return true;
        }
        return false;
    }

    bool isTypeTriShape(int type)
    {
        switch (type)
        {
            case Nif::RC_NiTriShape:
            case Nif::RC_BSLODTriShape:
            case Nif::RC_BSSegmentedTriShape:
                return true;
        }

        return false;
    }

    void prepareTriangleMesh(btTriangleMesh& mesh, const Nif::NiTriBasedGeomData& data)
    {
        // FIXME: copying vertices/indices individually is unreasonable
        const std::vector<osg::Vec3f>& vertices = data.mVertices;
        mesh.preallocateVertices(static_cast<int>(vertices.size()));
        for (const osg::Vec3f& vertex : vertices)
            mesh.findOrAddVertex(Misc::Convert::toBullet(vertex), false);

        mesh.preallocateIndices(static_cast<int>(data.mNumTriangles) * 3);
    }

    void fillTriangleMesh(btTriangleMesh& mesh, const Nif::NiTriShapeData& data)
    {
        prepareTriangleMesh(mesh, data);
        const std::vector<unsigned short>& triangles = data.mTriangles;
        for (std::size_t i = 0; i < triangles.size(); i += 3)
            mesh.addTriangleIndices(triangles[i + 0], triangles[i + 1], triangles[i + 2]);
    }

    void fillTriangleMesh(btTriangleMesh& mesh, const Nif::NiTriStripsData& data)
    {
        prepareTriangleMesh(mesh, data);
        for (const std::vector<unsigned short>& strip : data.mStrips)
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

    template <class Function>
    auto handleNiGeometry(const Nif::NiGeometry& geometry, Function&& function)
        -> decltype(function(static_cast<const Nif::NiTriShapeData&>(geometry.mData.get())))
    {
        if (isTypeTriShape(geometry.recType))
        {
            auto data = static_cast<const Nif::NiTriShapeData*>(geometry.mData.getPtr());
            if (data->mTriangles.empty())
                return {};

            return function(static_cast<const Nif::NiTriShapeData&>(*data));
        }

        if (geometry.recType == Nif::RC_NiTriStrips)
        {
            auto data = static_cast<const Nif::NiTriStripsData*>(geometry.mData.getPtr());
            if (data->mStrips.empty())
                return {};

            return function(static_cast<const Nif::NiTriStripsData&>(*data));
        }

        return {};
    }

    std::unique_ptr<btTriangleMesh> makeChildMesh(const Nif::NiGeometry& geometry)
    {
        return handleNiGeometry(geometry, [&](const auto& data) {
            auto mesh = std::make_unique<btTriangleMesh>();
            fillTriangleMesh(*mesh, data);
            return mesh;
        });
    }

}

namespace NifBullet
{

    osg::ref_ptr<Resource::BulletShape> BulletNifLoader::load(Nif::FileView nif)
    {
        mShape = new Resource::BulletShape;

        mCompoundShape.reset();
        mAvoidCompoundShape.reset();

        mShape->mFileHash = nif.getHash();

        const size_t numRoots = nif.numRoots();
        std::vector<const Nif::NiAVObject*> roots;
        for (size_t i = 0; i < numRoots; ++i)
        {
            const Nif::Record* r = nif.getRoot(i);
            if (!r)
                continue;
            const Nif::NiAVObject* node = dynamic_cast<const Nif::NiAVObject*>(r);
            if (node)
                roots.emplace_back(node);
        }
        mShape->mFileName = Files::pathToUnicodeString(nif.getFilename());
        if (roots.empty())
        {
            warn("Found no root nodes in NIF file " + mShape->mFileName);
            return mShape;
        }

        for (const Nif::NiAVObject* node : roots)
        {
            // Try to find a valid bounding box first. If one's found for any root node, use that.
            if (findBoundingBox(*node))
            {
                const btVector3 extents = Misc::Convert::toBullet(mShape->mCollisionBox.mExtents);
                const btVector3 center = Misc::Convert::toBullet(mShape->mCollisionBox.mCenter);
                auto compound = std::make_unique<btCompoundShape>();
                auto boxShape = std::make_unique<btBoxShape>(extents);
                btTransform transform = btTransform::getIdentity();
                transform.setOrigin(center);
                compound->addChildShape(transform, boxShape.get());
                std::ignore = boxShape.release();

                mShape->mCollisionShape.reset(compound.release());
                return mShape;
            }
        }

        HandleNodeArgs args;

        // files with the name convention xmodel.nif usually have keyframes stored in a separate file xmodel.kf (see
        // Animation::addAnimSource). assume all nodes in the file will be animated
        // TODO: investigate whether this should and could be optimized.
        args.mAnimated = pathFileNameStartsWithX(mShape->mFileName);

        // If there's no bounding box, we'll have to generate a Bullet collision shape
        // from the collision data present in every root node.
        for (const Nif::NiAVObject* node : roots)
            handleRoot(nif, *node, args);

        if (mCompoundShape)
            mShape->mCollisionShape = std::move(mCompoundShape);

        if (mAvoidCompoundShape)
            mShape->mAvoidCollisionShape = std::move(mAvoidCompoundShape);

        return mShape;
    }

    // Find a boundingBox in the node hierarchy.
    // Return: use bounding box for collision?
    bool BulletNifLoader::findBoundingBox(const Nif::NiAVObject& node)
    {
        unsigned int type = node.mBounds.mType;
        switch (type)
        {
            case Nif::BoundingVolume::Type::BASE_BV:
                break;
            case Nif::BoundingVolume::Type::BOX_BV:
                mShape->mCollisionBox.mExtents = node.mBounds.mBox.mExtents;
                mShape->mCollisionBox.mCenter = node.mBounds.mBox.mCenter;
                break;
            default:
            {
                std::stringstream warning;
                warning << "Unsupported BoundingVolume type " << type << " in node " << node.recIndex;
                warning << " in file " << mShape->mFileName;
                warn(warning.str());
            }
        }

        if (type != Nif::BoundingVolume::Type::BASE_BV && node.hasBBoxCollision())
            return true;

        if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&node))
        {
            for (const auto& child : ninode->mChildren)
                if (!child.empty() && findBoundingBox(child.get()))
                    return true;
        }
        return false;
    }

    void BulletNifLoader::handleRoot(Nif::FileView nif, const Nif::NiAVObject& node, HandleNodeArgs args)
    {
        // Gamebryo/Bethbryo meshes
        if (nif.getVersion() >= Nif::NIFStream::generateVersion(10, 0, 1, 0))
        {
            // Handle BSXFlags
            const Nif::NiIntegerExtraData* bsxFlags = nullptr;
            for (const auto& e : node.getExtraList())
            {
                if (e->recType == Nif::RC_BSXFlags)
                {
                    bsxFlags = static_cast<const Nif::NiIntegerExtraData*>(e.getPtr());
                    break;
                }
            }

            // Collision flag
            if (!bsxFlags || !(bsxFlags->mData & 2))
                return;

            // Editor marker flag
            if (bsxFlags->mData & 32)
                args.mHasMarkers = true;

            // FIXME: hack, using rendered geometry instead of Bethesda Havok data
            args.mAutogenerated = true;
        }
        // Pre-Gamebryo meshes
        else
        {
            // Handle RootCollisionNode
            const Nif::NiNode* colNode = nullptr;
            if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&node))
            {
                for (const auto& child : ninode->mChildren)
                {
                    if (!child.empty() && child.getPtr()->recType == Nif::RC_RootCollisionNode)
                    {
                        colNode = static_cast<const Nif::NiNode*>(child.getPtr());
                        break;
                    }
                }
            }

            args.mAutogenerated = colNode == nullptr;

            // FIXME: BulletNifLoader should never have to provide rendered geometry for camera collision
            if (colNode && colNode->mChildren.empty())
            {
                args.mAutogenerated = true;
                mShape->mVisualCollisionType = Resource::VisualCollisionType::Camera;
            }
        }

        handleNode(node, nullptr, args);
    }

    void BulletNifLoader::handleNode(const Nif::NiAVObject& node, const Nif::Parent* parent, HandleNodeArgs args)
    {
        // TODO: allow on-the fly collision switching via toggling this flag
        if (node.recType == Nif::RC_NiCollisionSwitch && !node.collisionActive())
            return;

        for (Nif::NiTimeControllerPtr ctrl = node.mController; !ctrl.empty(); ctrl = ctrl->mNext)
        {
            if (args.mAnimated)
                break;
            if (!ctrl->isActive())
                continue;
            switch (ctrl->recType)
            {
                case Nif::RC_NiKeyframeController:
                case Nif::RC_NiPathController:
                case Nif::RC_NiRollController:
                    args.mAnimated = true;
                    break;
                default:
                    continue;
            }
        }

        if (node.recType == Nif::RC_RootCollisionNode)
        {
            if (args.mAutogenerated)
            {
                // Encountered a RootCollisionNode inside an autogenerated mesh.

                // We treat empty RootCollisionNodes as NCC flag (set collisionType to `Camera`)
                // and generate the camera collision shape based on rendered geometry.
                if (mShape->mVisualCollisionType == Resource::VisualCollisionType::Camera)
                    return;

                // Otherwise we'll want to notify the user.
                Log(Debug::Info) << "RootCollisionNode is not attached to the root node in " << mShape->mFileName
                                 << ". Treating it as a common NiTriShape.";
            }
            else
            {
                args.mIsCollisionNode = true;
            }
        }

        // Don't collide with AvoidNode shapes
        if (node.recType == Nif::RC_AvoidNode)
            args.mAvoid = true;

        // Check for extra data
        for (const auto& e : node.getExtraList())
        {
            if (e->recType == Nif::RC_NiStringExtraData)
            {
                // String markers may contain important information
                // affecting the entire subtree of this node
                auto sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());

                if (Misc::StringUtils::ciStartsWith(sd->mData, "NC"))
                {
                    // NCC flag in vanilla is partly case sensitive: prefix NC is case insensitive but second C needs be
                    // uppercase
                    if (sd->mData.length() > 2 && sd->mData[2] == 'C')
                        // Collide only with camera.
                        mShape->mVisualCollisionType = Resource::VisualCollisionType::Camera;
                    else
                        // No collision.
                        mShape->mVisualCollisionType = Resource::VisualCollisionType::Default;
                }
                // Don't autogenerate collision if MRK is set.
                // FIXME: verify if this covers the entire subtree
                else if (sd->mData == "MRK" && args.mAutogenerated)
                {
                    return;
                }
            }
        }

        if (args.mAutogenerated || args.mIsCollisionNode)
        {
            // NOTE: a trishape with bounds, but no BBoxCollision flag should NOT go through handleNiTriShape!
            // It must be ignored completely.
            // (occurs in tr_ex_imp_wall_arch_04.nif)
            if (node.mBounds.mType == Nif::BoundingVolume::Type::BASE_BV && isTypeNiGeometry(node.recType))
                handleNiTriShape(static_cast<const Nif::NiGeometry&>(node), parent, args);
        }

        // For NiNodes, loop through children
        if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&node))
        {
            const Nif::Parent currentParent{ *ninode, parent };
            for (const auto& child : ninode->mChildren)
            {
                if (child.empty())
                    continue;

                assert(std::find(child->mParents.begin(), child->mParents.end(), ninode) != child->mParents.end());
                handleNode(child.get(), &currentParent, args);
            }
        }
    }

    void BulletNifLoader::handleNiTriShape(
        const Nif::NiGeometry& niGeometry, const Nif::Parent* nodeParent, HandleNodeArgs args)
    {
        // mHasMarkers is specifically BSXFlags editor marker flag.
        // If this changes, the check must be corrected.
        if (args.mHasMarkers && Misc::StringUtils::ciStartsWith(niGeometry.mName, "EditorMarker"))
            return;

        if (niGeometry.mData.empty() || niGeometry.mData->mVertices.empty())
            return;

        if (!niGeometry.mSkin.empty())
            args.mAnimated = false;
        // TODO: handle NiSkinPartition

        std::unique_ptr<btTriangleMesh> childMesh = makeChildMesh(niGeometry);
        if (childMesh == nullptr || childMesh->getNumTriangles() == 0)
            return;

        auto childShape = std::make_unique<Resource::TriangleMeshShape>(childMesh.get(), true);
        std::ignore = childMesh.release();

        osg::Matrixf transform = niGeometry.mTransform.toMatrix();
        for (const Nif::Parent* parent = nodeParent; parent != nullptr; parent = parent->mParent)
            transform *= parent->mNiNode.mTransform.toMatrix();
        childShape->setLocalScaling(Misc::Convert::toBullet(transform.getScale()));
        transform.orthoNormalize(transform);

        btTransform trans;
        trans.setOrigin(Misc::Convert::toBullet(transform.getTrans()));
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                trans.getBasis()[i][j] = transform(j, i);

        if (!args.mAvoid)
        {
            if (!mCompoundShape)
                mCompoundShape.reset(new btCompoundShape);

            if (args.mAnimated)
                mShape->mAnimatedShapes.emplace(niGeometry.recIndex, mCompoundShape->getNumChildShapes());
            mCompoundShape->addChildShape(trans, childShape.get());
        }
        else
        {
            if (!mAvoidCompoundShape)
                mAvoidCompoundShape.reset(new btCompoundShape);
            mAvoidCompoundShape->addChildShape(trans, childShape.get());
        }

        std::ignore = childShape.release();
    }

} // namespace NifBullet
