#include "../nif/node.hpp"

#include <components/bullethelpers/processtrianglecallback.hpp>
#include <components/nif/data.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/node.hpp>
#include <components/nifbullet/bulletnifloader.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <span>
#include <type_traits>

namespace
{
    template <class T>
    bool compareObjects(const T* lhs, const T* rhs)
    {
        return (!lhs && !rhs) || (lhs && rhs && *lhs == *rhs);
    }

    std::vector<btVector3> getTriangles(const btBvhTriangleMeshShape& shape)
    {
        std::vector<btVector3> result;
        auto callback = BulletHelpers::makeProcessTriangleCallback([&](btVector3* triangle, int, int) {
            for (std::size_t i = 0; i < 3; ++i)
                result.push_back(triangle[i]);
        });
        btVector3 aabbMin;
        btVector3 aabbMax;
        shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
        shape.processAllTriangles(&callback, aabbMin, aabbMax);
        return result;
    }

    bool isNear(btScalar lhs, btScalar rhs)
    {
        return std::abs(lhs - rhs) <= 1e-5;
    }

    bool isNear(const btVector3& lhs, const btVector3& rhs)
    {
        return std::equal(static_cast<const btScalar*>(lhs), static_cast<const btScalar*>(lhs) + 3,
            static_cast<const btScalar*>(rhs), [](btScalar l, btScalar r) { return isNear(l, r); });
    }

    bool isNear(const btMatrix3x3& lhs, const btMatrix3x3& rhs)
    {
        for (int i = 0; i < 3; ++i)
            if (!isNear(lhs[i], rhs[i]))
                return false;
        return true;
    }

    bool isNear(const btTransform& lhs, const btTransform& rhs)
    {
        return isNear(lhs.getOrigin(), rhs.getOrigin()) && isNear(lhs.getBasis(), rhs.getBasis());
    }

    bool isNear(std::span<const btVector3> lhs, std::span<const btVector3> rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        return std::equal(
            lhs.begin(), lhs.end(), rhs.begin(), [](const btVector3& l, const btVector3& r) { return isNear(l, r); });
    }

    struct WriteVec3f
    {
        osg::Vec3f mValue;

        friend std::ostream& operator<<(std::ostream& stream, const WriteVec3f& value)
        {
            return stream << "osg::Vec3f {" << std::setprecision(std::numeric_limits<float>::max_exponent10)
                          << value.mValue.x() << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10)
                          << value.mValue.y() << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10)
                          << value.mValue.z() << "}";
        }
    };
}

static std::ostream& operator<<(std::ostream& stream, const btVector3& value)
{
    return stream << "btVector3 {" << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.getX()
                  << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.getY() << ", "
                  << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.getZ() << "}";
}

static std::ostream& operator<<(std::ostream& stream, const btMatrix3x3& value)
{
    stream << "btMatrix3x3 {";
    for (int i = 0; i < 3; ++i)
        stream << value.getRow(i) << ", ";
    return stream << "}";
}

static std::ostream& operator<<(std::ostream& stream, const btTransform& value)
{
    return stream << "btTransform {" << value.getBasis() << ", " << value.getOrigin() << "}";
}

static std::ostream& operator<<(std::ostream& stream, const btCollisionShape* value);

static std::ostream& operator<<(std::ostream& stream, const btCompoundShape& value)
{
    stream << "btCompoundShape {" << value.getLocalScaling() << ", ";
    stream << "{";
    for (int i = 0; i < value.getNumChildShapes(); ++i)
        stream << value.getChildShape(i) << ", ";
    stream << "},";
    stream << "{";
    for (int i = 0; i < value.getNumChildShapes(); ++i)
        stream << value.getChildTransform(i) << ", ";
    stream << "}";
    return stream << "}";
}

static std::ostream& operator<<(std::ostream& stream, const btBoxShape& value)
{
    return stream << "btBoxShape {" << value.getLocalScaling() << ", " << value.getHalfExtentsWithoutMargin() << "}";
}

namespace Resource
{

    static std::ostream& operator<<(std::ostream& stream, const TriangleMeshShape& value)
    {
        stream << "Resource::TriangleMeshShape {" << value.getLocalScaling() << ", "
               << value.usesQuantizedAabbCompression() << ", " << value.getOwnsBvh() << ", {";
        auto callback = BulletHelpers::makeProcessTriangleCallback([&](btVector3* triangle, int, int) {
            for (std::size_t i = 0; i < 3; ++i)
                stream << triangle[i] << ", ";
        });
        btVector3 aabbMin;
        btVector3 aabbMax;
        value.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
        value.processAllTriangles(&callback, aabbMin, aabbMax);
        return stream << "}}";
    }

