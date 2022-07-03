#include "bulletnifloader.hpp"

#include <cassert>
#include <vector>
#include <variant>
#include <sstream>
#include <tuple>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <components/debug/debuglog.hpp>

#include <components/misc/convert.hpp>
#include <components/misc/stringops.hpp>

#include <components/nif/node.hpp>
#include <components/nif/data.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/parent.hpp>

#include <components/settings/settings.hpp>

namespace
{

osg::Matrixf getWorldTransform(const Nif::Node& node, const Nif::Parent* nodeParent)
{
    osg::Matrixf result = node.trafo.toMatrix();
    for (const Nif::Parent* parent = nodeParent; parent != nullptr; parent = parent->mParent)
        result *= parent->mNiNode.trafo.toMatrix();
    return result;
}

bool pathFileNameStartsWithX(const std::string& path)
{
    const std::size_t slashpos = path.find_last_of("/\\");
    const std::size_t letterPos = slashpos == std::string::npos ? 0 : slashpos + 1;
    return letterPos < path.size() && (path[letterPos] == 'x' || path[letterPos] == 'X');
}

void fillTriangleMesh(btTriangleMesh& mesh, const Nif::NiTriShapeData& data, const osg::Matrixf &transform)
{
    const std::vector<osg::Vec3f> &vertices = data.vertices;
    const std::vector<unsigned short> &triangles = data.triangles;
    mesh.preallocateVertices(static_cast<int>(vertices.size()));
    mesh.preallocateIndices(static_cast<int>(triangles.size()));

    for (std::size_t i = 0; i < triangles.size(); i += 3)
    {
        mesh.addTriangle(
            Misc::Convert::toBullet(vertices[triangles[i + 0]] * transform),
            Misc::Convert::toBullet(vertices[triangles[i + 1]] * transform),
            Misc::Convert::toBullet(vertices[triangles[i + 2]] * transform)
        );
    }
}

void fillTriangleMesh(btTriangleMesh& mesh, const Nif::NiTriStripsData& data, const osg::Matrixf &transform)
{
    const std::vector<osg::Vec3f> &vertices = data.vertices;
    const std::vector<std::vector<unsigned short>> &strips = data.strips;
    mesh.preallocateVertices(static_cast<int>(vertices.size()));
    int numTriangles = 0;
    for (const std::vector<unsigned short>& strip : strips)
    {
        // Each strip with N points contains information about N-2 triangles.
        if (strip.size() >= 3)
            numTriangles += static_cast<int>(strip.size()-2);
    }
    mesh.preallocateIndices(static_cast<int>(numTriangles));

    // It's triangulation time. Totally not a NifSkope spell ripoff.
    for (const std::vector<unsigned short>& strip : strips)
    {
        // Can't make a triangle from less than 3 points.
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
            if (a != b && b != c && a != c)
            {
                if (i%2==0)
                {
                    mesh.addTriangle(
                        Misc::Convert::toBullet(vertices[a] * transform),
                        Misc::Convert::toBullet(vertices[b] * transform),
                        Misc::Convert::toBullet(vertices[c] * transform)
                    );
                }
                else
                {
                    mesh.addTriangle(
                        Misc::Convert::toBullet(vertices[a] * transform),
                        Misc::Convert::toBullet(vertices[c] * transform),
                        Misc::Convert::toBullet(vertices[b] * transform)
                    );
                }
            }
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
        if (data->triangles.empty())
            return {};

        return function(static_cast<const Nif::NiTriShapeData&>(*data));
    }

    if (geometry.recType == Nif::RC_NiTriStrips)
    {
        if (geometry.data->recType != Nif::RC_NiTriStripsData)
            return {};

        auto data = static_cast<const Nif::NiTriStripsData*>(geometry.data.getPtr());
        if (data->strips.empty())
            return {};

        return function(static_cast<const Nif::NiTriStripsData&>(*data));
    }

    return {};
}

std::monostate fillTriangleMesh(std::unique_ptr<btTriangleMesh>& mesh, const Nif::NiGeometry& geometry, const osg::Matrixf &transform)
{
    return handleNiGeometry(geometry, [&] (const auto& data)
    {
        if (mesh == nullptr)
            mesh = std::make_unique<btTriangleMesh>(false);
        fillTriangleMesh(*mesh, data, transform);
        return std::monostate {};
    });
}

std::unique_ptr<btTriangleMesh> makeChildMesh(const Nif::NiGeometry& geometry)
{
    return handleNiGeometry(geometry, [&] (const auto& data)
    {
        auto mesh = std::make_unique<btTriangleMesh>();
        fillTriangleMesh(*mesh, data, osg::Matrixf());
        return mesh;
    });
}

}

