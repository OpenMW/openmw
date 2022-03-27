#include "operators.hpp"
#include "settings.hpp"

#include <components/detournavigator/navigatorimpl.hpp>
#include <components/detournavigator/exceptions.hpp>
#include <components/detournavigator/navigatorutils.hpp>
#include <components/detournavigator/navmeshdb.hpp>
#include <components/misc/rng.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/esm3/loadland.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/bullethelpers/heightfield.hpp>

#include <osg/ref_ptr>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <deque>
#include <memory>
#include <limits>

MATCHER_P3(Vec3fEq, x, y, z, "")
{
    return std::abs(arg.x() - x) < 1e-4 && std::abs(arg.y() - y) < 1e-4 && std::abs(arg.z() - z) < 1e-4;
}

namespace
{
    using namespace testing;
    using namespace DetourNavigator;
    using namespace DetourNavigator::Tests;

    struct DetourNavigatorNavigatorTest : Test
    {
        Settings mSettings = makeSettings();
        std::unique_ptr<Navigator> mNavigator;
        const osg::Vec3f mPlayerPosition;
        const std::string mWorldspace;
        const osg::Vec3f mAgentHalfExtents;
        osg::Vec3f mStart;
        osg::Vec3f mEnd;
        std::deque<osg::Vec3f> mPath;
        std::back_insert_iterator<std::deque<osg::Vec3f>> mOut;
        float mStepSize;
        AreaCosts mAreaCosts;
        Loading::Listener mListener;
        const osg::Vec2i mCellPosition {0, 0};
        const int mHeightfieldTileSize = ESM::Land::REAL_SIZE / (ESM::Land::LAND_SIZE - 1);
        const float mEndTolerance = 0;
        const btTransform mTransform {btMatrix3x3::getIdentity(), btVector3(256, 256, 0)};
        const ObjectTransform mObjectTransform {ESM::Position {{256, 256, 0}, {0, 0, 0}}, 0.0f};

        DetourNavigatorNavigatorTest()
            : mPlayerPosition(256, 256, 0)
            , mWorldspace("sys::default")
            , mAgentHalfExtents(29, 29, 66)
            , mStart(52, 460, 1)
            , mEnd(460, 52, 1)
            , mOut(mPath)
            , mStepSize(28.333332061767578125f)
        {
            mNavigator.reset(new NavigatorImpl(mSettings, std::make_unique<NavMeshDb>(":memory:", std::numeric_limits<std::uint64_t>::max())));
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

    template <std::size_t size>
    HeightfieldSurface makeSquareHeightfieldSurface(const std::array<float, size>& values)
    {
        const auto [min, max] = std::minmax_element(values.begin(), values.end());
        const float greater = std::max(std::abs(*min), std::abs(*max));
        HeightfieldSurface surface;
        surface.mHeights = values.data();
        surface.mMinHeight = -greater;
        surface.mMaxHeight = greater;
        surface.mSize = static_cast<int>(std::sqrt(size));
        return surface;
    }

    template <class T>
    osg::ref_ptr<const Resource::BulletShapeInstance> makeBulletShapeInstance(std::unique_ptr<T>&& shape)
    {
        osg::ref_ptr<Resource::BulletShape> bulletShape(new Resource::BulletShape);
        bulletShape->mCollisionShape.reset(std::move(shape).release());
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

    btVector3 getHeightfieldShift(const osg::Vec2i& cellPosition, int cellSize, float minHeight, float maxHeight)
    {
        return BulletHelpers::getHeightfieldShift(cellPosition.x(), cellPosition.x(), cellSize, minHeight, maxHeight);
    }

    TEST_F(DetourNavigatorNavigatorTest, find_path_for_empty_should_return_empty)
    {
        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::NavMeshNotFound);
        EXPECT_EQ(mPath, std::deque<osg::Vec3f>());
    }

    TEST_F(DetourNavigatorNavigatorTest, find_path_for_existing_agent_with_no_navmesh_should_throw_exception)
    {
        mNavigator->addAgent(mAgentHalfExtents);
        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::StartPolygonNotFound);
    }

    TEST_F(DetourNavigatorNavigatorTest, add_agent_should_count_each_agent)
    {
        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->removeAgent(mAgentHalfExtents);
        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::StartPolygonNotFound);
    }