    static std::ostream& operator<<(std::ostream& stream, const ScaledTriangleMeshShape& value)
    {
        return stream << "Resource::ScaledTriangleMeshShape {" << value.getLocalScaling() << ", "
                      << value.getChildShape() << "}";
    }

    static bool operator==(const CollisionBox& l, const CollisionBox& r)
    {
        const auto tie = [](const CollisionBox& v) { return std::tie(v.mExtents, v.mCenter); };
        return tie(l) == tie(r);
    }

    static std::ostream& operator<<(std::ostream& stream, const CollisionBox& value)
    {
        return stream << "CollisionBox {" << WriteVec3f{ value.mExtents } << ", " << WriteVec3f{ value.mCenter } << "}";
    }

}

static std::ostream& operator<<(std::ostream& stream, const btCollisionShape& value)
{
    switch (value.getShapeType())
    {
        case COMPOUND_SHAPE_PROXYTYPE:
            return stream << static_cast<const btCompoundShape&>(value);
        case BOX_SHAPE_PROXYTYPE:
            return stream << static_cast<const btBoxShape&>(value);
        case TRIANGLE_MESH_SHAPE_PROXYTYPE:
            if (const auto casted = dynamic_cast<const Resource::TriangleMeshShape*>(&value))
                return stream << *casted;
            break;
        case SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE:
            if (const auto casted = dynamic_cast<const Resource::ScaledTriangleMeshShape*>(&value))
                return stream << *casted;
            break;
    }
    return stream << "btCollisionShape {" << value.getShapeType() << "}";
}

static std::ostream& operator<<(std::ostream& stream, const btCollisionShape* value)
{
    return value ? stream << "&" << *value : stream << "nullptr";
}

namespace std
{
    static std::ostream& operator<<(std::ostream& stream, const map<int, int>& value)
    {
        stream << "std::map<int, int> {";
        for (const auto& v : value)
            stream << "{" << v.first << ", " << v.second << "},";
        return stream << "}";
    }
}

namespace Resource
{
    static bool operator==(const Resource::BulletShape& lhs, const Resource::BulletShape& rhs)
    {
        return compareObjects(lhs.mCollisionShape.get(), rhs.mCollisionShape.get())
            && compareObjects(lhs.mAvoidCollisionShape.get(), rhs.mAvoidCollisionShape.get())
            && lhs.mCollisionBox == rhs.mCollisionBox && lhs.mVisualCollisionType == rhs.mVisualCollisionType
            && lhs.mAnimatedShapes == rhs.mAnimatedShapes;
    }

    static std::ostream& operator<<(std::ostream& stream, Resource::VisualCollisionType value)
    {
        switch (value)
        {
            case Resource::VisualCollisionType::None:
                return stream << "Resource::VisualCollisionType::None";
            case Resource::VisualCollisionType::Default:
                return stream << "Resource::VisualCollisionType::Default";
            case Resource::VisualCollisionType::Camera:
                return stream << "Resource::VisualCollisionType::Camera";
        }
        return stream << static_cast<std::underlying_type_t<Resource::VisualCollisionType>>(value);
    }

    static std::ostream& operator<<(std::ostream& stream, const Resource::BulletShape& value)
    {
        return stream << "Resource::BulletShape {" << value.mCollisionShape.get() << ", "
                      << value.mAvoidCollisionShape.get() << ", " << value.mCollisionBox << ", "
                      << value.mAnimatedShapes << ", " << value.mVisualCollisionType << "}";
    }
}

static bool operator==(const btCollisionShape& lhs, const btCollisionShape& rhs);

static bool operator==(const btCompoundShape& lhs, const btCompoundShape& rhs)
{
    if (lhs.getNumChildShapes() != rhs.getNumChildShapes() || lhs.getLocalScaling() != rhs.getLocalScaling())
        return false;
    for (int i = 0; i < lhs.getNumChildShapes(); ++i)
    {
        if (!compareObjects(lhs.getChildShape(i), rhs.getChildShape(i))
            || !isNear(lhs.getChildTransform(i), rhs.getChildTransform(i)))
            return false;
    }
    return true;
}

