#include <components/nifbullet/bulletnifloader.hpp>
#include <components/bullethelpers/processtrianglecallback.hpp>
#include <components/nif/node.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>

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
        auto callback = BulletHelpers::makeProcessTriangleCallback([&] (btVector3* triangle, int, int) {
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
        return std::equal(
            static_cast<const btScalar*>(lhs),
            static_cast<const btScalar*>(lhs) + 3,
            static_cast<const btScalar*>(rhs),
            [] (btScalar lhs, btScalar rhs) { return isNear(lhs, rhs); }
        );
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
}

static std::ostream& operator <<(std::ostream& stream, const btVector3& value)
{
    return stream << "btVector3 {"
        << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.getX() << ", "
        << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.getY() << ", "
        << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.getZ() << "}";
}

static std::ostream& operator <<(std::ostream& stream, const btMatrix3x3& value)
{
    stream << "btMatrix3x3 {";
    for (int i = 0; i < 3; ++i)
         stream << value.getRow(i) << ", ";
    return stream << "}";
}

static std::ostream& operator <<(std::ostream& stream, const btTransform& value)
{
    return stream << "btTransform {" << value.getBasis() << ", " << value.getOrigin() << "}";
}

static std::ostream& operator <<(std::ostream& stream, const btCollisionShape* value);

static std::ostream& operator <<(std::ostream& stream, const btCompoundShape& value)
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

static std::ostream& operator <<(std::ostream& stream, const btBoxShape& value)
{
    return stream << "btBoxShape {" << value.getLocalScaling() << ", " << value.getHalfExtentsWithoutMargin() << "}";
}

namespace Resource
{

static std::ostream& operator <<(std::ostream& stream, const TriangleMeshShape& value)
{
    stream << "Resource::TriangleMeshShape {" << value.getLocalScaling() << ", "
        << value.usesQuantizedAabbCompression() << ", " << value.getOwnsBvh() << ", {";
    auto callback = BulletHelpers::makeProcessTriangleCallback([&] (btVector3* triangle, int, int) {
        for (std::size_t i = 0; i < 3; ++i)
            stream << triangle[i] << ", ";
    });
    btVector3 aabbMin;
    btVector3 aabbMax;
    value.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
    value.processAllTriangles(&callback, aabbMin, aabbMax);
    return stream << "}}";
}

}

static std::ostream& operator <<(std::ostream& stream, const btCollisionShape& value)
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
    }
    return stream << "btCollisionShape {" << value.getShapeType() << "}";
}

static std::ostream& operator <<(std::ostream& stream, const btCollisionShape* value)
{
    return value ? stream << "&" << *value : stream << "nullptr";
}

namespace std
{
    static std::ostream& operator <<(std::ostream& stream, const map<int, int>& value)
    {
        stream << "std::map<int, int> {";
        for (const auto& v : value)
            stream << "{" << v.first << ", " << v.second << "},";
        return stream << "}";
    }
}

namespace Resource
{
    static bool operator ==(const Resource::BulletShape& lhs, const Resource::BulletShape& rhs)
    {
        return compareObjects(lhs.mCollisionShape, rhs.mCollisionShape)
            && compareObjects(lhs.mAvoidCollisionShape, rhs.mAvoidCollisionShape)
            && lhs.mCollisionBox.extents == rhs.mCollisionBox.extents
            && lhs.mCollisionBox.center == rhs.mCollisionBox.center
            && lhs.mAnimatedShapes == rhs.mAnimatedShapes;
    }

    static std::ostream& operator <<(std::ostream& stream, const Resource::BulletShape& value)
    {
        return stream << "Resource::BulletShape {"
            << value.mCollisionShape << ", "
            << value.mAvoidCollisionShape << ", "
            << "osg::Vec3f {" << value.mCollisionBox.extents << "}" << ", "
            << "osg::Vec3f {" << value.mCollisionBox.center << "}" << ", "
            << value.mAnimatedShapes
            << "}";
    }
}

static bool operator ==(const btCollisionShape& lhs, const btCollisionShape& rhs);

static bool operator ==(const btCompoundShape& lhs, const btCompoundShape& rhs)
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

static bool operator ==(const btBoxShape& lhs, const btBoxShape& rhs)
{
    return isNear(lhs.getLocalScaling(), rhs.getLocalScaling())
        && lhs.getHalfExtentsWithoutMargin() == rhs.getHalfExtentsWithoutMargin();
}