namespace NifBullet
{

osg::ref_ptr<Resource::BulletShape> BulletNifLoader::load(const Nif::File& nif)
{
    mShape = new Resource::BulletShape;

    mCompoundShape.reset();
    mStaticMesh.reset();
    mAvoidStaticMesh.reset();

    mShape->mFileHash = nif.getHash();

    const size_t numRoots = nif.numRoots();
    std::vector<const Nif::Node*> roots;
    for (size_t i = 0; i < numRoots; ++i)
    {
        const Nif::Record* r = nif.getRoot(i);
        if (!r)
            continue;
        const Nif::Node* node = dynamic_cast<const Nif::Node*>(r);
        if (node)
            roots.emplace_back(node);
    }
    const std::string filename = nif.getFilename();
    mShape->mFileName = filename;
    if (roots.empty())
    {
        warn("Found no root nodes in NIF file " + filename);
        return mShape;
    }

    // Try to find a valid bounding box first. If one's found for any root node, use that.
    for (const Nif::Node* node : roots)
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
    // files with the name convention xmodel.nif usually have keyframes stored in a separate file xmodel.kf (see Animation::addAnimSource).
    // assume all nodes in the file will be animated
    const bool isAnimated = pathFileNameStartsWithX(filename);

    // If there's no bounding box, we'll have to generate a Bullet collision shape
    // from the collision data present in every root node.
    for (const Nif::Node* node : roots)
    {
        bool hasCollisionNode = hasRootCollisionNode(*node);
        bool hasCollisionShape = hasCollisionNode && !collisionShapeIsEmpty(*node);
        if (hasCollisionNode && !hasCollisionShape)
            mShape->mCollisionType = Resource::BulletShape::CollisionType::Camera;
        bool generateCollisionShape = !hasCollisionShape;
        handleNode(filename, *node, nullptr, 0, generateCollisionShape, isAnimated, generateCollisionShape, false, mShape->mCollisionType);
    }

    if (mCompoundShape)
    {
        if (mStaticMesh != nullptr && mStaticMesh->getNumTriangles() > 0)
        {
            btTransform trans;
            trans.setIdentity();
            std::unique_ptr<btCollisionShape> child = std::make_unique<Resource::TriangleMeshShape>(mStaticMesh.get(), true);
            mCompoundShape->addChildShape(trans, child.get());
            std::ignore = child.release();
            std::ignore = mStaticMesh.release();
        }
        mShape->mCollisionShape = std::move(mCompoundShape);
    }
    else if (mStaticMesh != nullptr && mStaticMesh->getNumTriangles() > 0)
    {
        mShape->mCollisionShape.reset(new Resource::TriangleMeshShape(mStaticMesh.get(), true));
        std::ignore = mStaticMesh.release();
    }

    if (mAvoidStaticMesh != nullptr && mAvoidStaticMesh->getNumTriangles() > 0)
    {
        mShape->mAvoidCollisionShape.reset(new Resource::TriangleMeshShape(mAvoidStaticMesh.get(), false));
        std::ignore = mAvoidStaticMesh.release();
    }

    return mShape;
}

// Find a boundingBox in the node hierarchy.
// Return: use bounding box for collision?
bool BulletNifLoader::findBoundingBox(const Nif::Node& node, const std::string& filename)
{
    if (node.hasBounds)
    {
        unsigned int type = node.bounds.type;
        switch (type)
        {
            case Nif::NiBoundingVolume::Type::BOX_BV:
                mShape->mCollisionBox.mExtents = node.bounds.box.extents;
                mShape->mCollisionBox.mCenter = node.bounds.box.center;
                break;
            default:
            {
                std::stringstream warning;
                warning << "Unsupported NiBoundingVolume type " << type << " in node " << node.recIndex;
                warning << " in file " << filename;
                warn(warning.str());
            }
        }

        if (node.hasBBoxCollision())
        {
            return true;
        }
    }

    if (const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(&node))
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
            {
                if (findBoundingBox(list[i].get(), filename))
                    return true;
            }
        }
    }
    return false;
}

bool BulletNifLoader::hasRootCollisionNode(const Nif::Node& rootNode) const
{
    if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&rootNode))
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(list[i].empty())
                continue;
            if (list[i].getPtr()->recType == Nif::RC_RootCollisionNode)
                return true;
        }
    }
    return false;
}

bool BulletNifLoader::collisionShapeIsEmpty(const Nif::Node& rootNode) const
{
    if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(&rootNode))
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(list[i].empty())
                continue;
            const Nif::Node* childNode = list[i].getPtr();
            if (childNode->recType != Nif::RC_RootCollisionNode)
                continue;
            const Nif::NiNode* niChildnode = static_cast<const Nif::NiNode*>(childNode);  // RootCollisionNode is always a NiNode
            if (childNode->hasBounds || niChildnode->children.length() > 0)
                return false;
        }
    }
    return true;
}