static bool operator==(const btBoxShape& lhs, const btBoxShape& rhs)
{
    return isNear(lhs.getLocalScaling(), rhs.getLocalScaling())
        && lhs.getHalfExtentsWithoutMargin() == rhs.getHalfExtentsWithoutMargin();
}

static bool operator==(const btBvhTriangleMeshShape& lhs, const btBvhTriangleMeshShape& rhs)
{
    return isNear(lhs.getLocalScaling(), rhs.getLocalScaling())
        && lhs.usesQuantizedAabbCompression() == rhs.usesQuantizedAabbCompression()
        && lhs.getOwnsBvh() == rhs.getOwnsBvh() && isNear(getTriangles(lhs), getTriangles(rhs));
}

static bool operator==(const btScaledBvhTriangleMeshShape& lhs, const btScaledBvhTriangleMeshShape& rhs)
{
    return isNear(lhs.getLocalScaling(), rhs.getLocalScaling())
        && compareObjects(lhs.getChildShape(), rhs.getChildShape());
}

static bool operator==(const btCollisionShape& lhs, const btCollisionShape& rhs)
{
    if (lhs.getShapeType() != rhs.getShapeType())
        return false;
    switch (lhs.getShapeType())
    {
        case COMPOUND_SHAPE_PROXYTYPE:
            return static_cast<const btCompoundShape&>(lhs) == static_cast<const btCompoundShape&>(rhs);
        case BOX_SHAPE_PROXYTYPE:
            return static_cast<const btBoxShape&>(lhs) == static_cast<const btBoxShape&>(rhs);
        case TRIANGLE_MESH_SHAPE_PROXYTYPE:
            if (const auto lhsCasted = dynamic_cast<const Resource::TriangleMeshShape*>(&lhs))
                if (const auto rhsCasted = dynamic_cast<const Resource::TriangleMeshShape*>(&rhs))
                    return *lhsCasted == *rhsCasted;
            return false;
        case SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE:
            if (const auto lhsCasted = dynamic_cast<const Resource::ScaledTriangleMeshShape*>(&lhs))
                if (const auto rhsCasted = dynamic_cast<const Resource::ScaledTriangleMeshShape*>(&rhs))
                    return *lhsCasted == *rhsCasted;
            return false;
    }
    return false;
}

namespace
{
    using namespace testing;
    using namespace Nif::Testing;
    using NifBullet::BulletNifLoader;

    constexpr VFS::Path::NormalizedView testNif("test.nif");
    constexpr VFS::Path::NormalizedView xtestNif("xtest.nif");

    void copy(const btTransform& src, Nif::NiTransform& dst)
    {
        dst.mTranslation = osg::Vec3f(static_cast<float>(src.getOrigin().x()), static_cast<float>(src.getOrigin().y()),
            static_cast<float>(src.getOrigin().z()));
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
                dst.mRotation.mValues[row][column] = static_cast<float>(src.getBasis().getRow(row)[column]);
    }

    struct TestBulletNifLoader : Test
    {
        BulletNifLoader mLoader;
        Nif::NiAVObject mNode;
        Nif::NiAVObject mNode2;
        Nif::NiNode mNiNode;
        Nif::NiNode mNiNode2;
        Nif::NiNode mNiNode3;
        Nif::NiTriShapeData mNiTriShapeData;
        Nif::NiTriShape mNiTriShape;
        Nif::NiTriShapeData mNiTriShapeData2;
        Nif::NiTriShape mNiTriShape2;
        Nif::NiTriStripsData mNiTriStripsData;
        Nif::NiTriStrips mNiTriStrips;
        Nif::NiSkinInstance mNiSkinInstance;
        Nif::NiStringExtraData mNiStringExtraData;
        Nif::NiStringExtraData mNiStringExtraData2;
        Nif::NiIntegerExtraData mNiIntegerExtraData;
        Nif::NiTimeController mController;
        btTransform mTransform{ btMatrix3x3(btQuaternion(btVector3(1, 0, 0), 0.5f)), btVector3(1, 2, 3) };
        btTransform mTransformScale2{ btMatrix3x3(btQuaternion(btVector3(1, 0, 0), 0.5f)), btVector3(2, 4, 6) };
        btTransform mTransformScale3{ btMatrix3x3(btQuaternion(btVector3(1, 0, 0), 0.5f)), btVector3(3, 6, 9) };
        btTransform mTransformScale4{ btMatrix3x3(btQuaternion(btVector3(1, 0, 0), 0.5f)), btVector3(4, 8, 12) };
        const std::string mHash = "hash";

