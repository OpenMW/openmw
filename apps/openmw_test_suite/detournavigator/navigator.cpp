#include "operators.hpp"

#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/exceptions.hpp>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <gtest/gtest.h>

#include <iterator>
#include <deque>

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

        DetourNavigatorNavigatorTest()
            : mPlayerPosition(0, 0, 0)
            , mAgentHalfExtents(29, 29, 66)
            , mStart(-215, 215, 1)
            , mEnd(215, -215, 1)
            , mOut(mPath)
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
            mSettings.mMaxEdgeLen = 12;
            mSettings.mMaxNavMeshQueryNodes = 2048;
            mSettings.mMaxVertsPerPoly = 6;
            mSettings.mRegionMergeSize = 20;
            mSettings.mRegionMinSize = 8;
            mSettings.mTileSize = 64;
            mSettings.mMaxPolygonPathSize = 1024;
            mSettings.mMaxSmoothPathSize = 1024;
            mSettings.mTrianglesPerChunk = 256;
            mNavigator.reset(new Navigator(mSettings));
        }
    };

    TEST_F(DetourNavigatorNavigatorTest, find_path_for_empty_should_throw_exception)
    {
        EXPECT_THROW(mNavigator->findPath(mAgentHalfExtents, mStart, mEnd, mOut), InvalidArgument);
    }

    TEST_F(DetourNavigatorNavigatorTest, find_path_for_existing_agent_with_no_navmesh_should_throw_exception)
    {
        mNavigator->addAgent(mAgentHalfExtents);
        EXPECT_THROW(mNavigator->findPath(mAgentHalfExtents, mStart, mEnd, mOut), NavigatorException);
    }

    TEST_F(DetourNavigatorNavigatorTest, find_path_for_removed_agent_should_throw_exception)
    {
        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->removeAgent(mAgentHalfExtents);
        EXPECT_THROW(mNavigator->findPath(mAgentHalfExtents, mStart, mEnd, mOut), InvalidArgument);
    }

    TEST_F(DetourNavigatorNavigatorTest, add_agent_should_count_each_agent)
    {
        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->removeAgent(mAgentHalfExtents);
        EXPECT_THROW(mNavigator->findPath(mAgentHalfExtents, mStart, mEnd, mOut), NavigatorException);
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
        btHeightfieldTerrainShape shape(5, 5, heightfieldData.data(), 1, 0, 0, 2, PHY_FLOAT, false);
        shape.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(1, shape, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait();

        mNavigator->findPath(mAgentHalfExtents, mStart, mEnd, mOut);

        EXPECT_EQ(mPath, std::deque<osg::Vec3f>({
            osg::Vec3f(-215, 215, 1.85963428020477294921875),
            osg::Vec3f(-194.9653167724609375, 194.9653167724609375, -6.5760211944580078125),
            osg::Vec3f(-174.930633544921875, 174.930633544921875, -15.01167774200439453125),
            osg::Vec3f(-154.8959503173828125, 154.8959503173828125, -23.4473323822021484375),
            osg::Vec3f(-134.86126708984375, 134.86126708984375, -31.8829898834228515625),
            osg::Vec3f(-114.82657623291015625, 114.82657623291015625, -40.3186492919921875),
            osg::Vec3f(-94.7918853759765625, 94.7918853759765625, -47.39907073974609375),
            osg::Vec3f(-74.75719451904296875, 74.75719451904296875, -53.7258148193359375),
            osg::Vec3f(-54.722499847412109375, 54.722499847412109375, -60.052555084228515625),
            osg::Vec3f(-34.68780517578125, 34.68780517578125, -66.37929534912109375),
            osg::Vec3f(-14.6531162261962890625, 14.6531162261962890625, -72.70604705810546875),
            osg::Vec3f(5.3815765380859375, -5.3815765380859375, -75.35065460205078125),
            osg::Vec3f(25.41626739501953125, -25.41626739501953125, -67.96945953369140625),
            osg::Vec3f(45.450958251953125, -45.450958251953125, -60.58824920654296875),
            osg::Vec3f(65.48564910888671875, -65.48564910888671875, -53.20705413818359375),
            osg::Vec3f(85.5203399658203125, -85.5203399658203125, -45.825855255126953125),
            osg::Vec3f(105.55503082275390625, -105.55503082275390625, -38.44464874267578125),
            osg::Vec3f(125.5897216796875, -125.5897216796875, -31.063449859619140625),
            osg::Vec3f(145.6244049072265625, -145.6244049072265625, -23.6822509765625),
            osg::Vec3f(165.659088134765625, -165.659088134765625, -16.3010540008544921875),
            osg::Vec3f(185.6937713623046875, -185.6937713623046875, -8.91985416412353515625),
            osg::Vec3f(205.7284698486328125, -205.7284698486328125, -1.53864824771881103515625),
            osg::Vec3f(215, -215, 1.877177715301513671875),
        })) << mPath;
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
        btHeightfieldTerrainShape shape(5, 5, heightfieldData.data(), 1, 0, 0, 2, PHY_FLOAT, false);
        shape.setLocalScaling(btVector3(128, 128, 1));

        const std::array<btScalar, 5 * 5> heightfieldData2 {{
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
        }};
        btHeightfieldTerrainShape shape2(5, 5, heightfieldData2.data(), 1, 0, 0, 2, PHY_FLOAT, false);
        shape2.setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(1, shape, btTransform::getIdentity());
        mNavigator->addObject(2, shape2, btTransform::getIdentity());
        mNavigator->update(mPlayerPosition);
        mNavigator->wait();

        mNavigator->findPath(mAgentHalfExtents, mStart, mEnd, mOut);

        EXPECT_EQ(mPath, std::deque<osg::Vec3f>({
            osg::Vec3f(-215, 215, 1.96328866481781005859375),
            osg::Vec3f(-194.9653167724609375, 194.9653167724609375, -0.2422157227993011474609375),
            osg::Vec3f(-174.930633544921875, 174.930633544921875, -2.44772052764892578125),
            osg::Vec3f(-154.8959503173828125, 154.8959503173828125, -4.653223514556884765625),
            osg::Vec3f(-134.86126708984375, 134.86126708984375, -6.858728885650634765625),
            osg::Vec3f(-114.82657623291015625, 114.82657623291015625, -9.0642337799072265625),
            osg::Vec3f(-94.7918853759765625, 94.7918853759765625, -11.26973724365234375),
            osg::Vec3f(-74.75719451904296875, 74.75719451904296875, -13.26497173309326171875),
            osg::Vec3f(-54.722499847412109375, 54.722499847412109375, -15.24860286712646484375),
            osg::Vec3f(-34.68780517578125, 34.68780517578125, -17.2322368621826171875),
            osg::Vec3f(-14.6531162261962890625, 14.6531162261962890625, -19.2158660888671875),
            osg::Vec3f(5.3815765380859375, -5.3815765380859375, -20.1338443756103515625),
            osg::Vec3f(25.41626739501953125, -25.41626739501953125, -18.150211334228515625),
            osg::Vec3f(45.450958251953125, -45.450958251953125, -16.1665802001953125),
            osg::Vec3f(65.48564910888671875, -65.48564910888671875, -14.18294811248779296875),
            osg::Vec3f(85.5203399658203125, -85.5203399658203125, -12.19931507110595703125),
            osg::Vec3f(105.55503082275390625, -105.55503082275390625, -10.08488559722900390625),
            osg::Vec3f(125.5897216796875, -125.5897216796875, -7.879383563995361328125),
            osg::Vec3f(145.6244049072265625, -145.6244049072265625, -5.673877239227294921875),
            osg::Vec3f(165.659088134765625, -165.659088134765625, -3.4683735370635986328125),
            osg::Vec3f(185.6937713623046875, -185.6937713623046875, -1.2628715038299560546875),
            osg::Vec3f(205.7284698486328125, -205.7284698486328125, 0.9426348209381103515625),
            osg::Vec3f(215, -215, 1.96328866481781005859375),
        })) << mPath;
    }
}
