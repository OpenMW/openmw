#include "bulletnifloader.hpp"

#include <vector>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>

#include <components/debug/debuglog.hpp>

#include <components/misc/stringops.hpp>

#include <components/nif/node.hpp>
#include <components/nif/data.hpp>
#include <components/nif/extra.hpp>

namespace
{

osg::Matrixf getWorldTransform(const Nif::Node *node)
{
    if(node->parent != nullptr)
        return node->trafo.toMatrix() * getWorldTransform(node->parent);
    return node->trafo.toMatrix();
}

btVector3 getbtVector(const osg::Vec3f &v)
{
    return btVector3(v.x(), v.y(), v.z());
}

bool pathFileNameStartsWithX(const std::string& path)
{
    const std::size_t slashpos = path.find_last_of("/\\");
    const std::size_t letterPos = slashpos == std::string::npos ? 0 : slashpos + 1;
    return letterPos < path.size() && (path[letterPos] == 'x' || path[letterPos] == 'X');
}

void fillTriangleMeshWithTransform(btTriangleMesh& mesh, const Nif::NiTriShapeData& data, const osg::Matrixf &transform)
{
    mesh.preallocateVertices(static_cast<int>(data.vertices.size()));
    mesh.preallocateIndices(static_cast<int>(data.triangles.size()));

    const std::vector<osg::Vec3f> &vertices = data.vertices;
    const std::vector<unsigned short> &triangles = data.triangles;

    for (std::size_t i = 0; i < triangles.size(); i += 3)
    {
        mesh.addTriangle(
            getbtVector(vertices[triangles[i + 0]] * transform),
            getbtVector(vertices[triangles[i + 1]] * transform),
            getbtVector(vertices[triangles[i + 2]] * transform)
        );
    }
}

void fillTriangleMeshWithTransform(btTriangleMesh& mesh, const Nif::NiTriStripsData& data, const osg::Matrixf &transform)
{
    const std::vector<osg::Vec3f> &vertices = data.vertices;
    const std::vector<std::vector<unsigned short>> &strips = data.strips;
    if (vertices.empty() || strips.empty())
        return;
    mesh.preallocateVertices(static_cast<int>(data.vertices.size()));
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

        unsigned short a = strip[0], b = strip[0], c = strip[1];
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
                        getbtVector(vertices[a] * transform),
                        getbtVector(vertices[b] * transform),
                        getbtVector(vertices[c] * transform)
                    );
                }
                else
                {
                    mesh.addTriangle(
                        getbtVector(vertices[a] * transform),
                        getbtVector(vertices[c] * transform),
                        getbtVector(vertices[b] * transform)
                    );
                }
            }
        }
    }
}

void fillTriangleMeshWithTransform(btTriangleMesh& mesh, const Nif::Node* nifNode, const osg::Matrixf &transform)
{
    if (nifNode->recType == Nif::RC_NiTriShape)
        fillTriangleMeshWithTransform(mesh, static_cast<const Nif::NiTriShape*>(nifNode)->data.get(), transform);
    else // if (nifNode->recType == Nif::RC_NiTriStrips)
        fillTriangleMeshWithTransform(mesh, static_cast<const Nif::NiTriStrips*>(nifNode)->data.get(), transform);
}

void fillTriangleMesh(btTriangleMesh& mesh, const Nif::Node* node)
{
    fillTriangleMeshWithTransform(mesh, node, osg::Matrixf());
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

    if (nif.numRoots() < 1)
    {
        warn("Found no root nodes in NIF.");
        return mShape;
    }

    Nif::Record *r = nif.getRoot(0);
    assert(r != nullptr);

    Nif::Node *node = dynamic_cast<Nif::Node*>(r);
    if (node == nullptr)
    {
        warn("First root in file was not a node, but a " +
             r->recName + ". Skipping file.");
        return mShape;
    }

    if (findBoundingBox(node))
    {
        std::unique_ptr<btCompoundShape> compound (new btCompoundShape);
        std::unique_ptr<btBoxShape> boxShape(new btBoxShape(getbtVector(mShape->mCollisionBoxHalfExtents)));
        btTransform transform = btTransform::getIdentity();
        transform.setOrigin(getbtVector(mShape->mCollisionBoxTranslate));
        compound->addChildShape(transform, boxShape.get());
        boxShape.release();

        mShape->mCollisionShape = compound.release();
        return mShape;
    }
    else
    {
        bool autogenerated = hasAutoGeneratedCollision(node);

        // files with the name convention xmodel.nif usually have keyframes stored in a separate file xmodel.kf (see Animation::addAnimSource).
        // assume all nodes in the file will be animated
        const auto filename = nif.getFilename();
        const bool isAnimated = pathFileNameStartsWithX(filename);

        handleNode(filename, node, 0, autogenerated, isAnimated, autogenerated);

        if (mCompoundShape)
        {
            if (mStaticMesh)
            {
                btTransform trans;
                trans.setIdentity();
                std::unique_ptr<btCollisionShape> child(new Resource::TriangleMeshShape(mStaticMesh.get(), true));
                mCompoundShape->addChildShape(trans, child.get());
                child.release();
                mStaticMesh.release();
            }
            mShape->mCollisionShape = mCompoundShape.release();
        }
        else if (mStaticMesh)
        {
            mShape->mCollisionShape = new Resource::TriangleMeshShape(mStaticMesh.get(), true);
            mStaticMesh.release();
        }

        if (mAvoidStaticMesh)
        {
            mShape->mAvoidCollisionShape = new Resource::TriangleMeshShape(mAvoidStaticMesh.get(), false);
            mAvoidStaticMesh.release();
        }

        return mShape;
    }
}