        TestBulletNifLoader()
        {
            init(mNode);
            init(mNode2);
            init(mNiNode);
            init(mNiNode2);
            init(mNiNode3);
            init(mNiTriShape);
            init(mNiTriShape2);
            init(mNiTriStrips);
            init(mNiSkinInstance);
            init(mNiStringExtraData);
            init(mNiStringExtraData2);
            init(mController);

            mNiTriShapeData.recType = Nif::RC_NiTriShapeData;
            mNiTriShapeData.mVertices = { osg::Vec3f(0, 0, 0), osg::Vec3f(1, 0, 0), osg::Vec3f(1, 1, 0) };
            mNiTriShapeData.mNumTriangles = 1;
            mNiTriShapeData.mTriangles = { 0, 1, 2 };
            mNiTriShape.mData = Nif::NiGeometryDataPtr(&mNiTriShapeData);

            mNiTriShapeData2.recType = Nif::RC_NiTriShapeData;
            mNiTriShapeData2.mVertices = { osg::Vec3f(0, 0, 1), osg::Vec3f(1, 0, 1), osg::Vec3f(1, 1, 1) };
            mNiTriShapeData2.mNumTriangles = 1;
            mNiTriShapeData2.mTriangles = { 0, 1, 2 };
            mNiTriShape2.mData = Nif::NiGeometryDataPtr(&mNiTriShapeData2);

            mNiTriStripsData.recType = Nif::RC_NiTriStripsData;
            mNiTriStripsData.mVertices
                = { osg::Vec3f(0, 0, 0), osg::Vec3f(1, 0, 0), osg::Vec3f(1, 1, 0), osg::Vec3f(0, 1, 0) };
            mNiTriStripsData.mNumTriangles = 2;
            mNiTriStripsData.mStrips = { { 0, 1, 2, 3 } };
            mNiTriStrips.mData = Nif::NiGeometryDataPtr(&mNiTriStripsData);
        }
    };

    TEST_F(TestBulletNifLoader, for_zero_num_roots_should_return_default)
    {
        Nif::NIFFile file(testNif);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
        EXPECT_EQ(result->mFileName, "test.nif");
        EXPECT_EQ(result->mFileHash, mHash);
    }