    TEST_F(DetourNavigatorNavigatorTest, update_then_find_path_should_return_path)
    {
        constexpr std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::requiredTilesPresent);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(76.70135498046875, 439.965301513671875, -0.9659786224365234375),
            Vec3fEq(96.73604583740234375, 419.93060302734375, -4.002437114715576171875),
            Vec3fEq(116.770751953125, 399.89593505859375, -7.0388965606689453125),
            Vec3fEq(136.8054351806640625, 379.861236572265625, -11.5593852996826171875),
            Vec3fEq(156.840118408203125, 359.826568603515625, -20.7333812713623046875),
            Vec3fEq(176.8748016357421875, 339.7918701171875, -34.014251708984375),
            Vec3fEq(196.90948486328125, 319.757171630859375, -47.2951202392578125),
            Vec3fEq(216.944183349609375, 299.722503662109375, -59.4111785888671875),
            Vec3fEq(236.9788665771484375, 279.68780517578125, -65.76436614990234375),
            Vec3fEq(257.0135498046875, 259.65313720703125, -68.12311553955078125),
            Vec3fEq(277.048248291015625, 239.618438720703125, -66.5666656494140625),
            Vec3fEq(297.082916259765625, 219.583740234375, -60.305889129638671875),
            Vec3fEq(317.11761474609375, 199.549041748046875, -49.181324005126953125),
            Vec3fEq(337.15228271484375, 179.5143585205078125, -35.742702484130859375),
            Vec3fEq(357.186981201171875, 159.47967529296875, -22.304073333740234375),
            Vec3fEq(377.221649169921875, 139.4449920654296875, -12.65070629119873046875),
            Vec3fEq(397.25634765625, 119.41030120849609375, -7.41098117828369140625),
            Vec3fEq(417.291046142578125, 99.3756103515625, -4.382833957672119140625),
            Vec3fEq(437.325714111328125, 79.340911865234375, -1.354687213897705078125),
            Vec3fEq(457.360443115234375, 59.3062286376953125, 1.624610424041748046875),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, add_object_should_change_navmesh)
    {
        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        CollisionShapeInstance compound(std::make_unique<btCompoundShape>());
        compound.shape().addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(0, 0, 0)), new btBoxShape(btVector3(20, 20, 100)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(76.70135498046875, 439.965301513671875, -0.9659786224365234375),
            Vec3fEq(96.73604583740234375, 419.93060302734375, -4.002437114715576171875),
            Vec3fEq(116.770751953125, 399.89593505859375, -7.0388965606689453125),
            Vec3fEq(136.8054351806640625, 379.861236572265625, -11.5593852996826171875),
            Vec3fEq(156.840118408203125, 359.826568603515625, -20.7333812713623046875),
            Vec3fEq(176.8748016357421875, 339.7918701171875, -34.014251708984375),
            Vec3fEq(196.90948486328125, 319.757171630859375, -47.2951202392578125),
            Vec3fEq(216.944183349609375, 299.722503662109375, -59.4111785888671875),
            Vec3fEq(236.9788665771484375, 279.68780517578125, -65.76436614990234375),
            Vec3fEq(257.0135498046875, 259.65313720703125, -68.12311553955078125),
            Vec3fEq(277.048248291015625, 239.618438720703125, -66.5666656494140625),
            Vec3fEq(297.082916259765625, 219.583740234375, -60.305889129638671875),
            Vec3fEq(317.11761474609375, 199.549041748046875, -49.181324005126953125),
            Vec3fEq(337.15228271484375, 179.5143585205078125, -35.742702484130859375),
            Vec3fEq(357.186981201171875, 159.47967529296875, -22.304073333740234375),
            Vec3fEq(377.221649169921875, 139.4449920654296875, -12.65070629119873046875),
            Vec3fEq(397.25634765625, 119.41030120849609375, -7.41098117828369140625),
            Vec3fEq(417.291046142578125, 99.3756103515625, -4.382833957672119140625),
            Vec3fEq(437.325714111328125, 79.340911865234375, -1.354687213897705078125),
            Vec3fEq(457.360443115234375, 59.3062286376953125, 1.624610424041748046875),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;

        mNavigator->addObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mPath.clear();
        mOut = std::back_inserter(mPath);
        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(69.5299530029296875, 434.754913330078125, -2.6775772571563720703125),
            Vec3fEq(82.39324951171875, 409.50982666015625, -7.355137348175048828125),
            Vec3fEq(95.25653839111328125, 384.2647705078125, -12.0326976776123046875),
            Vec3fEq(108.11983489990234375, 359.019683837890625, -16.71025848388671875),
            Vec3fEq(120.983123779296875, 333.774627685546875, -21.3878192901611328125),
            Vec3fEq(133.8464202880859375, 308.529541015625, -26.0653781890869140625),
            Vec3fEq(146.7097015380859375, 283.284454345703125, -30.7429370880126953125),
            Vec3fEq(159.572998046875, 258.039398193359375, -35.420497894287109375),
            Vec3fEq(172.4362945556640625, 232.7943115234375, -27.2731761932373046875),
            Vec3fEq(185.2996063232421875, 207.54925537109375, -19.575878143310546875),
            Vec3fEq(206.6449737548828125, 188.917236328125, -20.3511219024658203125),
            Vec3fEq(227.9903564453125, 170.28521728515625, -22.9776935577392578125),
            Vec3fEq(253.4362640380859375, 157.8239593505859375, -31.1692962646484375),
            Vec3fEq(278.8822021484375, 145.3627166748046875, -30.253124237060546875),
            Vec3fEq(304.328094482421875, 132.9014739990234375, -22.219127655029296875),
            Vec3fEq(329.774017333984375, 120.44022369384765625, -13.2701435089111328125),
            Vec3fEq(355.219940185546875, 107.97898101806640625, -5.330339908599853515625),
            Vec3fEq(380.665863037109375, 95.51773834228515625, -3.5501649379730224609375),
            Vec3fEq(406.111785888671875, 83.05649566650390625, -1.76998889446258544921875),
            Vec3fEq(431.557708740234375, 70.5952606201171875, 0.01018683053553104400634765625),
            Vec3fEq(457.003662109375, 58.134021759033203125, 1.79036080837249755859375),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_changed_object_should_change_navmesh)
    {
        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        CollisionShapeInstance compound(std::make_unique<btCompoundShape>());
        compound.shape().addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(0, 0, 0)), new btBoxShape(btVector3(20, 20, 100)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->addObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(69.5299530029296875, 434.754913330078125, -2.6775772571563720703125),
            Vec3fEq(82.39324951171875, 409.50982666015625, -7.355137348175048828125),
            Vec3fEq(95.25653839111328125, 384.2647705078125, -12.0326976776123046875),
            Vec3fEq(108.11983489990234375, 359.019683837890625, -16.71025848388671875),
            Vec3fEq(120.983123779296875, 333.774627685546875, -21.3878192901611328125),
            Vec3fEq(133.8464202880859375, 308.529541015625, -26.0653781890869140625),
            Vec3fEq(146.7097015380859375, 283.284454345703125, -30.7429370880126953125),
            Vec3fEq(159.572998046875, 258.039398193359375, -35.420497894287109375),
            Vec3fEq(172.4362945556640625, 232.7943115234375, -27.2731761932373046875),
            Vec3fEq(185.2996063232421875, 207.54925537109375, -19.575878143310546875),
            Vec3fEq(206.6449737548828125, 188.917236328125, -20.3511219024658203125),
            Vec3fEq(227.9903564453125, 170.28521728515625, -22.9776935577392578125),
            Vec3fEq(253.4362640380859375, 157.8239593505859375, -31.1692962646484375),
            Vec3fEq(278.8822021484375, 145.3627166748046875, -30.253124237060546875),
            Vec3fEq(304.328094482421875, 132.9014739990234375, -22.219127655029296875),
            Vec3fEq(329.774017333984375, 120.44022369384765625, -13.2701435089111328125),
            Vec3fEq(355.219940185546875, 107.97898101806640625, -5.330339908599853515625),
            Vec3fEq(380.665863037109375, 95.51773834228515625, -3.5501649379730224609375),
            Vec3fEq(406.111785888671875, 83.05649566650390625, -1.76998889446258544921875),
            Vec3fEq(431.557708740234375, 70.5952606201171875, 0.01018683053553104400634765625),
            Vec3fEq(457.003662109375, 58.134021759033203125, 1.79036080837249755859375),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;

        compound.shape().updateChildTransform(0, btTransform(btMatrix3x3::getIdentity(), btVector3(1000, 0, 0)));

        mNavigator->updateObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mPath.clear();
        mOut = std::back_inserter(mPath);
        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(76.70135498046875, 439.965301513671875, -0.9659786224365234375),
            Vec3fEq(96.73604583740234375, 419.93060302734375, -4.002437114715576171875),
            Vec3fEq(116.770751953125, 399.89593505859375, -7.0388965606689453125),
            Vec3fEq(136.8054351806640625, 379.861236572265625, -11.5593852996826171875),
            Vec3fEq(156.840118408203125, 359.826568603515625, -20.7333812713623046875),
            Vec3fEq(176.8748016357421875, 339.7918701171875, -34.014251708984375),
            Vec3fEq(196.90948486328125, 319.757171630859375, -47.2951202392578125),
            Vec3fEq(216.944183349609375, 299.722503662109375, -59.4111785888671875),
            Vec3fEq(236.9788665771484375, 279.68780517578125, -65.76436614990234375),
            Vec3fEq(257.0135498046875, 259.65313720703125, -68.12311553955078125),
            Vec3fEq(277.048248291015625, 239.618438720703125, -66.5666656494140625),
            Vec3fEq(297.082916259765625, 219.583740234375, -60.305889129638671875),
            Vec3fEq(317.11761474609375, 199.549041748046875, -49.181324005126953125),
            Vec3fEq(337.15228271484375, 179.5143585205078125, -35.742702484130859375),
            Vec3fEq(357.186981201171875, 159.47967529296875, -22.304073333740234375),
            Vec3fEq(377.221649169921875, 139.4449920654296875, -12.65070629119873046875),
            Vec3fEq(397.25634765625, 119.41030120849609375, -7.41098117828369140625),
            Vec3fEq(417.291046142578125, 99.3756103515625, -4.382833957672119140625),
            Vec3fEq(437.325714111328125, 79.340911865234375, -1.354687213897705078125),
            Vec3fEq(457.360443115234375, 59.3062286376953125, 1.624610424041748046875),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, for_overlapping_heightfields_objects_should_use_higher)
    {
        const std::array<btScalar, 5 * 5> heightfieldData1 {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        CollisionShapeInstance heightfield1(makeSquareHeightfieldTerrainShape(heightfieldData1));
        heightfield1.shape().setLocalScaling(btVector3(128, 128, 1));

        const std::array<btScalar, 5 * 5> heightfieldData2 {{
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
        }};
        CollisionShapeInstance heightfield2(makeSquareHeightfieldTerrainShape(heightfieldData2));
        heightfield2.shape().setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&heightfield1.shape()), ObjectShapes(heightfield1.instance(), mObjectTransform), mTransform);
        mNavigator->addObject(ObjectId(&heightfield2.shape()), ObjectShapes(heightfield2.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(76.70135498046875, 439.965301513671875, -0.903246104717254638671875),
            Vec3fEq(96.73604583740234375, 419.93060302734375, -3.8064472675323486328125),
            Vec3fEq(116.770751953125, 399.89593505859375, -6.709649562835693359375),
            Vec3fEq(136.8054351806640625, 379.861236572265625, -9.33333873748779296875),
            Vec3fEq(156.840118408203125, 359.826568603515625, -9.33333873748779296875),
            Vec3fEq(176.8748016357421875, 339.7918701171875, -9.33333873748779296875),
            Vec3fEq(196.90948486328125, 319.757171630859375, -9.33333873748779296875),
            Vec3fEq(216.944183349609375, 299.722503662109375, -9.33333873748779296875),
            Vec3fEq(236.9788665771484375, 279.68780517578125, -9.33333873748779296875),
            Vec3fEq(257.0135498046875, 259.65313720703125, -9.33333873748779296875),
            Vec3fEq(277.048248291015625, 239.618438720703125, -9.33333873748779296875),
            Vec3fEq(297.082916259765625, 219.583740234375, -9.33333873748779296875),
            Vec3fEq(317.11761474609375, 199.549041748046875, -9.33333873748779296875),
            Vec3fEq(337.15228271484375, 179.5143585205078125, -9.33333873748779296875),
            Vec3fEq(357.186981201171875, 159.47967529296875, -9.33333873748779296875),
            Vec3fEq(377.221649169921875, 139.4449920654296875, -9.33333873748779296875),
            Vec3fEq(397.25634765625, 119.41030120849609375, -6.891522884368896484375),
            Vec3fEq(417.291046142578125, 99.3756103515625, -4.053897380828857421875),
            Vec3fEq(437.325714111328125, 79.340911865234375, -1.21627247333526611328125),
            Vec3fEq(457.360443115234375, 59.3062286376953125, 1.621352672576904296875),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, only_one_heightfield_per_cell_is_allowed)
    {
        const std::array<float, 5 * 5> heightfieldData1 {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface1 = makeSquareHeightfieldSurface(heightfieldData1);
        const int cellSize1 = mHeightfieldTileSize * (surface1.mSize - 1);

        const std::array<float, 5 * 5> heightfieldData2 {{
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
        }};
        const HeightfieldSurface surface2 = makeSquareHeightfieldSurface(heightfieldData2);
        const int cellSize2 = mHeightfieldTileSize * (surface2.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        EXPECT_TRUE(mNavigator->addHeightfield(mCellPosition, cellSize1, surface1));
        EXPECT_FALSE(mNavigator->addHeightfield(mCellPosition, cellSize2, surface2));
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
        std::unique_ptr<btHeightfieldTerrainShape> shapePtr = makeSquareHeightfieldTerrainShape(heightfieldData);
        shapePtr->setLocalScaling(btVector3(128, 128, 1));
        bulletShape->mCollisionShape.reset(shapePtr.release());

        std::array<btScalar, 5 * 5> heightfieldDataAvoid {{
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
            -25, -25, -25, -25, -25,
        }};
        std::unique_ptr<btHeightfieldTerrainShape> shapeAvoidPtr = makeSquareHeightfieldTerrainShape(heightfieldDataAvoid);
        shapeAvoidPtr->setLocalScaling(btVector3(128, 128, 1));
        bulletShape->mAvoidCollisionShape.reset(shapeAvoidPtr.release());

        osg::ref_ptr<const Resource::BulletShapeInstance> instance(new Resource::BulletShapeInstance(bulletShape));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(instance->mCollisionShape.get()), ObjectShapes(instance, mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(69.013885498046875, 434.49853515625, -0.74384129047393798828125),
            Vec3fEq(81.36110687255859375, 408.997100830078125, -3.4876689910888671875),
            Vec3fEq(93.7083282470703125, 383.495635986328125, -6.2314929962158203125),
            Vec3fEq(106.0555419921875, 357.99420166015625, -8.97531890869140625),
            Vec3fEq(118.40276336669921875, 332.49273681640625, -11.7191448211669921875),
            Vec3fEq(130.7499847412109375, 306.991302490234375, -14.4629726409912109375),
            Vec3fEq(143.0972137451171875, 281.4898681640625, -17.206798553466796875),
            Vec3fEq(155.4444122314453125, 255.9884033203125, -19.9506206512451171875),
            Vec3fEq(167.7916412353515625, 230.4869537353515625, -19.91887664794921875),
            Vec3fEq(189.053619384765625, 211.75982666015625, -20.1138629913330078125),
            Vec3fEq(210.3155975341796875, 193.032684326171875, -20.3088512420654296875),
            Vec3fEq(231.577606201171875, 174.3055419921875, -20.503841400146484375),
            Vec3fEq(252.839599609375, 155.5784149169921875, -19.9803981781005859375),
            Vec3fEq(278.407989501953125, 143.3704071044921875, -17.2675113677978515625),
            Vec3fEq(303.976348876953125, 131.16241455078125, -14.55462360382080078125),
            Vec3fEq(329.54473876953125, 118.9544219970703125, -11.84173583984375),
            Vec3fEq(355.11309814453125, 106.74642181396484375, -9.12884807586669921875),
            Vec3fEq(380.681488037109375, 94.538421630859375, -6.4159603118896484375),
            Vec3fEq(406.249847412109375, 82.33042144775390625, -3.7030735015869140625),
            Vec3fEq(431.8182373046875, 70.1224365234375, -0.990187108516693115234375),
            Vec3fEq(457.38665771484375, 57.9144439697265625, 1.72269880771636962890625),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_water_ground_lower_than_water_with_only_swim_flag)
    {
        std::array<float, 5 * 5> heightfieldData {{
            -50,  -50,  -50,  -50,    0,
            -50, -100, -150, -100,  -50,
            -50, -150, -200, -150, -100,
            -50, -100, -150, -100, -100,
              0,  -50, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addWater(mCellPosition, cellSize, 300);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 256;
        mStart.z() = 300;
        mEnd.x() = 256;
        mEnd.z() = 300;

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_swim, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(256, 460, 185.33331298828125),
            Vec3fEq(256, 431.666656494140625, 185.33331298828125),
            Vec3fEq(256, 403.33331298828125, 185.33331298828125),
            Vec3fEq(256, 375, 185.33331298828125),
            Vec3fEq(256, 346.666656494140625, 185.33331298828125),
            Vec3fEq(256, 318.33331298828125, 185.33331298828125),
            Vec3fEq(256, 290, 185.33331298828125),
            Vec3fEq(256, 261.666656494140625, 185.33331298828125),
            Vec3fEq(256, 233.3333282470703125, 185.33331298828125),
            Vec3fEq(256, 205, 185.33331298828125),
            Vec3fEq(256, 176.6666717529296875, 185.33331298828125),
            Vec3fEq(256, 148.3333282470703125, 185.33331298828125),
            Vec3fEq(256, 120, 185.33331298828125),
            Vec3fEq(256, 91.6666717529296875, 185.33331298828125),
            Vec3fEq(255.999969482421875, 63.33333587646484375, 185.33331298828125),
            Vec3fEq(255.999969482421875, 56.66666412353515625, 185.33331298828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_water_when_ground_cross_water_with_swim_and_walk_flags)
    {
        std::array<float, 7 * 7> heightfieldData {{
            0,    0,    0,    0,    0,    0, 0,
            0, -100, -100, -100, -100, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -150, -200, -150, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -100, -100, -100, -100, 0,
            0,    0,    0,    0,    0,    0, 0,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addWater(mCellPosition, cellSize, -25);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 256;
        mEnd.x() = 256;

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_swim | Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(256, 460, -129.4098663330078125),
            Vec3fEq(256, 431.666656494140625, -129.6970062255859375),
            Vec3fEq(256, 403.33331298828125, -129.6970062255859375),
            Vec3fEq(256, 375, -129.4439239501953125),
            Vec3fEq(256, 346.666656494140625, -129.02587890625),
            Vec3fEq(256, 318.33331298828125, -128.6078338623046875),
            Vec3fEq(256, 290, -128.1021728515625),
            Vec3fEq(256, 261.666656494140625, -126.46875),
            Vec3fEq(256, 233.3333282470703125, -119.4891357421875),
            Vec3fEq(256, 205, -110.62021636962890625),
            Vec3fEq(256, 176.6666717529296875, -101.7512969970703125),
            Vec3fEq(256, 148.3333282470703125, -92.88237762451171875),
            Vec3fEq(256, 120, -75.29378509521484375),
            Vec3fEq(256, 91.6666717529296875, -55.201839447021484375),
            Vec3fEq(256.000030517578125, 63.33333587646484375, -34.800380706787109375),
            Vec3fEq(256.000030517578125, 56.66666412353515625, -30.00003814697265625)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_water_when_ground_cross_water_with_max_int_cells_size_and_swim_and_walk_flags)
    {
        std::array<float, 7 * 7> heightfieldData {{
            0,    0,    0,    0,    0,    0, 0,
            0, -100, -100, -100, -100, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -150, -200, -150, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -100, -100, -100, -100, 0,
            0,    0,    0,    0,    0,    0, 0,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->addWater(mCellPosition, std::numeric_limits<int>::max(), -25);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 256;
        mEnd.x() = 256;

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_swim | Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(256, 460, -129.4098663330078125),
            Vec3fEq(256, 431.666656494140625, -129.6970062255859375),
            Vec3fEq(256, 403.33331298828125, -129.6970062255859375),
            Vec3fEq(256, 375, -129.4439239501953125),
            Vec3fEq(256, 346.666656494140625, -129.02587890625),
            Vec3fEq(256, 318.33331298828125, -128.6078338623046875),
            Vec3fEq(256, 290, -128.1021728515625),
            Vec3fEq(256, 261.666656494140625, -126.46875),
            Vec3fEq(256, 233.3333282470703125, -119.4891357421875),
            Vec3fEq(256, 205, -110.62021636962890625),
            Vec3fEq(256, 176.6666717529296875, -101.7512969970703125),
            Vec3fEq(256, 148.3333282470703125, -92.88237762451171875),
            Vec3fEq(256, 120, -75.29378509521484375),
            Vec3fEq(256, 91.6666717529296875, -55.201839447021484375),
            Vec3fEq(256.000030517578125, 63.33333587646484375, -34.800380706787109375),
            Vec3fEq(256.000030517578125, 56.66666412353515625, -30.00003814697265625)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, path_should_be_over_ground_when_ground_cross_water_with_only_walk_flag)
    {
        std::array<float, 7 * 7> heightfieldData {{
            0,    0,    0,    0,    0,    0, 0,
            0, -100, -100, -100, -100, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -150, -200, -150, -100, 0,
            0, -100, -150, -150, -150, -100, 0,
            0, -100, -100, -100, -100, -100, 0,
            0,    0,    0,    0,    0,    0, 0,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addWater(mCellPosition, cellSize, -25);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mStart.x() = 256;
        mEnd.x() = 256;

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(256, 460, -129.4098663330078125),
            Vec3fEq(256, 431.666656494140625, -129.6970062255859375),
            Vec3fEq(256, 403.33331298828125, -129.6970062255859375),
            Vec3fEq(256, 375, -129.4439239501953125),
            Vec3fEq(256, 346.666656494140625, -129.02587890625),
            Vec3fEq(256, 318.33331298828125, -128.6078338623046875),
            Vec3fEq(256, 290, -128.1021728515625),
            Vec3fEq(256, 261.666656494140625, -126.46875),
            Vec3fEq(256, 233.3333282470703125, -119.4891357421875),
            Vec3fEq(256, 205, -110.62021636962890625),
            Vec3fEq(256, 176.6666717529296875, -101.7512969970703125),
            Vec3fEq(256, 148.3333282470703125, -92.88237762451171875),
            Vec3fEq(256, 120, -75.29378509521484375),
            Vec3fEq(256, 91.6666717529296875, -55.201839447021484375),
            Vec3fEq(256.000030517578125, 63.33333587646484375, -34.800380706787109375),
            Vec3fEq(256.000030517578125, 56.66666412353515625, -30.00003814697265625)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_object_remove_and_update_then_find_path_should_return_path)
    {
        const std::array<btScalar, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        CollisionShapeInstance heightfield(makeSquareHeightfieldTerrainShape(heightfieldData));
        heightfield.shape().setLocalScaling(btVector3(128, 128, 1));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addObject(ObjectId(&heightfield.shape()), ObjectShapes(heightfield.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mNavigator->removeObject(ObjectId(&heightfield.shape()));
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mNavigator->addObject(ObjectId(&heightfield.shape()), ObjectShapes(heightfield.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(76.70135498046875, 439.965301513671875, -0.9659786224365234375),
            Vec3fEq(96.73604583740234375, 419.93060302734375, -4.002437114715576171875),
            Vec3fEq(116.770751953125, 399.89593505859375, -7.0388965606689453125),
            Vec3fEq(136.8054351806640625, 379.861236572265625, -11.5593852996826171875),
            Vec3fEq(156.840118408203125, 359.826568603515625, -20.7333812713623046875),
            Vec3fEq(176.8748016357421875, 339.7918701171875, -34.014251708984375),
            Vec3fEq(196.90948486328125, 319.757171630859375, -47.2951202392578125),
            Vec3fEq(216.944183349609375, 299.722503662109375, -59.4111785888671875),
            Vec3fEq(236.9788665771484375, 279.68780517578125, -65.76436614990234375),
            Vec3fEq(257.0135498046875, 259.65313720703125, -68.12311553955078125),
            Vec3fEq(277.048248291015625, 239.618438720703125, -66.5666656494140625),
            Vec3fEq(297.082916259765625, 219.583740234375, -60.305889129638671875),
            Vec3fEq(317.11761474609375, 199.549041748046875, -49.181324005126953125),
            Vec3fEq(337.15228271484375, 179.5143585205078125, -35.742702484130859375),
            Vec3fEq(357.186981201171875, 159.47967529296875, -22.304073333740234375),
            Vec3fEq(377.221649169921875, 139.4449920654296875, -12.65070629119873046875),
            Vec3fEq(397.25634765625, 119.41030120849609375, -7.41098117828369140625),
            Vec3fEq(417.291046142578125, 99.3756103515625, -4.382833957672119140625),
            Vec3fEq(437.325714111328125, 79.340911865234375, -1.354687213897705078125),
            Vec3fEq(457.360443115234375, 59.3062286376953125, 1.624610424041748046875),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_heightfield_remove_and_update_then_find_path_should_return_path)
    {
        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mNavigator->removeHeightfield(mCellPosition);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(76.70135498046875, 439.965301513671875, -0.9659786224365234375),
            Vec3fEq(96.73604583740234375, 419.93060302734375, -4.002437114715576171875),
            Vec3fEq(116.770751953125, 399.89593505859375, -7.0388965606689453125),
            Vec3fEq(136.8054351806640625, 379.861236572265625, -11.5593852996826171875),
            Vec3fEq(156.840118408203125, 359.826568603515625, -20.7333812713623046875),
            Vec3fEq(176.8748016357421875, 339.7918701171875, -34.014251708984375),
            Vec3fEq(196.90948486328125, 319.757171630859375, -47.2951202392578125),
            Vec3fEq(216.944183349609375, 299.722503662109375, -59.4111785888671875),
            Vec3fEq(236.9788665771484375, 279.68780517578125, -65.76436614990234375),
            Vec3fEq(257.0135498046875, 259.65313720703125, -68.12311553955078125),
            Vec3fEq(277.048248291015625, 239.618438720703125, -66.5666656494140625),
            Vec3fEq(297.082916259765625, 219.583740234375, -60.305889129638671875),
            Vec3fEq(317.11761474609375, 199.549041748046875, -49.181324005126953125),
            Vec3fEq(337.15228271484375, 179.5143585205078125, -35.742702484130859375),
            Vec3fEq(357.186981201171875, 159.47967529296875, -22.304073333740234375),
            Vec3fEq(377.221649169921875, 139.4449920654296875, -12.65070629119873046875),
            Vec3fEq(397.25634765625, 119.41030120849609375, -7.41098117828369140625),
            Vec3fEq(417.291046142578125, 99.3756103515625, -4.382833957672119140625),
            Vec3fEq(437.325714111328125, 79.340911865234375, -1.354687213897705078125),
            Vec3fEq(457.360443115234375, 59.3062286376953125, 1.624610424041748046875),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, update_then_find_random_point_around_circle_should_return_position)
    {
        const std::array<float, 6 * 6> heightfieldData {{
            0,   0,     0,     0,    0,    0,
            0, -25,   -25,   -25,  -25,  -25,
            0, -25, -1000, -1000, -100, -100,
            0, -25, -1000, -1000, -100, -100,
            0, -25,  -100,  -100, -100, -100,
            0, -25,  -100,  -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        Misc::Rng::init(42);

        const auto result = findRandomPointAroundCircle(*mNavigator, mAgentHalfExtents, mStart, 100.0, Flag_walk);

        ASSERT_THAT(result, Optional(Vec3fEq(70.35845947265625, 335.592041015625, -2.6667339801788330078125)))
            << (result ? *result : osg::Vec3f());

        const auto distance = (*result - mStart).length();

        EXPECT_FLOAT_EQ(distance, 125.80865478515625) << distance;
    }

    TEST_F(DetourNavigatorNavigatorTest, multiple_threads_should_lock_tiles)
    {
        mSettings.mAsyncNavMeshUpdaterThreads = 2;
        mNavigator.reset(new NavigatorImpl(mSettings, std::make_unique<NavMeshDb>(":memory:", std::numeric_limits<std::uint64_t>::max())));

        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);
        const btVector3 shift = getHeightfieldShift(mCellPosition, cellSize, surface.mMinHeight, surface.mMaxHeight);

        std::vector<CollisionShapeInstance<btBoxShape>> boxes;
        std::generate_n(std::back_inserter(boxes), 100, [] { return std::make_unique<btBoxShape>(btVector3(20, 20, 100)); });

        mNavigator->addAgent(mAgentHalfExtents);

        mNavigator->addHeightfield(mCellPosition, cellSize, surface);

        for (std::size_t i = 0; i < boxes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(shift.x() + i * 10, shift.y() + i * 10, i * 10));
            mNavigator->addObject(ObjectId(&boxes[i].shape()), ObjectShapes(boxes[i].instance(), mObjectTransform), transform);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));

        for (std::size_t i = 0; i < boxes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(shift.x() + i * 10 + 1, shift.y() + i * 10 + 1, i * 10 + 1));
            mNavigator->updateObject(ObjectId(&boxes[i].shape()), ObjectShapes(boxes[i].instance(), mObjectTransform), transform);
        }

        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 1.99998295307159423828125),
            Vec3fEq(69.5299530029296875, 434.754913330078125, -2.6775772571563720703125),
            Vec3fEq(82.39324951171875, 409.50982666015625, -7.355137348175048828125),
            Vec3fEq(95.25653839111328125, 384.2647705078125, -12.0326976776123046875),
            Vec3fEq(108.11983489990234375, 359.019683837890625, -16.71025848388671875),
            Vec3fEq(120.983123779296875, 333.774627685546875, -21.3878192901611328125),
            Vec3fEq(133.8464202880859375, 308.529541015625, -26.0653781890869140625),
            Vec3fEq(146.7097015380859375, 283.284454345703125, -30.7429370880126953125),
            Vec3fEq(159.572998046875, 258.039398193359375, -35.420497894287109375),
            Vec3fEq(172.4362945556640625, 232.7943115234375, -27.2731761932373046875),
            Vec3fEq(185.2996063232421875, 207.54925537109375, -20.3612518310546875),
            Vec3fEq(206.6449737548828125, 188.917236328125, -20.578319549560546875),
            Vec3fEq(227.9903564453125, 170.28521728515625, -26.291717529296875),
            Vec3fEq(253.4362640380859375, 157.8239593505859375, -34.784488677978515625),
            Vec3fEq(278.8822021484375, 145.3627166748046875, -30.253124237060546875),
            Vec3fEq(304.328094482421875, 132.9014739990234375, -25.72176361083984375),
            Vec3fEq(329.774017333984375, 120.44022369384765625, -21.1904010772705078125),
            Vec3fEq(355.219940185546875, 107.97898101806640625, -16.6590404510498046875),
            Vec3fEq(380.665863037109375, 95.51773834228515625, -12.127681732177734375),
            Vec3fEq(406.111785888671875, 83.05649566650390625, -7.5963191986083984375),
            Vec3fEq(431.557708740234375, 70.5952606201171875, -3.0649592876434326171875),
            Vec3fEq(457.003662109375, 58.134021759033203125, 1.4664003849029541015625),
            Vec3fEq(460, 56.66666412353515625, 1.99998295307159423828125)
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
            mNavigator->addObject(ObjectId(&shapes[i].shape()), ObjectShapes(shapes[i].instance(), mObjectTransform), transform);
        }
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const auto start = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < shapes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(i * 32 + 1, i * 32 + 1, i * 32 + 1));
            mNavigator->updateObject(ObjectId(&shapes[i].shape()), ObjectShapes(shapes[i].instance(), mObjectTransform), transform);
        }
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        for (std::size_t i = 0; i < shapes.size(); ++i)
        {
            const btTransform transform(btMatrix3x3::getIdentity(), btVector3(i * 32 + 2, i * 32 + 2, i * 32 + 2));
            mNavigator->updateObject(ObjectId(&shapes[i].shape()), ObjectShapes(shapes[i].instance(), mObjectTransform), transform);
        }
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const auto duration = std::chrono::steady_clock::now() - start;

        EXPECT_GT(duration, mSettings.mMinUpdateInterval)
            << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(duration).count() << " ms";
    }

    TEST_F(DetourNavigatorNavigatorTest, update_then_raycast_should_return_position)
    {
        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const osg::Vec3f start(57, 460, 1);
        const osg::Vec3f end(460, 57, 1);
        const auto result = raycast(*mNavigator, mAgentHalfExtents, start, end, Flag_walk);

        ASSERT_THAT(result, Optional(Vec3fEq(end.x(), end.y(), 1.95257937908172607421875)))
            << (result ? *result : osg::Vec3f());
    }

    TEST_F(DetourNavigatorNavigatorTest, update_for_oscillating_object_that_does_not_change_navmesh_should_not_trigger_navmesh_update)
    {
        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        CollisionShapeInstance oscillatingBox(std::make_unique<btBoxShape>(btVector3(20, 20, 20)));
        const btVector3 oscillatingBoxShapePosition(288, 288, 400);
        CollisionShapeInstance borderBox(std::make_unique<btBoxShape>(btVector3(50, 50, 50)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->addObject(ObjectId(&oscillatingBox.shape()), ObjectShapes(oscillatingBox.instance(), mObjectTransform),
                              btTransform(btMatrix3x3::getIdentity(), oscillatingBoxShapePosition));
        // add this box to make navmesh bound box independent from oscillatingBoxShape rotations
        mNavigator->addObject(ObjectId(&borderBox.shape()), ObjectShapes(borderBox.instance(), mObjectTransform),
                              btTransform(btMatrix3x3::getIdentity(), oscillatingBoxShapePosition + btVector3(0, 0, 200)));
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const Version expectedVersion {1, 4};

        const auto navMeshes = mNavigator->getNavMeshes();
        ASSERT_EQ(navMeshes.size(), 1);
        ASSERT_EQ(navMeshes.begin()->second->lockConst()->getVersion(), expectedVersion);

        for (int n = 0; n < 10; ++n)
        {
            const btTransform transform(btQuaternion(btVector3(0, 0, 1), n * 2 * osg::PI / 10),
                                        oscillatingBoxShapePosition);
            mNavigator->updateObject(ObjectId(&oscillatingBox.shape()), ObjectShapes(oscillatingBox.instance(), mObjectTransform), transform);
            mNavigator->update(mPlayerPosition);
            mNavigator->wait(mListener, WaitConditionType::allJobsDone);
        }

        ASSERT_EQ(navMeshes.size(), 1);
        ASSERT_EQ(navMeshes.begin()->second->lockConst()->getVersion(), expectedVersion);
    }

    TEST_F(DetourNavigatorNavigatorTest, should_provide_path_over_flat_heightfield)
    {
        const HeightfieldPlane plane {100};
        const int cellSize = mHeightfieldTileSize * 4;

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, plane);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::requiredTilesPresent);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, 101.99999237060546875),
            Vec3fEq(76.70135498046875, 439.965301513671875, 101.99999237060546875),
            Vec3fEq(96.73604583740234375, 419.93060302734375, 101.99999237060546875),
            Vec3fEq(116.770751953125, 399.89593505859375, 101.99999237060546875),
            Vec3fEq(136.8054351806640625, 379.861236572265625, 101.99999237060546875),
            Vec3fEq(156.840118408203125, 359.826568603515625, 101.99999237060546875),
            Vec3fEq(176.8748016357421875, 339.7918701171875, 101.99999237060546875),
            Vec3fEq(196.90948486328125, 319.757171630859375, 101.99999237060546875),
            Vec3fEq(216.944183349609375, 299.722503662109375, 101.99999237060546875),
            Vec3fEq(236.9788665771484375, 279.68780517578125, 101.99999237060546875),
            Vec3fEq(257.0135498046875, 259.65313720703125, 101.99999237060546875),
            Vec3fEq(277.048248291015625, 239.618438720703125, 101.99999237060546875),
            Vec3fEq(297.082916259765625, 219.583740234375, 101.99999237060546875),
            Vec3fEq(317.11761474609375, 199.549041748046875, 101.99999237060546875),
            Vec3fEq(337.15228271484375, 179.5143585205078125, 101.99999237060546875),
            Vec3fEq(357.186981201171875, 159.47967529296875, 101.99999237060546875),
            Vec3fEq(377.221649169921875, 139.4449920654296875, 101.99999237060546875),
            Vec3fEq(397.25634765625, 119.41030120849609375, 101.99999237060546875),
            Vec3fEq(417.291046142578125, 99.3756103515625, 101.99999237060546875),
            Vec3fEq(437.325714111328125, 79.340911865234375, 101.99999237060546875),
            Vec3fEq(457.360443115234375, 59.3062286376953125, 101.99999237060546875),
            Vec3fEq(460, 56.66666412353515625, 101.99999237060546875)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, for_not_reachable_destination_find_path_should_provide_partial_path)
    {
        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        CollisionShapeInstance compound(std::make_unique<btCompoundShape>());
        compound.shape().addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(204, -204, 0)),
                                       new btBoxShape(btVector3(200, 200, 1000)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->addObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, mEndTolerance, mOut),
                  Status::PartialPath);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66664886474609375, 460, -2.5371043682098388671875),
            Vec3fEq(76.42063140869140625, 439.6884765625, -2.9134314060211181640625),
            Vec3fEq(96.17461395263671875, 419.376953125, -4.50826549530029296875),
            Vec3fEq(115.9285888671875, 399.0654296875, -6.1030979156494140625),
            Vec3fEq(135.6825714111328125, 378.753936767578125, -7.697928905487060546875),
            Vec3fEq(155.436553955078125, 358.442413330078125, -20.9574985504150390625),
            Vec3fEq(175.190521240234375, 338.130889892578125, -35.907512664794921875),
            Vec3fEq(194.9445037841796875, 317.8193359375, -50.85752105712890625),
            Vec3fEq(214.698486328125, 297.5078125, -65.807525634765625),
            Vec3fEq(222.0001068115234375, 290.000091552734375, -71.333465576171875)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, end_tolerance_should_extent_available_destinations)
    {
        const std::array<float, 5 * 5> heightfieldData {{
            0,   0,    0,    0,    0,
            0, -25,  -25,  -25,  -25,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
            0, -25, -100, -100, -100,
        }};
        const HeightfieldSurface surface = makeSquareHeightfieldSurface(heightfieldData);
        const int cellSize = mHeightfieldTileSize * (surface.mSize - 1);

        CollisionShapeInstance compound(std::make_unique<btCompoundShape>());
        compound.shape().addChildShape(btTransform(btMatrix3x3::getIdentity(), btVector3(204, -204, 0)),
                                       new btBoxShape(btVector3(100, 100, 1000)));

        mNavigator->addAgent(mAgentHalfExtents);
        mNavigator->addHeightfield(mCellPosition, cellSize, surface);
        mNavigator->addObject(ObjectId(&compound.shape()), ObjectShapes(compound.instance(), mObjectTransform), mTransform);
        mNavigator->update(mPlayerPosition);
        mNavigator->wait(mListener, WaitConditionType::allJobsDone);

        const float endTolerance = 1000.0f;

        EXPECT_EQ(findPath(*mNavigator, mAgentHalfExtents, mStepSize, mStart, mEnd, Flag_walk, mAreaCosts, endTolerance, mOut),
                  Status::Success);

        EXPECT_THAT(mPath, ElementsAre(
            Vec3fEq(56.66666412353515625, 460, -2.5371043682098388671875),
            Vec3fEq(71.5649566650390625, 435.899810791015625, -5.817593097686767578125),
            Vec3fEq(86.46324920654296875, 411.79962158203125, -9.66499996185302734375),
            Vec3fEq(101.36154937744140625, 387.699462890625, -13.512401580810546875),
            Vec3fEq(116.2598419189453125, 363.599273681640625, -17.359806060791015625),
            Vec3fEq(131.1581268310546875, 339.499114990234375, -21.2072086334228515625),
            Vec3fEq(146.056427001953125, 315.39892578125, -25.0546112060546875),
            Vec3fEq(160.9547271728515625, 291.298736572265625, -28.9020137786865234375),
            Vec3fEq(175.8530120849609375, 267.198577880859375, -32.749416351318359375),
            Vec3fEq(190.751312255859375, 243.098388671875, -33.819454193115234375),
            Vec3fEq(205.64959716796875, 218.9982147216796875, -31.020172119140625),
            Vec3fEq(220.5478973388671875, 194.898040771484375, -26.844608306884765625),
            Vec3fEq(235.446197509765625, 170.7978668212890625, -26.785541534423828125),
            Vec3fEq(250.3444671630859375, 146.6976776123046875, -26.7264766693115234375),
            Vec3fEq(265.242767333984375, 122.59751129150390625, -20.59339141845703125),
            Vec3fEq(280.141021728515625, 98.4973297119140625, -14.040531158447265625),
            Vec3fEq(295.039306640625, 74.39715576171875, -7.48766994476318359375),
            Vec3fEq(306, 56.66666412353515625, -2.6667339801788330078125)
        )) << mPath;
    }

    TEST_F(DetourNavigatorNavigatorTest, only_one_water_per_cell_is_allowed)
    {
        const int cellSize1 = 100;
        const float level1 = 1;
        const int cellSize2 = 200;
        const float level2 = 2;

        mNavigator->addAgent(mAgentHalfExtents);
        EXPECT_TRUE(mNavigator->addWater(mCellPosition, cellSize1, level1));
        EXPECT_FALSE(mNavigator->addWater(mCellPosition, cellSize2, level2));
    }
}