static bool operator ==(const btBvhTriangleMeshShape& lhs, const btBvhTriangleMeshShape& rhs)
{
    return isNear(lhs.getLocalScaling(), rhs.getLocalScaling())
        && lhs.usesQuantizedAabbCompression() == rhs.usesQuantizedAabbCompression()
        && lhs.getOwnsBvh() == rhs.getOwnsBvh()
        && getTriangles(lhs) == getTriangles(rhs);
}

static bool operator ==(const btCollisionShape& lhs, const btCollisionShape& rhs)
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
    }
    return false;
}

namespace
{
    using namespace testing;
    using NifBullet::BulletNifLoader;

    void init(Nif::Transformation& value)
    {
        value = Nif::Transformation::getIdentity();
    }

    void init(Nif::Extra& value)
    {
        value.next = Nif::ExtraPtr(nullptr);
    }

    void init(Nif::Named& value)
    {
        value.extra = Nif::ExtraPtr(nullptr);
        value.extralist = Nif::ExtraList();
        value.controller = Nif::ControllerPtr(nullptr);
    }

    void init(Nif::Node& value)
    {
        init(static_cast<Nif::Named&>(value));
        value.flags = 0;
        init(value.trafo);
        value.hasBounds = false;
        value.parent = nullptr;
        value.isBone = false;
    }

    void init(Nif::NiGeometry& value)
    {
        init(static_cast<Nif::Node&>(value));
        value.data = Nif::NiGeometryDataPtr(nullptr);
        value.skin = Nif::NiSkinInstancePtr(nullptr);
    }

    void init(Nif::NiTriShape& value)
    {
        init(static_cast<Nif::NiGeometry&>(value));
        value.recType = Nif::RC_NiTriShape;
    }

    void init(Nif::NiSkinInstance& value)
    {
        value.data = Nif::NiSkinDataPtr(nullptr);
        value.root = Nif::NodePtr(nullptr);
    }

    void init(Nif::Controller& value)
    {
        value.next = Nif::ControllerPtr(nullptr);
        value.flags = 0;
        value.frequency = 0;
        value.phase = 0;
        value.timeStart = 0;
        value.timeStop = 0;
        value.target = Nif::NamedPtr(nullptr);
    }

    void copy(const btTransform& src, Nif::Transformation& dst)
    {
        dst.pos = osg::Vec3f(src.getOrigin().x(), src.getOrigin().y(), src.getOrigin().z());
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
                dst.rotation.mValues[column][row] = src.getBasis().getRow(row)[column];
    }

    struct NifFileMock : Nif::File
    {
        MOCK_METHOD(Nif::Record*, getRecord, (std::size_t), (const, override));
        MOCK_METHOD(std::size_t, numRecords, (), (const, override));
        MOCK_METHOD(Nif::Record*, getRoot, (std::size_t), (const, override));
        MOCK_METHOD(std::size_t, numRoots, (), (const, override));
        MOCK_METHOD(std::string, getString, (uint32_t), (const, override));
        MOCK_METHOD(void, setUseSkinning, (bool), (override));
        MOCK_METHOD(bool, getUseSkinning, (), (const, override));
        MOCK_METHOD(std::string, getFilename, (), (const, override));
        MOCK_METHOD(unsigned int, getVersion, (), (const, override));
        MOCK_METHOD(unsigned int, getUserVersion, (), (const, override));
        MOCK_METHOD(unsigned int, getBethVersion, (), (const, override));
    };

    struct RecordMock : Nif::Record
    {
        MOCK_METHOD(void, read, (Nif::NIFStream *nif), (override));
    };

