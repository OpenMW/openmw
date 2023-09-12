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
        -> decltype(function(static_cast<const Nif::NiTriShapeData&>(geometry.data.get())))
    {
        if (geometry.recType == Nif::RC_NiTriShape || geometry.recType == Nif::RC_BSLODTriShape)
        {
            if (geometry.data->recType != Nif::RC_NiTriShapeData)
                return {};

            auto data = static_cast<const Nif::NiTriShapeData*>(geometry.data.getPtr());
            if (data->mTriangles.empty())
                return {};

            return function(static_cast<const Nif::NiTriShapeData&>(*data));
        }

        if (geometry.recType == Nif::RC_NiTriStrips)
        {
            if (geometry.data->recType != Nif::RC_NiTriStripsData)
                return {};

            auto data = static_cast<const Nif::NiTriStripsData*>(geometry.data.getPtr());
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
        const std::string filename = Files::pathToUnicodeString(nif.getFilename());
        mShape->mFileName = filename;
        if (roots.empty())
        {
            warn("Found no root nodes in NIF file " + filename);
            return mShape;
        }

        // Try to find a valid bounding box first. If one's found for any root node, use that.
        for (const Nif::NiAVObject* node : roots)
        {
            if (findBoundingBox(*node, filename))
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
        // files with the name convention xmodel.nif usually have keyframes stored in a separate file xmodel.kf (see
        // Animation::addAnimSource). assume all nodes in the file will be animated
        // TODO: investigate whether this should and could be optimized.
        const bool isAnimated = pathFileNameStartsWithX(filename);

        // If there's no bounding box, we'll have to generate a Bullet collision shape
        // from the collision data present in every root node.
        for (const Nif::NiAVObject* node : roots)
        {
            const Nif::NiNode* colNode = findRootCollisionNode(*node);
            bool hasCollisionShape = false;
            if (colNode != nullptr)
            {
                if (colNode->mBounds.type == Nif::NiBoundingVolume::Type::BASE_BV && !colNode->mChildren.empty())
                    hasCollisionShape = true;
                else
                    mShape->mVisualCollisionType = Resource::VisualCollisionType::Camera;
            }
            HandleNodeArgs args;
            args.mAutogenerated = args.mIsCollisionNode = !hasCollisionShape;
            args.mAnimated = isAnimated;
            handleNode(filename, *node, nullptr, args, mShape->mVisualCollisionType);
        }

        if (mCompoundShape)
            mShape->mCollisionShape = std::move(mCompoundShape);

        if (mAvoidCompoundShape)
            mShape->mAvoidCollisionShape = std::move(mAvoidCompoundShape);

        return mShape;
    }

    // Find a boundingBox in the node hierarchy.
    // Return: use bounding box for collision?
    bool BulletNifLoader::findBoundingBox(const Nif::NiAVObject& node, const std::string& filename)
    {
        unsigned int type = node.mBounds.type;
        switch (type)
        {
            case Nif::NiBoundingVolume::Type::BASE_BV:
                break;
            case Nif::NiBoundingVolume::Type::BOX_BV:
                mShape->mCollisionBox.mExtents = node.mBounds.box.extents;
                mShape->mCollisionBox.mCenter = node.mBounds.box.center;
                break;
            default:
            {
                std::stringstream warning;
                warning << "Unsupported NiBoundingVolume type " << type << " in node " << node.recIndex;
                warning << " in file " << filename;
                warn(warning.str());
            }
        }

        if (type != Nif::NiBoundingVolume::Type::BASE_BV && node.hasBBoxCollision())
            return true;

        if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&node))
        {
            for (const auto& child : ninode->mChildren)
                if (!child.empty() && findBoundingBox(child.get(), filename))
                    return true;
        }
        return false;
    }

    const Nif::NiNode* BulletNifLoader::findRootCollisionNode(const Nif::NiAVObject& rootNode) const
    {
        if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&rootNode))
        {
            for (const auto& child : ninode->mChildren)
            {
                if (!child.empty() && child.getPtr()->recType == Nif::RC_RootCollisionNode)
                    return static_cast<const Nif::NiNode*>(child.getPtr());
            }
        }
        return nullptr;
    }

    void BulletNifLoader::handleNode(const std::string& fileName, const Nif::NiAVObject& node,
        const Nif::Parent* parent, HandleNodeArgs args, Resource::VisualCollisionType& visualCollisionType)
    {
        // TODO: allow on-the fly collision switching via toggling this flag
        if (node.recType == Nif::RC_NiCollisionSwitch && !node.collisionActive())
            return;

        for (Nif::ControllerPtr ctrl = node.mController; !ctrl.empty(); ctrl = ctrl->next)
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
            args.mIsCollisionNode = true;
            if (args.mAutogenerated)
            {
                // Encountered a RootCollisionNode inside an autogenerated mesh.

                // We treat empty RootCollisionNodes as NCC flag (set collisionType to `Camera`)
                // and generate the camera collision shape based on rendered geometry.
                if (visualCollisionType == Resource::VisualCollisionType::Camera)
                    return;

                // Otherwise we'll want to notify the user.
                Log(Debug::Info) << "RootCollisionNode is not attached to the root node in " << fileName
                                 << ". Treating it as a common NiTriShape.";
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
                        visualCollisionType = Resource::VisualCollisionType::Camera;
                    else
                        // No collision.
                        visualCollisionType = Resource::VisualCollisionType::Default;
                }
                // Don't autogenerate collision if MRK is set.
                // FIXME: verify if this covers the entire subtree
                else if (sd->mData == "MRK" && args.mAutogenerated)
                {
                    return;
                }
            }
            else if (e->recType == Nif::RC_BSXFlags)
            {
                auto bsxFlags = static_cast<const Nif::NiIntegerExtraData*>(e.getPtr());
                if (bsxFlags->mData & 32) // Editor marker flag
                    args.mHasMarkers = true;
            }
        }

        if (args.mIsCollisionNode)
        {
            // NOTE: a trishape with bounds, but no BBoxCollision flag should NOT go through handleNiTriShape!
            // It must be ignored completely.
            // (occurs in tr_ex_imp_wall_arch_04.nif)
            if (node.mBounds.type == Nif::NiBoundingVolume::Type::BASE_BV
                && (node.recType == Nif::RC_NiTriShape || node.recType == Nif::RC_NiTriStrips
                    || node.recType == Nif::RC_BSLODTriShape))
            {
                handleNiTriShape(static_cast<const Nif::NiGeometry&>(node), parent, args);
            }
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
                handleNode(fileName, child.get(), &currentParent, args, visualCollisionType);
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

        if (niGeometry.data.empty() || niGeometry.data->mVertices.empty())
            return;

        if (!niGeometry.skin.empty())
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