    TEST_F(TestBulletNifLoader, should_ignore_nullptr_root)
    {
        Nif::NIFFile file(testNif);
        file.mRoots.push_back(nullptr);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_default_root_nif_node_should_return_default)
    {
        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_default_root_collision_node_nif_node_should_return_default)
    {
        mNode.recType = Nif::RC_RootCollisionNode;

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_default_root_nif_node_and_filename_starting_with_x_should_return_default)
    {
        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_bounding_box_should_return_shape_with_bounding_box_data)
    {
        mNode.mName = "Bounding Box";
        mNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode.mBounds.mBox.mExtents = osg::Vec3f(1, 2, 3);
        mNode.mBounds.mBox.mCenter = osg::Vec3f(-1, -2, -3);

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;
        expected.mCollisionBox.mExtents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.mCenter = osg::Vec3f(-1, -2, -3);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_child_bounding_box_should_return_shape_with_bounding_box_data)
    {
        mNode.mName = "Bounding Box";
        mNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode.mBounds.mBox.mExtents = osg::Vec3f(1, 2, 3);
        mNode.mBounds.mBox.mCenter = osg::Vec3f(-1, -2, -3);
        mNode.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNode) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;
        expected.mCollisionBox.mExtents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.mCenter = osg::Vec3f(-1, -2, -3);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_with_bounds_and_child_bounding_box_should_use_bounding_box)
    {
        mNode.mName = "Bounding Box";
        mNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode.mBounds.mBox.mExtents = osg::Vec3f(1, 2, 3);
        mNode.mBounds.mBox.mCenter = osg::Vec3f(-1, -2, -3);
        mNode.mParents.push_back(&mNiNode);

        mNiNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNiNode.mBounds.mBox.mExtents = osg::Vec3f(4, 5, 6);
        mNiNode.mBounds.mBox.mCenter = osg::Vec3f(-4, -5, -6);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNode) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;
        expected.mCollisionBox.mExtents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.mCenter = osg::Vec3f(-1, -2, -3);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(
        TestBulletNifLoader, for_root_and_two_children_where_both_with_bounds_but_one_is_bounding_box_use_bounding_box)
    {
        mNode.mName = "Bounding Box";
        mNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode.mBounds.mBox.mExtents = osg::Vec3f(1, 2, 3);
        mNode.mBounds.mBox.mCenter = osg::Vec3f(-1, -2, -3);
        mNode.mParents.push_back(&mNiNode);

        mNode2.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode2.mBounds.mBox.mExtents = osg::Vec3f(4, 5, 6);
        mNode2.mBounds.mBox.mCenter = osg::Vec3f(-4, -5, -6);
        mNode2.mParents.push_back(&mNiNode);

        mNiNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNiNode.mBounds.mBox.mExtents = osg::Vec3f(7, 8, 9);
        mNiNode.mBounds.mBox.mCenter = osg::Vec3f(-7, -8, -9);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNode), Nif::NiAVObjectPtr(&mNode2) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;
        expected.mCollisionBox.mExtents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.mCenter = osg::Vec3f(-1, -2, -3);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader,
        for_root_and_two_children_where_both_with_bounds_but_second_is_bounding_box_use_bounding_box)
    {
        mNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode.mBounds.mBox.mExtents = osg::Vec3f(1, 2, 3);
        mNode.mBounds.mBox.mCenter = osg::Vec3f(-1, -2, -3);
        mNode.mParents.push_back(&mNiNode);

        mNode2.mName = "Bounding Box";
        mNode2.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode2.mBounds.mBox.mExtents = osg::Vec3f(4, 5, 6);
        mNode2.mBounds.mBox.mCenter = osg::Vec3f(-4, -5, -6);
        mNode2.mParents.push_back(&mNiNode);

        mNiNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNiNode.mBounds.mBox.mExtents = osg::Vec3f(7, 8, 9);
        mNiNode.mBounds.mBox.mCenter = osg::Vec3f(-7, -8, -9);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNode), Nif::NiAVObjectPtr(&mNode2) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;
        expected.mCollisionBox.mExtents = osg::Vec3f(4, 5, 6);
        expected.mCollisionBox.mCenter = osg::Vec3f(-4, -5, -6);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_nif_node_with_bounds_should_return_shape_with_null_collision_shape)
    {
        mNode.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNode.mBounds.mBox.mExtents = osg::Vec3f(1, 2, 3);
        mNode.mBounds.mBox.mCenter = osg::Vec3f(-1, -2, -3);

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_root_node_should_return_static_shape)
    {
        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiTriShape);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));

        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        auto triShape = std::make_unique<Resource::TriangleMeshShape>(triangles.release(), true);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(triShape.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_root_node_with_bounds_should_return_static_shape)
    {
        mNiTriShape.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNiTriShape.mBounds.mBox.mExtents = osg::Vec3f(1, 2, 3);
        mNiTriShape.mBounds.mBox.mCenter = osg::Vec3f(-1, -2, -3);

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiTriShape);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));

        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        auto triShape = std::make_unique<Resource::TriangleMeshShape>(triangles.release(), true);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(triShape.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_should_return_static_shape)
    {
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));

        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        auto triShape = std::make_unique<Resource::TriangleMeshShape>(triangles.release(), true);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(triShape.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_nested_tri_shape_child_should_return_static_shape)
    {
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiNode2) };
        mNiNode2.mParents.push_back(&mNiNode);
        mNiNode2.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };
        mNiTriShape.mParents.push_back(&mNiNode2);

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));

        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        auto triShape = std::make_unique<Resource::TriangleMeshShape>(triangles.release(), true);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(triShape.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_two_tri_shape_children_should_return_static_shape_with_all_meshes)
    {
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiTriShape2.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape), Nif::NiAVObjectPtr(&mNiTriShape2) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<btTriangleMesh> triangles2(new btTriangleMesh(false));
        triangles2->addTriangle(btVector3(0, 0, 1), btVector3(1, 0, 1), btVector3(1, 1, 1));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        auto triShape = std::make_unique<Resource::TriangleMeshShape>(triangles.release(), true);
        auto triShape2 = std::make_unique<Resource::TriangleMeshShape>(triangles2.release(), true);

        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(triShape.release(), btVector3(1, 1, 1)));
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(triShape2.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader,
        for_tri_shape_child_node_and_filename_starting_with_x_and_not_empty_skin_should_return_static_shape)
    {
        mNiTriShape.mSkin = Nif::NiSkinInstancePtr(&mNiSkinInstance);
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        auto triShape = std::make_unique<Resource::TriangleMeshShape>(triangles.release(), true);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(triShape.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_root_node_and_filename_starting_with_x_should_return_animated_shape)
    {
        copy(mTransform, mNiTriShape.mTransform);
        mNiTriShape.mTransform.mScale = 3;

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiTriShape);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(mTransform, new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(3, 3, 3)));
        Resource::BulletShape expected;
        expected.mCollisionShape.reset(shape.release());
        expected.mAnimatedShapes = { { -1, 0 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_and_filename_starting_with_x_should_return_animated_shape)
    {
        copy(mTransform, mNiTriShape.mTransform);
        mNiTriShape.mTransform.mScale = 3;
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };
        mNiNode.mTransform.mScale = 4;

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(
            mTransformScale4, new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(12, 12, 12)));
        Resource::BulletShape expected;
        expected.mCollisionShape.reset(shape.release());
        expected.mAnimatedShapes = { { -1, 0 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(
        TestBulletNifLoader, for_two_tri_shape_children_nodes_and_filename_starting_with_x_should_return_animated_shape)
    {
        copy(mTransform, mNiTriShape.mTransform);
        mNiTriShape.mTransform.mScale = 3;
        mNiTriShape.mParents.push_back(&mNiNode);

        copy(mTransform, mNiTriShape2.mTransform);
        mNiTriShape2.mTransform.mScale = 3;
        mNiTriShape2.mParents.push_back(&mNiNode);

        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape), Nif::NiAVObjectPtr(&mNiTriShape2) };

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));

        std::unique_ptr<btTriangleMesh> triangles2(new btTriangleMesh(false));
        triangles2->addTriangle(btVector3(0, 0, 1), btVector3(1, 0, 1), btVector3(1, 1, 1));
        std::unique_ptr<Resource::TriangleMeshShape> mesh2(new Resource::TriangleMeshShape(triangles2.release(), true));

        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(mTransform, new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(3, 3, 3)));
        shape->addChildShape(mTransform, new Resource::ScaledTriangleMeshShape(mesh2.release(), btVector3(3, 3, 3)));
        Resource::BulletShape expected;
        expected.mCollisionShape.reset(shape.release());
        expected.mAnimatedShapes = { { -1, 0 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_controller_should_return_animated_shape)
    {
        mController.recType = Nif::RC_NiKeyframeController;
        mController.mFlags |= Nif::NiTimeController::Flag_Active;
        copy(mTransform, mNiTriShape.mTransform);
        mNiTriShape.mTransform.mScale = 3;
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiTriShape.mController = Nif::NiTimeControllerPtr(&mController);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };
        mNiNode.mTransform.mScale = 4;

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(
            mTransformScale4, new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(12, 12, 12)));
        Resource::BulletShape expected;
        expected.mCollisionShape.reset(shape.release());
        expected.mAnimatedShapes = { { -1, 0 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_two_tri_shape_children_nodes_where_one_with_controller_should_return_animated_shape)
    {
        mController.recType = Nif::RC_NiKeyframeController;
        mController.mFlags |= Nif::NiTimeController::Flag_Active;
        copy(mTransform, mNiTriShape.mTransform);
        mNiTriShape.mTransform.mScale = 3;
        mNiTriShape.mParents.push_back(&mNiNode);
        copy(mTransform, mNiTriShape2.mTransform);
        mNiTriShape2.mTransform.mScale = 3;
        mNiTriShape2.mParents.push_back(&mNiNode);
        mNiTriShape2.mController = Nif::NiTimeControllerPtr(&mController);
        mNiNode.mChildren = Nif::NiAVObjectList{
            Nif::NiAVObjectPtr(&mNiTriShape),
            Nif::NiAVObjectPtr(&mNiTriShape2),
        };
        mNiNode.mTransform.mScale = 4;

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));

        std::unique_ptr<btTriangleMesh> triangles2(new btTriangleMesh(false));
        triangles2->addTriangle(btVector3(0, 0, 1), btVector3(1, 0, 1), btVector3(1, 1, 1));
        std::unique_ptr<Resource::TriangleMeshShape> mesh2(new Resource::TriangleMeshShape(triangles2.release(), true));

        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(
            mTransformScale4, new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(12, 12, 12)));
        shape->addChildShape(
            mTransformScale4, new Resource::ScaledTriangleMeshShape(mesh2.release(), btVector3(12, 12, 12)));
        Resource::BulletShape expected;
        expected.mCollisionShape.reset(shape.release());
        expected.mAnimatedShapes = { { -1, 1 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, should_add_static_mesh_to_existing_compound_mesh)
    {
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiNode);
        file.mRoots.push_back(&mNiTriShape2);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));

        std::unique_ptr<btTriangleMesh> triangles2(new btTriangleMesh(false));
        triangles2->addTriangle(btVector3(0, 0, 1), btVector3(1, 0, 1), btVector3(1, 1, 1));
        std::unique_ptr<Resource::TriangleMeshShape> mesh2(new Resource::TriangleMeshShape(triangles2.release(), true));

        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh2.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());
        expected.mAnimatedShapes = { { -1, 0 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(
        TestBulletNifLoader, for_root_avoid_node_and_tri_shape_child_node_should_return_shape_with_null_collision_shape)
    {
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };
        mNiNode.recType = Nif::RC_AvoidNode;

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));

        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));
        Resource::BulletShape expected;
        expected.mAvoidCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_empty_data_should_return_shape_with_null_collision_shape)
    {
        mNiTriShape.mData = Nif::NiGeometryDataPtr(nullptr);
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader,
        for_tri_shape_child_node_with_empty_data_triangles_should_return_shape_with_null_collision_shape)
    {
        auto data = static_cast<Nif::NiTriShapeData*>(mNiTriShape.mData.getPtr());
        data->mTriangles.clear();
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader,
        for_root_node_with_extra_data_string_equal_ncc_should_return_shape_with_cameraonly_collision)
    {
        mNiStringExtraData.mData = "NCC__";
        mNiStringExtraData.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mExtra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        expected.mVisualCollisionType = Resource::VisualCollisionType::Camera;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader,
        for_root_node_with_not_first_extra_data_string_equal_ncc_should_return_shape_with_cameraonly_collision)
    {
        mNiStringExtraData.mNext = Nif::ExtraPtr(&mNiStringExtraData2);
        mNiStringExtraData2.mData = "NCC__";
        mNiStringExtraData2.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mExtra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());
        expected.mVisualCollisionType = Resource::VisualCollisionType::Camera;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(
        TestBulletNifLoader, for_root_node_with_extra_data_string_starting_with_nc_should_return_shape_with_nocollision)
    {
        mNiStringExtraData.mData = "NC___";
        mNiStringExtraData.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mExtra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());
        expected.mVisualCollisionType = Resource::VisualCollisionType::Default;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader,
        for_root_node_with_not_first_extra_data_string_starting_with_nc_should_return_shape_with_nocollision)
    {
        mNiStringExtraData.mNext = Nif::ExtraPtr(&mNiStringExtraData2);
        mNiStringExtraData2.mData = "NC___";
        mNiStringExtraData2.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mExtra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());
        expected.mVisualCollisionType = Resource::VisualCollisionType::Default;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_extra_data_string_should_ignore_extra_data)
    {
        mNiStringExtraData.mData = "NC___";
        mNiStringExtraData.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.mExtra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_empty_root_collision_node_without_nc_should_return_shape_with_cameraonly_collision)
    {
        Nif::NiTriShape niTriShape;
        Nif::NiNode emptyCollisionNode;
        init(niTriShape);
        init(emptyCollisionNode);

        niTriShape.mData = Nif::NiGeometryDataPtr(&mNiTriShapeData);
        niTriShape.mParents.push_back(&mNiNode);

        emptyCollisionNode.recType = Nif::RC_RootCollisionNode;
        emptyCollisionNode.mParents.push_back(&mNiNode);

        mNiNode.mChildren
            = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&niTriShape), Nif::NiAVObjectPtr(&emptyCollisionNode) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());
        expected.mVisualCollisionType = Resource::VisualCollisionType::Camera;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, bsx_editor_marker_flag_disables_collision_for_markers)
    {
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiTriShape.mName = "EditorMarker";
        mNiIntegerExtraData.mData = 34; // BSXFlags "has collision" | "editor marker"
        mNiIntegerExtraData.recType = Nif::RC_BSXFlags;
        mNiNode.mExtraList.push_back(Nif::ExtraPtr(&mNiIntegerExtraData));
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;
        file.mVersion = Nif::NIFStream::generateVersion(10, 0, 1, 0);

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, mrk_editor_marker_flag_disables_collision_for_markers)
    {
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiTriShape.mName = "Tri EditorMarker";
        mNiStringExtraData.mData = "MRK";
        mNiStringExtraData.recType = Nif::RC_NiStringExtraData;
        mNiNode.mExtra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_strips_root_node_should_return_static_shape)
    {
        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiTriStrips);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        triangles->addTriangle(btVector3(1, 0, 0), btVector3(0, 1, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, should_ignore_tri_strips_data_with_empty_strips)
    {
        mNiTriStripsData.mStrips.clear();

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiTriStrips);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        const Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_static_mesh_should_ignore_tri_strips_data_with_less_than_3_strips)
    {
        mNiTriStripsData.mStrips.front() = { 0, 1 };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiTriStrips);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        const Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_avoid_collision_mesh_should_ignore_tri_strips_data_with_less_than_3_strips)
    {
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };
        mNiNode.recType = Nif::RC_AvoidNode;
        mNiTriStripsData.mStrips.front() = { 0, 1 };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiTriStrips);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        const Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_animated_mesh_should_ignore_tri_strips_data_with_less_than_3_strips)
    {
        mNiTriStripsData.mStrips.front() = { 0, 1 };
        mNiTriStrips.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriStrips) };

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiNode);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        const Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, should_not_add_static_mesh_with_no_triangles_to_compound_shape)
    {
        mNiTriStripsData.mStrips.front() = { 0, 1 };
        mNiTriShape.mParents.push_back(&mNiNode);
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiNode);
        file.mRoots.push_back(&mNiTriStrips);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        std::unique_ptr<btCompoundShape> compound(new btCompoundShape);
        compound->addChildShape(
            btTransform::getIdentity(), new Resource::ScaledTriangleMeshShape(mesh.release(), btVector3(1, 1, 1)));

        Resource::BulletShape expected;
        expected.mCollisionShape.reset(compound.release());
        expected.mAnimatedShapes = { { -1, 0 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, should_handle_node_with_multiple_parents)
    {
        copy(mTransform, mNiTriShape.mTransform);
        mNiTriShape.mTransform.mScale = 4;
        mNiTriShape.mParents = { &mNiNode, &mNiNode2 };
        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };
        mNiNode.mTransform.mScale = 2;
        mNiNode2.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape) };
        mNiNode2.mTransform.mScale = 3;

        Nif::NIFFile file(xtestNif);
        file.mRoots.push_back(&mNiNode);
        file.mRoots.push_back(&mNiNode2);
        file.mHash = mHash;

        const auto result = mLoader.load(file);

        std::unique_ptr<btTriangleMesh> triangles1(new btTriangleMesh(false));
        triangles1->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh1(new Resource::TriangleMeshShape(triangles1.release(), true));
        std::unique_ptr<btTriangleMesh> triangles2(new btTriangleMesh(false));
        triangles2->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh2(new Resource::TriangleMeshShape(triangles2.release(), true));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(
            mTransformScale2, new Resource::ScaledTriangleMeshShape(mesh1.release(), btVector3(8, 8, 8)));
        shape->addChildShape(
            mTransformScale3, new Resource::ScaledTriangleMeshShape(mesh2.release(), btVector3(12, 12, 12)));
        Resource::BulletShape expected;
        expected.mCollisionShape.reset(shape.release());
        expected.mAnimatedShapes = { { -1, 0 } };

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, dont_assign_invalid_bounding_box_extents)
    {
        copy(mTransform, mNiTriShape.mTransform);
        mNiTriShape.mTransform.mScale = 10;
        mNiTriShape.mParents.push_back(&mNiNode);

        mNiTriShape2.mName = "Bounding Box";
        mNiTriShape2.mBounds.mType = Nif::BoundingVolume::Type::BOX_BV;
        mNiTriShape2.mBounds.mBox.mExtents = osg::Vec3f(-1, -2, -3);
        mNiTriShape2.mParents.push_back(&mNiNode);

        mNiNode.mChildren = Nif::NiAVObjectList{ Nif::NiAVObjectPtr(&mNiTriShape), Nif::NiAVObjectPtr(&mNiTriShape2) };

        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&mNiNode);

        const auto result = mLoader.load(file);

        const bool extentsUnassigned
            = std::ranges::all_of(result->mCollisionBox.mExtents._v, [](float extent) { return extent == 0.f; });

        EXPECT_EQ(extentsUnassigned, true);
    }
}