    struct TestBulletNifLoader : Test
    {
        BulletNifLoader mLoader;
        const StrictMock<const NifFileMock> mNifFile;
        Nif::Node mNode;
        Nif::Node mNode2;
        Nif::NiNode mNiNode;
        Nif::NiNode mNiNode2;
        Nif::NiNode mNiNode3;
        Nif::NiTriShapeData mNiTriShapeData;
        Nif::NiTriShape mNiTriShape;
        Nif::NiTriShapeData mNiTriShapeData2;
        Nif::NiTriShape mNiTriShape2;
        Nif::NiSkinInstance mNiSkinInstance;
        Nif::NiStringExtraData mNiStringExtraData;
        Nif::NiStringExtraData mNiStringExtraData2;
        Nif::Controller mController;
        btTransform mTransform {btMatrix3x3(btQuaternion(btVector3(1, 0, 0), 0.5f)), btVector3(1, 2, 3)};
        btTransform mResultTransform {
            btMatrix3x3(
                1, 0, 0,
                0, 0.82417738437652587890625, 0.56633174419403076171875,
                0, -0.56633174419403076171875, 0.82417738437652587890625
            ),
            btVector3(1, 2, 3)
        };
        btTransform mResultTransform2 {
            btMatrix3x3(
                1, 0, 0,
                0, 0.7951543331146240234375, 0.606407105922698974609375,
                0, -0.606407105922698974609375, 0.7951543331146240234375
            ),
            btVector3(4, 8, 12)
        };

        TestBulletNifLoader()
        {
            init(mNode);
            init(mNode2);
            init(mNiNode);
            init(mNiNode2);
            init(mNiNode3);
            init(mNiTriShape);
            init(mNiTriShape2);
            init(mNiSkinInstance);
            init(mNiStringExtraData);
            init(mNiStringExtraData2);
            init(mController);

            mNiTriShapeData.recType = Nif::RC_NiTriShapeData;
            mNiTriShapeData.vertices = {osg::Vec3f(0, 0, 0), osg::Vec3f(1, 0, 0), osg::Vec3f(1, 1, 0)};
            mNiTriShapeData.triangles = {0, 1, 2};
            mNiTriShape.data = Nif::NiGeometryDataPtr(&mNiTriShapeData);

            mNiTriShapeData2.recType = Nif::RC_NiTriShapeData;
            mNiTriShapeData2.vertices = {osg::Vec3f(0, 0, 1), osg::Vec3f(1, 0, 1), osg::Vec3f(1, 1, 1)};
            mNiTriShapeData2.triangles = {0, 1, 2};
            mNiTriShape2.data = Nif::NiGeometryDataPtr(&mNiTriShapeData2);
        }
    };

