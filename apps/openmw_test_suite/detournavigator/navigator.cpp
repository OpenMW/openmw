#include "operators.hpp"

#include <components/detournavigator/navigatorimpl.hpp>
#include <components/detournavigator/exceptions.hpp>
#include <components/misc/rng.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/resource/bulletshape.hpp>

#include <osg/ref_ptr>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <deque>
#include <memory>

MATCHER_P3(Vec3fEq, x, y, z, "")
{
    return std::abs(arg.x() - x) < 1e-4 && std::abs(arg.y() - y) < 1e-4 && std::abs(arg.z() - z) < 1e-4;
}

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct DetourNavigatorNavigatorTest : Test
    {
        Settings mSettings;
        std::unique_ptr<Navigator> mNavigator;
        osg::Vec3f mPlayerPosition;
        osg::Vec3f mAgentHalfExtents;
        osg::Vec3f mStart;
        osg::Vec3f mEnd;
        std::deque<osg::Vec3f> mPath;
        std::back_insert_iterator<std::deque<osg::Vec3f>> mOut;
        float mStepSize;
        AreaCosts mAreaCosts;
        Loading::Listener mListener;

        DetourNavigatorNavigatorTest()
            : mPlayerPosition(0, 0, 0)
            , mAgentHalfExtents(29, 29, 66)
            , mStart(-204, 204, 1)
            , mEnd(204, -204, 1)
            , mOut(mPath)
            , mStepSize(28.333332061767578125f)
        {
            mSettings.mEnableWriteRecastMeshToFile = false;
            mSettings.mEnableWriteNavMeshToFile = false;
            mSettings.mEnableRecastMeshFileNameRevision = false;
            mSettings.mEnableNavMeshFileNameRevision = false;
            mSettings.mBorderSize = 16;
            mSettings.mCellHeight = 0.2f;
            mSettings.mCellSize = 0.2f;
            mSettings.mDetailSampleDist = 6;
            mSettings.mDetailSampleMaxError = 1;
            mSettings.mMaxClimb = 34;
            mSettings.mMaxSimplificationError = 1.3f;
            mSettings.mMaxSlope = 49;
            mSettings.mRecastScaleFactor = 0.017647058823529415f;
            mSettings.mSwimHeightScale = 0.89999997615814208984375f;
            mSettings.mMaxEdgeLen = 12;
            mSettings.mMaxNavMeshQueryNodes = 2048;
            mSettings.mMaxVertsPerPoly = 6;
            mSettings.mRegionMergeSize = 20;
            mSettings.mRegionMinSize = 8;
            mSettings.mTileSize = 64;
            mSettings.mWaitUntilMinDistanceToPlayer = std::numeric_limits<int>::max();
            mSettings.mAsyncNavMeshUpdaterThreads = 1;
            mSettings.mMaxNavMeshTilesCacheSize = 1024 * 1024;
            mSettings.mMaxPolygonPathSize = 1024;
            mSettings.mMaxSmoothPathSize = 1024;
            mSettings.mMaxPolys = 4096;
            mSettings.mMaxTilesNumber = 512;
            mSettings.mMinUpdateInterval = std::chrono::milliseconds(50);
            mNavigator.reset(new NavigatorImpl(mSettings));
        }
    };

    template <std::size_t size>
    std::unique_ptr<btHeightfieldTerrainShape> makeSquareHeightfieldTerrainShape(const std::array<btScalar, size>& values,
        btScalar heightScale = 1, int upAxis = 2, PHY_ScalarType heightDataType = PHY_FLOAT, bool flipQuadEdges = false)
    {
        const int width = static_cast<int>(std::sqrt(size));
        const btScalar min = *std::min_element(values.begin(), values.end());
        const btScalar max = *std::max_element(values.begin(), values.end());
        const btScalar greater = std::max(std::abs(min), std::abs(max));
        return std::make_unique<btHeightfieldTerrainShape>(width, width, values.data(), heightScale, -greater, greater,
                                                           upAxis, heightDataType, flipQuadEdges);
    }

    template <class T>
    osg::ref_ptr<const Resource::BulletShapeInstance> makeBulletShapeInstance(std::unique_ptr<T>&& shape)
    {
        osg::ref_ptr<Resource::BulletShape> bulletShape(new Resource::BulletShape);
        bulletShape->mCollisionShape = std::move(shape).release();
        return new Resource::BulletShapeInstance(bulletShape);
    }

    template <class T>
    class CollisionShapeInstance
    {
    public:
        CollisionShapeInstance(std::unique_ptr<T>&& shape) : mInstance(makeBulletShapeInstance(std::move(shape))) {}

        T& shape() { return static_cast<T&>(*mInstance->mCollisionShape); }
        const osg::ref_ptr<const Resource::BulletShapeInstance>& instance() const { return mInstance; }

    private:
        osg::ref_ptr<const Resource::BulletShapeInstance> mInstance;
    };

    TEST_F(DetourNavigatorNavigatorTest, find_path_for_empty_should_return_empty)
    {
        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut),
                  Status::NavMeshNotFound);
        EXPECT_EQ(mPath, std::deque<osg::Vec3f>());
    }

    TEST_F(DetourNavigatorNavigatorTest, find_path_for_existing_agent_with_no_navmesh_should_throw_exception)
    {
        mNavigator->addAgent(mAgentHalfExtents);
        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut),
                  Status::StartPolygonNotFound);
    }

    TEST_F(DetourNavigatorNavigatorTest, add_agent_should_count_each_agent)
    {
        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->removeAgent(mAgentHalfExtents);
        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut),
                  Status::StartPolygonNotFound);
    }

    TEST_F(DetourNavigatorNavigatorTest, update_then_find_path_should_return_path)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::requiredTilesPresent);

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204.0000152587890625, 204, 1.99998295307159423828125),
            Vec3fEq(-183.96533203125, 183.9653167724609375, 1.99998819828033447265625),
            Vec3fEq(-163.930633544921875, 163.9306182861328125, 1.99999344348907470703125),
            Vec3fEq(-143.8959503173828125, 143.89593505859375, -2.720611572265625),
            Vec3fEq(-123.86126708984375, 123.86124420166015625, -13.1089687347412109375),
            Vec3fEq(-103.82657623291015625, 103.8265533447265625, -23.497333526611328125),
            Vec3fEq(-83.7918853759765625, 83.7918548583984375, -33.885692596435546875),
            Vec3fEq(-63.757190704345703125, 63.757171630859375, -44.274051666259765625),
            Vec3fEq(-43.722503662109375, 43.72248077392578125, -54.66241455078125),
            Vec3fEq(-23.687808990478515625, 23.6877918243408203125, -65.05077362060546875),
            Vec3fEq(-3.6531188488006591796875, 3.6531002521514892578125, -75.43914031982421875),
            Vec3fEq(16.3815746307373046875, -16.381591796875, -69.74927520751953125),
            Vec3fEq(36.416263580322265625, -36.416286468505859375, -60.4739532470703125),
            Vec3fEq(56.450958251953125, -56.450977325439453125, -51.1986236572265625),
            Vec3fEq(76.48564910888671875, -76.4856719970703125, -41.92330169677734375),
            Vec3fEq(96.5203399658203125, -96.52036285400390625, -31.46941375732421875),
            Vec3fEq(116.55503082275390625, -116.5550537109375, -19.597003936767578125),
            Vec3fEq(136.5897216796875, -136.5897369384765625, -7.724592685699462890625),
            Vec3fEq(156.624420166015625, -156.624420166015625, 1.99999535083770751953125),
            Vec3fEq(176.6591033935546875, -176.65911865234375, 1.99999010562896728515625),
            Vec3fEq(196.69378662109375, -196.6938018798828125, 1.99998486042022705078125),
            Vec3fEq(204, -204.0000152587890625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, add_object_should_change_navmesh)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto heightfieldShapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& heightfieldShape = *heightfieldShapePtr;
        heightfieldShape.setLocalScaling(btVector3(128, 128, 1));

        CollisionShapeInstance compound(std::make_unique<btCompoundShape>());
        compound.shape().addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(0, 0, 0)), new btBoxShape(btVector3(20, 20, 100)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&heightfieldShape), nullptr, heightfieldShape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.99998295307159423828125),
            Vec3fEq(-183.965301513671875, 183.965301513671875, 1.99998819828033447265625),
            Vec3fEq(-163.9306182861328125, 163.9306182861328125, 1.99999344348907470703125),
            Vec3fEq(-143.89593505859375, 143.89593505859375, -2.7206256389617919921875),
            Vec3fEq(-123.86124420166015625, 123.86124420166015625, -13.1089839935302734375),
            Vec3fEq(-103.8265533447265625, 103.8265533447265625, -23.4973468780517578125),
            Vec3fEq(-83.7918548583984375, 83.7918548583984375, -33.885707855224609375),
            Vec3fEq(-63.75716400146484375, 63.75716400146484375, -44.27407073974609375),
            Vec3fEq(-43.72247314453125, 43.72247314453125, -54.662433624267578125),
            Vec3fEq(-23.6877803802490234375, 23.6877803802490234375, -65.0507965087890625),
            Vec3fEq(-3.653090000152587890625, 3.653090000152587890625, -75.43915557861328125),
            Vec3fEq(16.3816013336181640625, -16.3816013336181640625, -69.749267578125),
            Vec3fEq(36.416290283203125, -36.416290283203125, -60.4739532470703125),
            Vec3fEq(56.450984954833984375, -56.450984954833984375, -51.1986236572265625),
            Vec3fEq(76.4856719970703125, -76.4856719970703125, -41.92330169677734375),
            Vec3fEq(96.52036285400390625, -96.52036285400390625, -31.46941375732421875),
            Vec3fEq(116.5550537109375, -116.5550537109375, -19.597003936767578125),
            Vec3fEq(136.5897369384765625, -136.5897369384765625, -7.724592685699462890625),
            Vec3fEq(156.6244354248046875, -156.6244354248046875, 1.99999535083770751953125),
            Vec3fEq(176.6591339111328125, -176.6591339111328125, 1.99999010562896728515625),
            Vec3fEq(196.693817138671875, -196.693817138671875, 1.99998486042022705078125),
            Vec3fEq(204, -204, 1.99998295307159423828125)
        )) << mPath;

        mNavigator->addObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance()), btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mPath.clear();
        mOut = std::back_inserter(mPath);
        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.99998295307159423828125),
            Vec3fEq(-189.9427337646484375, 179.3997802734375, -3.622931003570556640625),
            Vec3fEq(-175.8854522705078125, 154.7995452880859375, -9.24583911895751953125),
            Vec3fEq(-161.82818603515625, 130.1993255615234375, -14.86874866485595703125),
            Vec3fEq(-147.770904541015625, 105.5991058349609375, -20.4916591644287109375),
            Vec3fEq(-133.7136383056640625, 80.99887847900390625, -26.1145648956298828125),
            Vec3fEq(-119.65636444091796875, 56.39865875244140625, -31.7374725341796875),
            Vec3fEq(-105.59909820556640625, 31.798435211181640625, -26.133396148681640625),
            Vec3fEq(-91.54183197021484375, 7.1982135772705078125, -31.5624217987060546875),
            Vec3fEq(-77.48455810546875, -17.402008056640625, -26.98972320556640625),
            Vec3fEq(-63.427295684814453125, -42.00223541259765625, -19.9045581817626953125),
            Vec3fEq(-42.193531036376953125, -60.761363983154296875, -20.4544773101806640625),
            Vec3fEq(-20.9597682952880859375, -79.5204925537109375, -23.599918365478515625),
            Vec3fEq(3.8312885761260986328125, -93.2384033203125, -30.7141361236572265625),
            Vec3fEq(28.6223468780517578125, -106.95632171630859375, -24.8243885040283203125),
            Vec3fEq(53.413402557373046875, -120.6742401123046875, -31.3303241729736328125),
            Vec3fEq(78.20446014404296875, -134.39215087890625, -25.8431549072265625),
            Vec3fEq(102.99552154541015625, -148.110076904296875, -20.3559894561767578125),
            Vec3fEq(127.7865753173828125, -161.827972412109375, -14.868824005126953125),
            Vec3fEq(152.57763671875, -175.5458984375, -9.3816623687744140625),
            Vec3fEq(177.3686981201171875, -189.2638092041015625, -3.894496917724609375),
            Vec3fEq(202.1597442626953125, -202.9817047119140625, 1.59266507625579833984375),
            Vec3fEq(204, -204, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_changed_object_should_change_navmesh)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto heightfieldShapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& heightfieldShape = *heightfieldShapePtr;
        heightfieldShape.setLocalScaling(btVector3(128, 128, 1));

        CollisionShapeInstance compound(std::make_unique<btCompoundShape>());
        compound.shape().addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(0, 0, 0)), new btBoxShape(btVector3(20, 20, 100)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&heightfieldShape), nullptr, heightfieldShape, btTransform::getIdentity());
        mNavigator->addObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance()), btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.99998295307159423828125),
            Vec3fEq(-189.9427337646484375, 179.3997802734375, -3.622931003570556640625),
            Vec3fEq(-175.8854522705078125, 154.7995452880859375, -9.24583911895751953125),
            Vec3fEq(-161.82818603515625, 130.1993255615234375, -14.86874866485595703125),
            Vec3fEq(-147.770904541015625, 105.5991058349609375, -20.4916591644287109375),
            Vec3fEq(-133.7136383056640625, 80.99887847900390625, -26.1145648956298828125),
            Vec3fEq(-119.65636444091796875, 56.39865875244140625, -31.7374725341796875),
            Vec3fEq(-105.59909820556640625, 31.798435211181640625, -26.133396148681640625),
            Vec3fEq(-91.54183197021484375, 7.1982135772705078125, -31.5624217987060546875),
            Vec3fEq(-77.48455810546875, -17.402008056640625, -26.98972320556640625),
            Vec3fEq(-63.427295684814453125, -42.00223541259765625, -19.9045581817626953125),
            Vec3fEq(-42.193531036376953125, -60.761363983154296875, -20.4544773101806640625),
            Vec3fEq(-20.9597682952880859375, -79.5204925537109375, -23.599918365478515625),
            Vec3fEq(3.8312885761260986328125, -93.2384033203125, -30.7141361236572265625),
            Vec3fEq(28.6223468780517578125, -106.95632171630859375, -24.8243885040283203125),
            Vec3fEq(53.413402557373046875, -120.6742401123046875, -31.3303241729736328125),
            Vec3fEq(78.20446014404296875, -134.39215087890625, -25.8431549072265625),
            Vec3fEq(102.99552154541015625, -148.110076904296875, -20.3559894561767578125),
            Vec3fEq(127.7865753173828125, -161.827972412109375, -14.868824005126953125),
            Vec3fEq(152.57763671875, -175.5458984375, -9.3816623687744140625),
            Vec3fEq(177.3686981201171875, -189.2638092041015625, -3.894496917724609375),
            Vec3fEq(202.1597442626953125, -202.9817047119140625, 1.59266507625579833984375),
            Vec3fEq(204, -204, 1.99998295307159423828125)
        )) << mPath;

        compound.shape().updateChildTransform(0, btTransform(btMatrix3x3::getIdentity(), btVector3(1000, 0, 0)));

        mNavigator->updateObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance()), btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mPath.clear();
        mOut = std::back_inserter(mPath);
        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.99998295307159423828125),
            Vec3fEq(-183.965301513671875, 183.965301513671875, 1.99998819828033447265625),
            Vec3fEq(-163.9306182861328125, 163.9306182861328125, 1.99999344348907470703125),
            Vec3fEq(-143.89593505859375, 143.89593505859375, -2.7206256389617919921875),
            Vec3fEq(-123.86124420166015625, 123.86124420166015625, -13.1089839935302734375),
            Vec3fEq(-103.8265533447265625, 103.8265533447265625, -23.4973468780517578125),
            Vec3fEq(-83.7918548583984375, 83.7918548583984375, -33.885707855224609375),
            Vec3fEq(-63.75716400146484375, 63.75716400146484375, -44.27407073974609375),
            Vec3fEq(-43.72247314453125, 43.72247314453125, -54.662433624267578125),
            Vec3fEq(-23.6877803802490234375, 23.6877803802490234375, -65.0507965087890625),
            Vec3fEq(-3.653090000152587890625, 3.653090000152587890625, -75.43915557861328125),
            Vec3fEq(16.3816013336181640625, -16.3816013336181640625, -69.749267578125),
            Vec3fEq(36.416290283203125, -36.416290283203125, -60.4739532470703125),
            Vec3fEq(56.450984954833984375, -56.450984954833984375, -51.1986236572265625),
            Vec3fEq(76.4856719970703125, -76.4856719970703125, -41.92330169677734375),
            Vec3fEq(96.52036285400390625, -96.52036285400390625, -31.46941375732421875),
            Vec3fEq(116.5550537109375, -116.5550537109375, -19.597003936767578125),
            Vec3fEq(136.5897369384765625, -136.5897369384765625, -7.724592685699462890625),
            Vec3fEq(156.6244354248046875, -156.6244354248046875, 1.99999535083770751953125),
            Vec3fEq(176.6591339111328125, -176.6591339111328125, 1.99999010562896728515625),
            Vec3fEq(196.693817138671875, -196.693817138671875, 1.99998486042022705078125),
            Vec3fEq(204, -204, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, for_overlapping_heightfields_should_use_higher)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        const std::array<btScalar, 5 * 5> heightfieldData2 {{
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
        }};
        const auto shapePtr2 = makeSquareHeightfieldTerrainShape(heightfieldData2);
        btHeightfieldTerrainShape& shape2 = *shapePtr2;
        shape2.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->addObject(ObjectId(&shape2), nullptr, shape2, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.999981403350830078125),
            Vec3fEq(-183.965301513671875, 183.965301513671875, -0.428465187549591064453125),
            Vec3fEq(-163.9306182861328125, 163.9306182861328125, -2.8569104671478271484375),
            Vec3fEq(-143.89593505859375, 143.89593505859375, -5.28535556793212890625),
            Vec3fEq(-123.86124420166015625, 123.86124420166015625, -7.7138004302978515625),
            Vec3fEq(-103.8265533447265625, 103.8265533447265625, -10.142246246337890625),
            Vec3fEq(-83.7918548583984375, 83.7918548583984375, -12.3704509735107421875),
            Vec3fEq(-63.75716400146484375, 63.75716400146484375, -14.354084014892578125),
            Vec3fEq(-43.72247314453125, 43.72247314453125, -16.3377170562744140625),
            Vec3fEq(-23.6877803802490234375, 23.6877803802490234375, -18.32135009765625),
            Vec3fEq(-3.653090000152587890625, 3.653090000152587890625, -20.3049831390380859375),
            Vec3fEq(16.3816013336181640625, -16.3816013336181640625, -19.044734954833984375),
            Vec3fEq(36.416290283203125, -36.416290283203125, -17.061100006103515625),
            Vec3fEq(56.450984954833984375, -56.450984954833984375, -15.0774688720703125),
            Vec3fEq(76.4856719970703125, -76.4856719970703125, -13.0938358306884765625),
            Vec3fEq(96.52036285400390625, -96.52036285400390625, -11.02784252166748046875),
            Vec3fEq(116.5550537109375, -116.5550537109375, -8.5993976593017578125),
            Vec3fEq(136.5897369384765625, -136.5897369384765625, -6.170953273773193359375),
            Vec3fEq(156.6244354248046875, -156.6244354248046875, -3.74250507354736328125),
            Vec3fEq(176.6591339111328125, -176.6591339111328125, -1.314060688018798828125),
            Vec3fEq(196.693817138671875, -196.693817138671875, 1.1143856048583984375),
            Vec3fEq(204, -204, 1.9999811649322509765625)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_around_avoid_shape)
    {
        osg::ref_ptr<Resource::BulletShape> bulletShape(new Resource::BulletShape);

        std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        shapePtr->setLocalScaling(btVector3(128, 128, 1));
        bulletShape->mCollisionShape = shapePtr.release();

        std::array<btScalar, 5 * 5> heightfieldDataAvoid {{
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
        }};
        auto shapeAvoidPtr = makeSquareHeightfieldTerrainShape(heightfieldDataAvoid);
        shapeAvoidPtr->setLocalScaling(btVector3(128, 128, 1));
        bulletShape->mAvoidCollisionShape = shapeAvoidPtr.release();

        osg::ref_ptr<const Resource::BulletShapeInstance> instance(new Resource::BulletShapeInstance(bulletShape));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(instance->getCollisionShape()), ObjectShapes(instance), btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.99997997283935546875),
            Vec3fEq(-191.328948974609375, 178.65789794921875, -0.815807759761810302734375),
            Vec3fEq(-178.65789794921875, 153.3157806396484375, -3.6315968036651611328125),
            Vec3fEq(-165.986846923828125, 127.9736785888671875, -6.4473857879638671875),
            Vec3fEq(-153.3157806396484375, 102.6315765380859375, -9.26317310333251953125),
            Vec3fEq(-140.6447296142578125, 77.28946685791015625, -12.07896137237548828125),
            Vec3fEq(-127.9736785888671875, 51.947368621826171875, -14.894748687744140625),
            Vec3fEq(-115.3026275634765625, 26.6052646636962890625, -17.7105388641357421875),
            Vec3fEq(-102.63158416748046875, 1.2631585597991943359375, -20.5263233184814453125),
            Vec3fEq(-89.9605712890625, -24.0789661407470703125, -19.591716766357421875),
            Vec3fEq(-68.54410552978515625, -42.629238128662109375, -19.847625732421875),
            Vec3fEq(-47.127635955810546875, -61.17951202392578125, -20.1035366058349609375),
            Vec3fEq(-25.711170196533203125, -79.72978973388671875, -20.359447479248046875),
            Vec3fEq(-4.294706821441650390625, -98.280059814453125, -20.6153545379638671875),
            Vec3fEq(17.121753692626953125, -116.83034515380859375, -17.3710460662841796875),
            Vec3fEq(42.7990570068359375, -128.80755615234375, -14.7094440460205078125),
            Vec3fEq(68.4763641357421875, -140.7847747802734375, -12.0478420257568359375),
            Vec3fEq(94.15366363525390625, -152.761993408203125, -9.3862361907958984375),
            Vec3fEq(119.83097076416015625, -164.7392120361328125, -6.724635601043701171875),
            Vec3fEq(145.508270263671875, -176.7164306640625, -4.06303119659423828125),
            Vec3fEq(171.185577392578125, -188.69366455078125, -1.40142619609832763671875),
            Vec3fEq(196.862884521484375, -200.6708831787109375, 1.2601754665374755859375),
            Vec3fEq(204, -204, 1.999979496002197265625)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_water_ground_lower_than_water_with_only_swim_flag)
    {
        std::array<btScalar, 5 * 5> heightfieldData {{
            -50,  -50,  -50,  -50,    0,
            -50, -100, -150, -100,  -50,
            -50, -150, -200, -150, -100,
            -50, -100, -150, -100, -100,
              0,  -50, -100, -100, -100,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addWater(osg::Vec2i(0, 0), 128 * 4, 300, btTransform::getIdentity());
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 0;
        mStart.z() = 300;
        mEnd.x() = 0;
        mEnd.z() = 300;

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_swim, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(0, 204, 185.33331298828125),
            Vec3fEq(0, 175.6666717529296875, 185.33331298828125),
            Vec3fEq(0, 147.3333282470703125, 185.33331298828125),
            Vec3fEq(0, 119, 185.33331298828125),
            Vec3fEq(0, 90.6666717529296875, 185.33331298828125),
            Vec3fEq(0, 62.333339691162109375, 185.33331298828125),
            Vec3fEq(0, 34.00000762939453125, 185.33331298828125),
            Vec3fEq(0, 5.66667461395263671875, 185.33331298828125),
            Vec3fEq(0, -22.6666584014892578125, 185.33331298828125),
            Vec3fEq(0, -50.999988555908203125, 185.33331298828125),
            Vec3fEq(0, -79.33332061767578125, 185.33331298828125),
            Vec3fEq(0, -107.666656494140625, 185.33331298828125),
            Vec3fEq(0, -135.9999847412109375, 185.33331298828125),
            Vec3fEq(0, -164.33331298828125, 185.33331298828125),
            Vec3fEq(0, -192.666656494140625, 185.33331298828125),
            Vec3fEq(0, -204, 185.33331298828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_water_when_ground_cross_water_with_swim_and_walk_flags)
    {
        std::array<btScalar, 7 * 7> heightfieldData {{
            0,    0,    0,    0,    0,    0, 0,
            0, -100, -100, -100, -100, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -150, -200, -150, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -100, -100, -100, -100, 0,
            0,    0,    0,    0,    0,    0, 0,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addWater(osg::Vec2i(0, 0), 128 * 4, -25, btTransform::getIdentity());
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 0;
        mEnd.x() = 0;

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_swim | Flag_walk, mAreaCosts, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(0, 204, -98.000030517578125),
            Vec3fEq(0, 175.6666717529296875, -108.30306243896484375),
            Vec3fEq(0, 147.3333282470703125, -118.6060791015625),
            Vec3fEq(0, 119, -128.90911865234375),
            Vec3fEq(0, 90.6666717529296875, -139.2121429443359375),
            Vec3fEq(0, 62.333339691162109375, -143.3333587646484375),
            Vec3fEq(0, 34.00000762939453125, -143.3333587646484375),
            Vec3fEq(0, 5.66667461395263671875, -143.3333587646484375),
            Vec3fEq(0, -22.6666584014892578125, -143.3333587646484375),
            Vec3fEq(0, -50.999988555908203125, -143.3333587646484375),
            Vec3fEq(0, -79.33332061767578125, -143.3333587646484375),
            Vec3fEq(0, -107.666656494140625, -133.0303192138671875),
            Vec3fEq(0, -135.9999847412109375, -122.72728729248046875),
            Vec3fEq(0, -164.33331298828125, -112.4242706298828125),
            Vec3fEq(0, -192.666656494140625, -102.12123870849609375),
            Vec3fEq(0, -204, -98.00002288818359375)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_water_when_ground_cross_water_with_max_int_cells_size_and_swim_and_walk_flags)
    {
        std::array<btScalar, 7 * 7> heightfieldData {{
            0,    0,    0,    0,    0,    0, 0,
            0, -100, -100, -100, -100, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -150, -200, -150, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -100, -100, -100, -100, 0,
            0,    0,    0,    0,    0,    0, 0,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->addWater(osg::Vec2i(0, 0), std::numeric_limits<int>::max(), -25, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 0;
        mEnd.x() = 0;

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_swim | Flag_walk, mAreaCosts, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(0, 204, -98.000030517578125),
            Vec3fEq(0, 175.6666717529296875, -108.30306243896484375),
            Vec3fEq(0, 147.3333282470703125, -118.6060791015625),
            Vec3fEq(0, 119, -128.90911865234375),
            Vec3fEq(0, 90.6666717529296875, -139.2121429443359375),
            Vec3fEq(0, 62.333339691162109375, -143.3333587646484375),
            Vec3fEq(0, 34.00000762939453125, -143.3333587646484375),
            Vec3fEq(0, 5.66667461395263671875, -143.3333587646484375),
            Vec3fEq(0, -22.6666584014892578125, -143.3333587646484375),
            Vec3fEq(0, -50.999988555908203125, -143.3333587646484375),
            Vec3fEq(0, -79.33332061767578125, -143.3333587646484375),
            Vec3fEq(0, -107.666656494140625, -133.0303192138671875),
            Vec3fEq(0, -135.9999847412109375, -122.72728729248046875),
            Vec3fEq(0, -164.33331298828125, -112.4242706298828125),
            Vec3fEq(0, -192.666656494140625, -102.12123870849609375),
            Vec3fEq(0, -204, -98.00002288818359375)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_ground_when_ground_cross_water_with_only_walk_flag)
    {
        std::array<btScalar, 7 * 7> heightfieldData {{
            0,    0,    0,    0,    0,    0, 0,
            0, -100, -100, -100, -100, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -150, -200, -150, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -100, -100, -100, -100, 0,
            0,    0,    0,    0,    0,    0, 0,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addWater(osg::Vec2i(0, 0), 128 * 4, -25, btTransform::getIdentity());
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 0;
        mEnd.x() = 0;

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(0, 204, -98.000030517578125),
            Vec3fEq(10.26930999755859375, 177.59320068359375, -107.4711456298828125),
            Vec3fEq(20.5386199951171875, 151.1864166259765625, -116.9422607421875),
            Vec3fEq(30.8079280853271484375, 124.77960968017578125, -126.41339111328125),
            Vec3fEq(41.077239990234375, 98.37281036376953125, -135.8845062255859375),
            Vec3fEq(51.346546173095703125, 71.96601104736328125, -138.2003936767578125),
            Vec3fEq(61.615856170654296875, 45.559215545654296875, -140.0838470458984375),
            Vec3fEq(71.88516998291015625, 19.1524181365966796875, -141.9673004150390625),
            Vec3fEq(82.15447235107421875, -7.254379749298095703125, -142.3074798583984375),
            Vec3fEq(81.04636383056640625, -35.56603240966796875, -142.7104339599609375),
            Vec3fEq(79.93825531005859375, -63.877685546875, -143.1133880615234375),
            Vec3fEq(78.83014678955078125, -92.18933868408203125, -138.7660675048828125),
            Vec3fEq(62.50392913818359375, -115.3460235595703125, -130.237823486328125),
            Vec3fEq(46.17771148681640625, -138.502716064453125, -121.8172149658203125),
            Vec3fEq(29.85149383544921875, -161.6594085693359375, -113.39659881591796875),
            Vec3fEq(13.52527523040771484375, -184.81610107421875, -104.97599029541015625),
            Vec3fEq(0, -204, -98.00002288818359375)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_remove_and_update_then_find_path_should_return_path)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mNavigator->removeObject(ObjectId(&shape));
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.99998295307159423828125),
            Vec3fEq(-183.965301513671875, 183.965301513671875, 1.99998819828033447265625),
            Vec3fEq(-163.9306182861328125, 163.9306182861328125, 1.99999344348907470703125),
            Vec3fEq(-143.89593505859375, 143.89593505859375, -2.7206256389617919921875),
            Vec3fEq(-123.86124420166015625, 123.86124420166015625, -13.1089839935302734375),
            Vec3fEq(-103.8265533447265625, 103.8265533447265625, -23.4973468780517578125),
            Vec3fEq(-83.7918548583984375, 83.7918548583984375, -33.885707855224609375),
            Vec3fEq(-63.75716400146484375, 63.75716400146484375, -44.27407073974609375),
            Vec3fEq(-43.72247314453125, 43.72247314453125, -54.662433624267578125),
            Vec3fEq(-23.6877803802490234375, 23.6877803802490234375, -65.0507965087890625),
            Vec3fEq(-3.653090000152587890625, 3.653090000152587890625, -75.43915557861328125),
            Vec3fEq(16.3816013336181640625, -16.3816013336181640625, -69.749267578125),
            Vec3fEq(36.416290283203125, -36.416290283203125, -60.4739532470703125),
            Vec3fEq(56.450984954833984375, -56.450984954833984375, -51.1986236572265625),
            Vec3fEq(76.4856719970703125, -76.4856719970703125, -41.92330169677734375),
            Vec3fEq(96.52036285400390625, -96.52036285400390625, -31.46941375732421875),
            Vec3fEq(116.5550537109375, -116.5550537109375, -19.597003936767578125),
            Vec3fEq(136.5897369384765625, -136.5897369384765625, -7.724592685699462890625),
            Vec3fEq(156.6244354248046875, -156.6244354248046875, 1.99999535083770751953125),
            Vec3fEq(176.6591339111328125, -176.6591339111328125, 1.99999010562896728515625),
            Vec3fEq(196.693817138671875, -196.693817138671875, 1.99998486042022705078125),
            Vec3fEq(204, -204, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_then_find_random_point_around_circle_should_return_position)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        Misc::Rng::init(42);

        const auto result = mNavigator->findRandomPointAroundCircle(mAgentHalfExtents, mStart, 100.0, Flag_walk);

        ASSERT_THAT(result, Optional(Vec3fEq(-198.909332275390625, 123.06096649169921875, 1.99998414516448974609375)))
            << (result ? *result : osg::Vec3f());

        const auto distance = (*result - mStart).length();

        EXPECT_FLOAT_EQ(distance, 81.105133056640625) << distance;
    }

    TEST_F(DetourNavigatorNavigatorTest, multiple_threads_should_lock_tiles)
    {
        mSettings.mAsyncNavMeshUpdaterThreads = 2;
        mNavigator.reset(new NavigatorImpl(mSettings));

        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto heightfieldShapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& heightfieldShape = *heightfieldShapePtr;
        heightfieldShape.setLocalScaling(btVector3(128, 128, 1));

        std::vector<CollisionShapeInstance<btBoxShape>> boxes;
        std::generate_n(std::back_inserter(boxes), 100, [] { return std::make_unique<btBoxShape>(btVector3(20, 20, 100)); });

        mNavigator->addAgent(mAgentHalfExtents);

        mNavigator->addObject(ObjectId(&heightfieldShape), nullptr, heightfieldShape, btTransform::getIdentity());

        for (std::size_t i = 0; i < boxes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(i * 10, i * 10, i * 10));
            mNavigator->addObject(ObjectId(&boxes[i].shape()), ObjectShapes(boxes[i].instance()), transform);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));

        for (std::size_t i = 0; i < boxes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(i * 10 + 1, i * 10 + 1, i * 10 + 1));
            mNavigator->updateObject(ObjectId(&boxes[i].shape()), ObjectShapes(boxes[i].instance()), transform);
        }

        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(mNavigator->findPath(mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mOut), Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(-204, 204, 1.99998295307159423828125),
            Vec3fEq(-189.9427337646484375, 179.3997802734375, 1.9999866485595703125),
            Vec3fEq(-175.8854522705078125, 154.7995452880859375, 1.99999034404754638671875),
            Vec3fEq(-161.82818603515625, 130.1993255615234375, -3.701923847198486328125),
            Vec3fEq(-147.770904541015625, 105.5991058349609375, -15.67664432525634765625),
            Vec3fEq(-133.7136383056640625, 80.99887847900390625, -27.6513614654541015625),
            Vec3fEq(-119.65636444091796875, 56.39865875244140625, -20.1209163665771484375),
            Vec3fEq(-105.59909820556640625, 31.798435211181640625, -25.0669879913330078125),
            Vec3fEq(-91.54183197021484375, 7.1982135772705078125, -31.5624217987060546875),
            Vec3fEq(-77.48455810546875, -17.402008056640625, -26.98972320556640625),
            Vec3fEq(-63.427295684814453125, -42.00223541259765625, -19.9045581817626953125),
            Vec3fEq(-42.193531036376953125, -60.761363983154296875, -20.4544773101806640625),
            Vec3fEq(-20.9597682952880859375, -79.5204925537109375, -23.599918365478515625),
            Vec3fEq(3.8312885761260986328125, -93.2384033203125, -30.7141361236572265625),
            Vec3fEq(28.6223468780517578125, -106.95632171630859375, -24.1782474517822265625),
            Vec3fEq(53.413402557373046875, -120.6742401123046875, -19.4096889495849609375),
            Vec3fEq(78.20446014404296875, -134.39215087890625, -27.6632633209228515625),
            Vec3fEq(102.99552154541015625, -148.110076904296875, -15.8613681793212890625),
            Vec3fEq(127.7865753173828125, -161.827972412109375, -4.059485912322998046875),
            Vec3fEq(152.57763671875, -175.5458984375, 1.9999904632568359375),
            Vec3fEq(177.3686981201171875, -189.2638092041015625, 1.9999866485595703125),
            Vec3fEq(202.1597442626953125, -202.9817047119140625, 1.9999830722808837890625),
            Vec3fEq(204, -204, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_changed_multiple_times_object_should_delay_navmesh_change)
    {
        std::vector<CollisionShapeInstance<btBoxShape>> shapes;
        std::generate_n(std::back_inserter(shapes), 100, [] { return std::make_unique<btBoxShape>(btVector3(64, 64, 64)); });

        mNavigator->addAgent(mAgentHalfExtents);

        for (std::size_t i = 0; i < shapes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(i * 32, i * 32, i * 32));
            mNavigator->addObject(ObjectId(&shapes[i].shape()), ObjectShapes(shapes[i].instance()), transform);
        }
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const auto start = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < shapes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(i * 32 + 1, i * 32 + 1, i * 32 + 1));
            mNavigator->updateObject(ObjectId(&shapes[i].shape()), ObjectShapes(shapes[i].instance()), transform);
        }
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        for (std::size_t i = 0; i < shapes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(i * 32 + 2, i * 32 + 2, i * 32 + 2));
            mNavigator->updateObject(ObjectId(&shapes[i].shape()), ObjectShapes(shapes[i].instance()), transform);
        }
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const auto duration = std::chrono::steady_clock::now() - start;

        EXPECT_GT(duration, mSettings.mMinUpdateInterval)
            << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(duration).count() << " ms";
    }

    TEST_F(DetourNavigatorNavigatorTest, update_then_raycast_should_return_position)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& shape = *shapePtr;
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&shape), nullptr, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const auto result = mNavigator->raycast(mAgentHalfExtents, mStart, mEnd, Flag_walk);

        ASSERT_THAT(result, Optional(Vec3fEq(mEnd.x(), mEnd.y(), 1.99998295307159423828125)))
            << (result ? *result : osg::Vec3f());
    }

    TEST_F(DetourNavigatorNavigatorTest, update_for_oscillating_object_that_does_not_change_navmesh_should_not_trigger_navmesh_update)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const auto heightfieldShapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        btHeightfieldTerrainShape& heightfieldShape = *heightfieldShapePtr;
        heightfieldShape.setLocalScaling(btVector3(128, 128, 1));

        CollisionShapeInstance oscillatingBox(std::make_unique<btBoxShape>(btVector3(20, 20, 20)));
        const btVector3 oscillatingBoxShapePosition(32, 32, 400);
        CollisionShapeInstance boderBox(std::make_unique<btBoxShape>(btVector3(50, 50, 50)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&heightfieldShape), nullptr, heightfieldShape, btTransform::getIdentity());
        mNavigator->addObject(ObjectId(&oscillatingBox.shape()), ObjectShapes(oscillatingBox.instance()),
                              btTransform(btMatrix3x3::getIdentity(), oscillatingBoxShapePosition));
        // add this box to make navmesh bound box independent from oscillatingBoxShape rotations
        mNavigator->addObject(ObjectId(&boderBox.shape()), ObjectShapes(boderBox.instance()),
                              btTransform(btMatrix3x3::getIdentity(), oscillatingBoxShapePosition + btVector3(0, 0, 200)));
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const auto navMeshes = mNavigator->getNavMeshes();
        ASSERT_EQ(navMeshes.size(), 1);
        {
            const auto navMesh = navMeshes.begin()->second->lockConst();
            ASSERT_EQ(navMesh->getGeneration(), 1);
            ASSERT_EQ(navMesh->getNavMeshRevision(), 4);
        }

        for (int n = 0; n < 10; ++n)
        {
            const btTransform transform(btQuaternion(btVector3(0, 0, 1), n * 2 * osg::PI / 10),
                                        oscillatingBoxShapePosition);
            mNavigator->updateObject(ObjectId(&oscillatingBox.shape()), ObjectShapes(oscillatingBox.instance()), transform);
            mNavigator->update(mPlayerPosition);
            mNavigator->wait(mListener, WaitConditionType::allJobsDone);
        }

        ASSERT_EQ(navMeshes.size(), 1);
        {
            const auto navMesh = navMeshes.begin()->second->lockConst();
            ASSERT_EQ(navMesh->getGeneration(), 1);
            ASSERT_EQ(navMesh->getNavMeshRevision(), 4);
        }
    }
}