// Find a boundingBox in the node hierarchy.
// Return: use bounding box for collision?
bool BulletNifLoader::findBoundingBox(const Nif::Node* node)
{
    if (node->hasBounds)
    {
        mShape->mCollisionBoxHalfExtents = node->boundXYZ;
        mShape->mCollisionBoxTranslate = node->boundPos;

        if (node->flags & Nif::NiNode::Flag_BBoxCollision)
        {
            return true;
        }
    }

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
            {
                bool found = findBoundingBox (list[i].getPtr());
                if (found)
                    return true;
            }
        }
    }
    return false;
}

bool BulletNifLoader::hasAutoGeneratedCollision(const Nif::Node* rootNode)
{
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(rootNode);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
            {
                if(list[i].getPtr()->recType == Nif::RC_RootCollisionNode)
                    return false;
            }
        }
    }
    return true;
}

void BulletNifLoader::handleNode(const std::string& fileName, const Nif::Node *node, int flags,
        bool isCollisionNode, bool isAnimated, bool autogenerated, bool avoid)
{
    // TODO: allow on-the fly collision switching via toggling this flag
    if (node->recType == Nif::RC_NiCollisionSwitch && !(node->flags & Nif::NiNode::Flag_ActiveCollision))
        return;

    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node->flags;

    if (!node->controller.empty() && node->controller->recType == Nif::RC_NiKeyframeController
            && (node->controller->flags & Nif::NiNode::ControllerFlag_Active))
        isAnimated = true;

    isCollisionNode = isCollisionNode || (node->recType == Nif::RC_RootCollisionNode);

    // Don't collide with AvoidNode shapes
    avoid = avoid || (node->recType == Nif::RC_AvoidNode);

    // We encountered a RootCollisionNode inside autogenerated mesh. It is not right.
    if (node->recType == Nif::RC_RootCollisionNode && autogenerated)
        Log(Debug::Info) << "RootCollisionNode is not attached to the root node in " << fileName << ". Treating it as a common NiTriShape.";

    // Check for extra data
    for (Nif::ExtraPtr e = node->extra; !e.empty(); e = e->next)
    {
        if (e->recType == Nif::RC_NiStringExtraData)
        {
            // String markers may contain important information
            // affecting the entire subtree of this node
            Nif::NiStringExtraData *sd = (Nif::NiStringExtraData*)e.getPtr();

            if (Misc::StringUtils::ciCompareLen(sd->string, "NC", 2) == 0)
            {
                // No collision. Use an internal flag setting to mark this.
                flags |= 0x800;
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
        if(!node->hasBounds && (node->recType == Nif::RC_NiTriShape || node->recType == Nif::RC_NiTriStrips))
        {
            handleNiTriShape(node, flags, getWorldTransform(node), isAnimated, avoid);
        }
    }

    // For NiNodes, loop through children
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
                handleNode(fileName, list[i].getPtr(), flags, isCollisionNode, isAnimated, autogenerated, avoid);
        }
    }
}

void BulletNifLoader::handleNiTriShape(const Nif::Node *nifNode, int flags, const osg::Matrixf &transform,
                                       bool isAnimated, bool avoid)
{
    assert(nifNode != nullptr);

    // If the object was marked "NCO" earlier, it shouldn't collide with
    // anything. So don't do anything.
    if ((flags & 0x800))
        return;

    if (nifNode->recType == Nif::RC_NiTriShape)
    {
        const Nif::NiTriShape* shape = static_cast<const Nif::NiTriShape*>(nifNode);
        if (!shape->skin.empty())
            isAnimated = false;
        if (shape->data.empty() || shape->data->triangles.empty())
            return;
    }
    else
    {
        const Nif::NiTriStrips* shape = static_cast<const Nif::NiTriStrips*>(nifNode);
        if (!shape->skin.empty())
            isAnimated = false;
        if (shape->data.empty() || shape->data->strips.empty())
            return;
    }


    if (isAnimated)
    {
        if (!mCompoundShape)
            mCompoundShape.reset(new btCompoundShape);

        std::unique_ptr<btTriangleMesh> childMesh(new btTriangleMesh);

        fillTriangleMesh(*childMesh, nifNode);

        std::unique_ptr<Resource::TriangleMeshShape> childShape(new Resource::TriangleMeshShape(childMesh.get(), true));
        childMesh.release();

        float scale = nifNode->trafo.scale;
        const Nif::Node* parent = nifNode;
        while (parent->parent)
        {
            parent = parent->parent;
            scale *= parent->trafo.scale;
        }
        osg::Quat q = transform.getRotate();
        osg::Vec3f v = transform.getTrans();
        childShape->setLocalScaling(btVector3(scale, scale, scale));

        btTransform trans(btQuaternion(q.x(), q.y(), q.z(), q.w()), btVector3(v.x(), v.y(), v.z()));

        mShape->mAnimatedShapes.emplace(nifNode->recIndex, mCompoundShape->getNumChildShapes());

        mCompoundShape->addChildShape(trans, childShape.get());
        childShape.release();
    }
    else if (avoid)
    {
        if (!mAvoidStaticMesh)
            mAvoidStaticMesh.reset(new btTriangleMesh(false));

        fillTriangleMeshWithTransform(*mAvoidStaticMesh, nifNode, transform);
    }
    else
    {
        if (!mStaticMesh)
            mStaticMesh.reset(new btTriangleMesh(false));

        // Static shape, just transform all vertices into position
        fillTriangleMeshWithTransform(*mStaticMesh, nifNode, transform);
    }
}

} // namespace NifBullet