    TEST_F(TestBulletNifLoader, for_zero_num_roots_should_return_default)
    {
        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(0));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_default_root_nif_node_should_return_default)
    {
        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_default_root_collision_node_nif_node_should_return_default)
    {
        mNode.recType = Nif::RC_RootCollisionNode;

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_default_root_nif_node_and_filename_starting_with_x_should_return_default)
    {
        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("xtest.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_nif_node_with_bounding_box_should_return_shape_with_compound_shape_and_box_inside)
    {
        mNode.hasBounds = true;
        mNode.flags |= Nif::NiNode::Flag_BBoxCollision;
        mNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode.bounds.box.extents = osg::Vec3f(1, 2, 3);
        mNode.bounds.box.center = osg::Vec3f(-1, -2, -3);

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;
        expected.mCollisionBox.extents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.center = osg::Vec3f(-1, -2, -3);
        std::unique_ptr<btBoxShape> box(new btBoxShape(btVector3(1, 2, 3)));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(-1, -2, -3)), box.release());
        expected.mCollisionShape = shape.release();

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_child_nif_node_with_bounding_box)
    {
        mNode.hasBounds = true;
        mNode.flags |= Nif::NiNode::Flag_BBoxCollision;
        mNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode.bounds.box.extents = osg::Vec3f(1, 2, 3);
        mNode.bounds.box.center = osg::Vec3f(-1, -2, -3);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNode)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;
        expected.mCollisionBox.extents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.center = osg::Vec3f(-1, -2, -3);
        std::unique_ptr<btBoxShape> box(new btBoxShape(btVector3(1, 2, 3)));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(-1, -2, -3)), box.release());
        expected.mCollisionShape = shape.release();

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_and_child_nif_node_with_bounding_box_but_root_without_flag_should_use_child_bounds)
    {
        mNode.hasBounds = true;
        mNode.flags |= Nif::NiNode::Flag_BBoxCollision;
        mNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode.bounds.box.extents = osg::Vec3f(1, 2, 3);
        mNode.bounds.box.center = osg::Vec3f(-1, -2, -3);

        mNiNode.hasBounds = true;
        mNiNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNiNode.bounds.box.extents = osg::Vec3f(4, 5, 6);
        mNiNode.bounds.box.center = osg::Vec3f(-4, -5, -6);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNode)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;
        expected.mCollisionBox.extents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.center = osg::Vec3f(-1, -2, -3);
        std::unique_ptr<btBoxShape> box(new btBoxShape(btVector3(1, 2, 3)));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(-1, -2, -3)), box.release());
        expected.mCollisionShape = shape.release();

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_and_two_children_where_both_with_bounds_but_only_first_with_flag_should_use_first_bounds)
    {
        mNode.hasBounds = true;
        mNode.flags |= Nif::NiNode::Flag_BBoxCollision;
        mNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode.bounds.box.extents = osg::Vec3f(1, 2, 3);
        mNode.bounds.box.center = osg::Vec3f(-1, -2, -3);

        mNode2.hasBounds = true;
        mNode2.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode2.bounds.box.extents = osg::Vec3f(4, 5, 6);
        mNode2.bounds.box.center = osg::Vec3f(-4, -5, -6);

        mNiNode.hasBounds = true;
        mNiNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNiNode.bounds.box.extents = osg::Vec3f(7, 8, 9);
        mNiNode.bounds.box.center = osg::Vec3f(-7, -8, -9);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNode), Nif::NodePtr(&mNode2)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;
        expected.mCollisionBox.extents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.center = osg::Vec3f(-1, -2, -3);
        std::unique_ptr<btBoxShape> box(new btBoxShape(btVector3(1, 2, 3)));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(-1, -2, -3)), box.release());
        expected.mCollisionShape = shape.release();

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_and_two_children_where_both_with_bounds_but_only_second_with_flag_should_use_second_bounds)
    {
        mNode.hasBounds = true;
        mNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode.bounds.box.extents = osg::Vec3f(1, 2, 3);
        mNode.bounds.box.center = osg::Vec3f(-1, -2, -3);

        mNode2.hasBounds = true;
        mNode2.flags |= Nif::NiNode::Flag_BBoxCollision;
        mNode2.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode2.bounds.box.extents = osg::Vec3f(4, 5, 6);
        mNode2.bounds.box.center = osg::Vec3f(-4, -5, -6);

        mNiNode.hasBounds = true;
        mNiNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNiNode.bounds.box.extents = osg::Vec3f(7, 8, 9);
        mNiNode.bounds.box.center = osg::Vec3f(-7, -8, -9);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNode), Nif::NodePtr(&mNode2)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;
        expected.mCollisionBox.extents = osg::Vec3f(4, 5, 6);
        expected.mCollisionBox.center = osg::Vec3f(-4, -5, -6);
        std::unique_ptr<btBoxShape> box(new btBoxShape(btVector3(4, 5, 6)));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(-4, -5, -6)), box.release());
        expected.mCollisionShape = shape.release();

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_nif_node_with_bounds_but_without_flag_should_return_shape_with_bounds_but_with_null_collision_shape)
    {
        mNode.hasBounds = true;
        mNode.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNode.bounds.box.extents = osg::Vec3f(1, 2, 3);
        mNode.bounds.box.center = osg::Vec3f(-1, -2, -3);

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;
        expected.mCollisionBox.extents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.center = osg::Vec3f(-1, -2, -3);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_root_node_should_return_shape_with_triangle_mesh_shape)
    {
        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiTriShape));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        Resource::BulletShape expected;
        expected.mCollisionShape = new Resource::TriangleMeshShape(triangles.release(), true);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_root_node_with_bounds_should_return_shape_with_bounds_but_with_null_collision_shape)
    {
        mNiTriShape.hasBounds = true;
        mNiTriShape.bounds.type = Nif::NiBoundingVolume::Type::BOX_BV;
        mNiTriShape.bounds.box.extents = osg::Vec3f(1, 2, 3);
        mNiTriShape.bounds.box.center = osg::Vec3f(-1, -2, -3);

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiTriShape));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;
        expected.mCollisionBox.extents = osg::Vec3f(1, 2, 3);
        expected.mCollisionBox.center = osg::Vec3f(-1, -2, -3);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_should_return_shape_with_triangle_mesh_shape)
    {
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        Resource::BulletShape expected;
        expected.mCollisionShape = new Resource::TriangleMeshShape(triangles.release(), true);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_nested_tri_shape_child_should_return_shape_with_triangle_mesh_shape)
    {
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiNode2)}));
        mNiNode2.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        Resource::BulletShape expected;
        expected.mCollisionShape = new Resource::TriangleMeshShape(triangles.release(), true);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_two_tri_shape_children_should_return_shape_with_triangle_mesh_shape_with_all_meshes)
    {
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({
            Nif::NodePtr(&mNiTriShape),
            Nif::NodePtr(&mNiTriShape2)
        }));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 1), btVector3(1, 0, 1), btVector3(1, 1, 1));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        Resource::BulletShape expected;
        expected.mCollisionShape = new Resource::TriangleMeshShape(triangles.release(), true);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_and_filename_starting_with_x_and_not_empty_skin_should_return_shape_with_triangle_mesh_shape)
    {
        mNiTriShape.skin = Nif::NiSkinInstancePtr(&mNiSkinInstance);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("xtest.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        Resource::BulletShape expected;
        expected.mCollisionShape = new Resource::TriangleMeshShape(triangles.release(), true);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_root_node_and_filename_starting_with_x_should_return_shape_with_compound_shape)
    {
        copy(mTransform, mNiTriShape.trafo);
        mNiTriShape.trafo.scale = 3;

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiTriShape));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("xtest.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        mesh->setLocalScaling(btVector3(3, 3, 3));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(mResultTransform, mesh.release());
        Resource::BulletShape expected;
        expected.mCollisionShape = shape.release();
        expected.mAnimatedShapes = {{-1, 0}};

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_and_filename_starting_with_x_should_return_shape_with_compound_shape)
    {
        copy(mTransform, mNiTriShape.trafo);
        mNiTriShape.trafo.scale = 3;
        mNiTriShape.parent = &mNiNode;
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));
        mNiNode.trafo.scale = 4;

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("xtest.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        mesh->setLocalScaling(btVector3(12, 12, 12));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(mResultTransform2, mesh.release());
        Resource::BulletShape expected;
        expected.mCollisionShape = shape.release();
        expected.mAnimatedShapes = {{-1, 0}};

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_two_tri_shape_children_nodes_and_filename_starting_with_x_should_return_shape_with_compound_shape)
    {
        copy(mTransform, mNiTriShape.trafo);
        mNiTriShape.trafo.scale = 3;

        copy(mTransform, mNiTriShape2.trafo);
        mNiTriShape2.trafo.scale = 3;

        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({
            Nif::NodePtr(&mNiTriShape),
            Nif::NodePtr(&mNiTriShape2),
        }));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("xtest.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        mesh->setLocalScaling(btVector3(3, 3, 3));

        std::unique_ptr<btTriangleMesh> triangles2(new btTriangleMesh(false));
        triangles2->addTriangle(btVector3(0, 0, 1), btVector3(1, 0, 1), btVector3(1, 1, 1));
        std::unique_ptr<Resource::TriangleMeshShape> mesh2(new Resource::TriangleMeshShape(triangles2.release(), true));
        mesh2->setLocalScaling(btVector3(3, 3, 3));

        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(mResultTransform, mesh.release());
        shape->addChildShape(mResultTransform, mesh2.release());
        Resource::BulletShape expected;
        expected.mCollisionShape = shape.release();
        expected.mAnimatedShapes = {{-1, 0}};

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_controller_should_return_shape_with_compound_shape)
    {
        mController.recType = Nif::RC_NiKeyframeController;
        mController.flags |= Nif::NiNode::ControllerFlag_Active;
        copy(mTransform, mNiTriShape.trafo);
        mNiTriShape.trafo.scale = 3;
        mNiTriShape.parent = &mNiNode;
        mNiTriShape.controller = Nif::ControllerPtr(&mController);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));
        mNiNode.trafo.scale = 4;

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        mesh->setLocalScaling(btVector3(12, 12, 12));
        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(mResultTransform2, mesh.release());
        Resource::BulletShape expected;
        expected.mCollisionShape = shape.release();
        expected.mAnimatedShapes = {{-1, 0}};

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_two_tri_shape_children_nodes_where_one_with_controller_should_return_shape_with_compound_shape)
    {
        mController.recType = Nif::RC_NiKeyframeController;
        mController.flags |= Nif::NiNode::ControllerFlag_Active;
        copy(mTransform, mNiTriShape.trafo);
        mNiTriShape.trafo.scale = 3;
        copy(mTransform, mNiTriShape2.trafo);
        mNiTriShape2.trafo.scale = 3;
        mNiTriShape2.parent = &mNiNode;
        mNiTriShape2.controller = Nif::ControllerPtr(&mController);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({
            Nif::NodePtr(&mNiTriShape),
            Nif::NodePtr(&mNiTriShape2),
        }));
        mNiNode.trafo.scale = 4;

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(1, 2, 3), btVector3(4, 2, 3), btVector3(4, 4.632747650146484375, 1.56172335147857666015625));
        std::unique_ptr<Resource::TriangleMeshShape> mesh(new Resource::TriangleMeshShape(triangles.release(), true));
        mesh->setLocalScaling(btVector3(1, 1, 1));

        std::unique_ptr<btTriangleMesh> triangles2(new btTriangleMesh(false));
        triangles2->addTriangle(btVector3(0, 0, 1), btVector3(1, 0, 1), btVector3(1, 1, 1));
        std::unique_ptr<Resource::TriangleMeshShape> mesh2(new Resource::TriangleMeshShape(triangles2.release(), true));
        mesh2->setLocalScaling(btVector3(12, 12, 12));

        std::unique_ptr<btCompoundShape> shape(new btCompoundShape);
        shape->addChildShape(mResultTransform2, mesh2.release());
        shape->addChildShape(btTransform::getIdentity(), mesh.release());
        Resource::BulletShape expected;
        expected.mCollisionShape = shape.release();
        expected.mAnimatedShapes = {{-1, 0}};

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_root_avoid_node_and_tri_shape_child_node_should_return_shape_with_null_collision_shape)
    {
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));
        mNiNode.recType = Nif::RC_AvoidNode;

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        Resource::BulletShape expected;
        expected.mAvoidCollisionShape = new Resource::TriangleMeshShape(triangles.release(), false);

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_empty_data_should_return_shape_with_null_collision_shape)
    {
        mNiTriShape.data = Nif::NiGeometryDataPtr(nullptr);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_empty_data_triangles_should_return_shape_with_null_collision_shape)
    {
        auto data = static_cast<Nif::NiTriShapeData*>(mNiTriShape.data.getPtr());
        data->triangles.clear();
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_extra_data_string_starting_with_nc_should_return_shape_with_null_collision_shape)
    {
        mNiStringExtraData.string = "NC___";
        mNiStringExtraData.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.extra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_not_first_extra_data_string_starting_with_nc_should_return_shape_with_null_collision_shape)
    {
        mNiStringExtraData.next = Nif::ExtraPtr(&mNiStringExtraData2);
        mNiStringExtraData2.string = "NC___";
        mNiStringExtraData2.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.extra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_extra_data_string_mrk_should_return_shape_with_null_collision_shape)
    {
        mNiStringExtraData.string = "MRK";
        mNiStringExtraData.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.extra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        Resource::BulletShape expected;

        EXPECT_EQ(*result, expected);
    }

    TEST_F(TestBulletNifLoader, for_tri_shape_child_node_with_extra_data_string_mrk_and_other_collision_node_should_return_shape_with_triangle_mesh_shape_with_all_meshes)
    {
        mNiStringExtraData.string = "MRK";
        mNiStringExtraData.recType = Nif::RC_NiStringExtraData;
        mNiTriShape.extra = Nif::ExtraPtr(&mNiStringExtraData);
        mNiNode2.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiTriShape)}));
        mNiNode2.recType = Nif::RC_RootCollisionNode;
        mNiNode.children = Nif::NodeList(std::vector<Nif::NodePtr>({Nif::NodePtr(&mNiNode2)}));
        mNiNode.recType = Nif::RC_NiNode;

        EXPECT_CALL(mNifFile, numRoots()).WillOnce(Return(1));
        EXPECT_CALL(mNifFile, getRoot(0)).WillOnce(Return(&mNiNode));
        EXPECT_CALL(mNifFile, getFilename()).WillOnce(Return("test.nif"));
        const auto result = mLoader.load(mNifFile);

        std::unique_ptr<btTriangleMesh> triangles(new btTriangleMesh(false));
        triangles->addTriangle(btVector3(0, 0, 0), btVector3(1, 0, 0), btVector3(1, 1, 0));
        Resource::BulletShape expected;
        expected.mCollisionShape = new Resource::TriangleMeshShape(triangles.release(), true);

        EXPECT_EQ(*result, expected);
    }
}