void BulletNifLoader::handleNode(const std::string& fileName, const Nif::Node& node, const Nif::Parent* parent,
    int flags, bool isCollisionNode, bool isAnimated, bool autogenerated, bool avoid, unsigned int& collisionType)
{
    // TODO: allow on-the fly collision switching via toggling this flag
    if (node.recType == Nif::RC_NiCollisionSwitch && !node.collisionActive())
        return;

    // If RootCollisionNode is empty we treat it as NCC flag and autogenerate collision shape as there was no RootCollisionNode.
    // So ignoring it here if `autogenerated` is true and collisionType was set to `Camera`.
    if (node.recType == Nif::RC_RootCollisionNode && autogenerated && collisionType == Resource::BulletShape::CollisionType::Camera)
        return;

    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node.flags;

    if (!node.controller.empty() && node.controller->recType == Nif::RC_NiKeyframeController && node.controller->isActive())
        isAnimated = true;

    isCollisionNode = isCollisionNode || (node.recType == Nif::RC_RootCollisionNode);

    // Don't collide with AvoidNode shapes
    avoid = avoid || (node.recType == Nif::RC_AvoidNode);

    // We encountered a RootCollisionNode inside autogenerated mesh. It is not right.
    if (node.recType == Nif::RC_RootCollisionNode && autogenerated)
        Log(Debug::Info) << "RootCollisionNode is not attached to the root node in " << fileName << ". Treating it as a common NiTriShape.";

    // Check for extra data
    for (Nif::ExtraPtr e = node.extra; !e.empty(); e = e->next)
    {
        if (e->recType == Nif::RC_NiStringExtraData)
        {
            // String markers may contain important information
            // affecting the entire subtree of this node
            Nif::NiStringExtraData *sd = (Nif::NiStringExtraData*)e.getPtr();

            if (Misc::StringUtils::ciCompareLen(sd->string, "NC", 2) == 0)
            {
                // NCC flag in vanilla is partly case sensitive: prefix NC is case insensitive but second C needs be uppercase
                if (sd->string.length() > 2 && sd->string[2] == 'C')
                    // Collide only with camera.
                    collisionType = Resource::BulletShape::CollisionType::Camera;
                else
                    // No collision.
                    collisionType = Resource::BulletShape::CollisionType::None;
            }
            else if (sd->string == "MRK" && autogenerated)
            {
                // Marker can still have collision if the model explicitely specifies it via a RootCollisionNode.
                return;
            }

        }
    }

    if (isCollisionNode)
    {
        // NOTE: a trishape with hasBounds=true, but no BBoxCollision flag should NOT go through handleNiTriShape!
        // It must be ignored completely.
        // (occurs in tr_ex_imp_wall_arch_04.nif)
        if(!node.hasBounds && (node.recType == Nif::RC_NiTriShape
                                || node.recType == Nif::RC_NiTriStrips
                                || node.recType == Nif::RC_BSLODTriShape))
        {
            handleNiTriShape(static_cast<const Nif::NiGeometry&>(node), parent, getWorldTransform(node, parent), isAnimated, avoid);
        }
    }

    // For NiNodes, loop through children
    if (const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(&node))
    {
        const Nif::NodeList &list = ninode->children;
        const Nif::Parent currentParent {*ninode, parent};
        for(size_t i = 0;i < list.length();i++)
        {
            if (list[i].empty())
                continue;

            assert(std::find(list[i]->parents.begin(), list[i]->parents.end(), ninode) != list[i]->parents.end());
            handleNode(fileName, list[i].get(), &currentParent, flags, isCollisionNode, isAnimated, autogenerated, avoid, collisionType);
        }
    }
}

void BulletNifLoader::handleNiTriShape(const Nif::NiGeometry& niGeometry, const Nif::Parent* nodeParent,
    const osg::Matrixf &transform, bool isAnimated, bool avoid)
{
    if (niGeometry.data.empty() || niGeometry.data->vertices.empty())
        return;

    if (!niGeometry.skin.empty())
        isAnimated = false;

    if (isAnimated)
    {
        std::unique_ptr<btTriangleMesh> childMesh = makeChildMesh(niGeometry);
        if (childMesh == nullptr || childMesh->getNumTriangles() == 0)
            return;

        if (!mCompoundShape)
            mCompoundShape.reset(new btCompoundShape);

        auto childShape = std::make_unique<Resource::TriangleMeshShape>(childMesh.get(), true);
        std::ignore = childMesh.release();

        float scale = niGeometry.trafo.scale;
        for (const Nif::Parent* parent = nodeParent; parent != nullptr; parent = parent->mParent)
            scale *= parent->mNiNode.trafo.scale;
        osg::Quat q = transform.getRotate();
        osg::Vec3f v = transform.getTrans();
        childShape->setLocalScaling(btVector3(scale, scale, scale));

        btTransform trans(btQuaternion(q.x(), q.y(), q.z(), q.w()), btVector3(v.x(), v.y(), v.z()));

        mShape->mAnimatedShapes.emplace(niGeometry.recIndex, mCompoundShape->getNumChildShapes());

        mCompoundShape->addChildShape(trans, childShape.get());
        std::ignore = childShape.release();
    }
    else if (avoid)
        fillTriangleMesh(mAvoidStaticMesh, niGeometry, transform);
    else
        fillTriangleMesh(mStaticMesh, niGeometry, transform);
}

} // namespace NifBullet
