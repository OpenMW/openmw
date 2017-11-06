#ifndef OPENMW_TEST_SUITE_MWMECHANICS_COLLISIONOBJECTS_HPP
#define OPENMW_TEST_SUITE_MWMECHANICS_COLLISIONOBJECTS_HPP

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <BulletCollision/CollisionShapes/btConvexShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btPolyhedralConvexShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <osg/Math>

#include <array>
#include <vector>
#include <algorithm>

namespace
{
    template <class T>
    void init(T& value)
    {
        value.object.setCollisionShape(&value.shape);
        value.object.setWorldTransform(value.transform);
        value.object.setCollisionFlags(value.flags);
    }

    btTriangleMesh makeTriangleMesh(const std::vector<std::vector<btVector3>>& triangles)
    {
        btTriangleMesh shape(false);
        for (const auto& triangle : triangles)
        {
            shape.addTriangle(triangle[0], triangle[1], triangle[2]);
        }
        return shape;
    }

    struct Actor
    {
        const btVector3 halfExtents {29.27999496459961, 28.479997634887695, 66.5};
        const btVector3 origin {0, 0, halfExtents.z() + btScalar(1)};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btCapsuleShapeZ shape {halfExtents.x(), 2 * halfExtents.z() - 2 * halfExtents.x()};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_KINEMATIC_OBJECT;
        btCollisionObject object;

        Actor() { init(*this); }
    };

    struct Floor
    {
        const btVector3 origin {0, 0, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(0, 0, 1), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Floor() { init(*this); }
    };

    template <int x, int y, int z, int radius = 250>
    struct Sphere
    {
        const btVector3 origin {x, y, z};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btSphereShape shape {radius};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Sphere() { init(*this); }
    };

    struct Capsule
    {
        const btVector3 halfExtents {29.27999496459961, 28.479997634887695, 66.5};
        const btVector3 origin {400, 0, halfExtents.z() + btScalar(1)};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btCapsuleShapeZ shape {halfExtents.x(), 2 * halfExtents.z() - 2 * halfExtents.x()};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Capsule() { init(*this); }
    };

    template <int x, int y, int z>
    struct Box
    {
        const btVector3 origin {x, y, z};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(100, 100, 100)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Box() { init(*this); }
    };

    struct Compound
    {
        struct Right
        {
            const btVector3 origin {0, 250, 0};
            const btTransform transform {btMatrix3x3::getIdentity(), origin};
            btBoxShape shape {btVector3(50, 200, 100)};
        } right;

        struct Top
        {
            const btVector3 origin {0, 0, 150};
            const btTransform transform {btMatrix3x3::getIdentity(), origin};
            btBoxShape shape {btVector3(50, 450, 50)};
        } top;

        struct Left
        {
            const btVector3 origin {0, -250, 0};
            const btTransform transform {btMatrix3x3::getIdentity(), origin};
            btBoxShape shape {btVector3(50, 200, 100)};
        } left;

        const btVector3 origin {400, 100, 100};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btCompoundShape shape;
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Compound()
        {
            shape.addChildShape(right.transform, &right.shape);
            shape.addChildShape(top.transform, &top.shape);
            shape.addChildShape(left.transform, &left.shape);
            shape.recalculateLocalAabb();
            init(*this);
        }
    };

    struct Stair
    {
        const btVector3 origin {400, 0, -5};
        const btTransform transform {btMatrix3x3(btQuaternion(btVector3(0, 1, 0), btScalar(osg::PI_4))), origin};
        btBoxShape shape {btVector3(10, 1000, 10)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Stair() { init(*this); }
    };

    struct LeftWall
    {
        const btVector3 origin {0, -1000, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(0, 1, 0), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        LeftWall() { init(*this); }
    };

    struct RightWall
    {
        const btVector3 origin {0, 1000, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(0, -1, 0), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        RightWall() { init(*this); }
    };

    struct Roof
    {
        const btVector3 origin {0, 0, 1000};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(0, 0, -1), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Roof() { init(*this); }
    };

    struct FirstPlatform
    {
        const btVector3 origin {0, 0, -100};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(100, 100, 100)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        FirstPlatform() { init(*this); }
    };

    struct SecondLoweredPlatform
    {
        const btVector3 origin {200, 0, -200};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(100, 100, 100)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        SecondLoweredPlatform() { init(*this); }
    };

    struct ThirdPlatform
    {
        const btVector3 origin {400, 0, -100};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(100, 100, 100)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        ThirdPlatform() { init(*this); }
    };

    struct Slope
    {
        const btScalar angle = osg::DegreesToRadians(btScalar(48));
        const btVector3 origin {400, 0, 200 * std::sin(angle)};
        const btTransform transform {btMatrix3x3(btQuaternion(btVector3(0, 1, 0), -angle)), origin};
        btBoxShape shape {btVector3(200, 100, 1)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Slope() { init(*this); }
    };

    struct ElevatedPlatform
    {
        const btScalar angle = osg::DegreesToRadians(btScalar(48));
        const btVector3 origin {600 + 200 * std::cos(angle), 0, 400 * std::sin(angle)};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(200, 100, 1)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        ElevatedPlatform() { init(*this); }
    };

    struct SteepSlope
    {
        const btScalar angle = osg::DegreesToRadians(btScalar(80));
        const btVector3 origin {400, 0, 200 * std::sin(angle)};
        const btTransform transform {btMatrix3x3(btQuaternion(btVector3(0, 1, 0), -angle)), origin};
        btBoxShape shape {btVector3(200, 100, 1)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        SteepSlope() { init(*this); }
    };

    struct UnreachableElevatedPlatform
    {
        const btScalar angle = osg::DegreesToRadians(btScalar(50));
        const btVector3 origin {600 + 200 * std::cos(angle), 0, 400 * std::sin(angle)};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(200, 100, 1)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        UnreachableElevatedPlatform() { init(*this); }
    };

    struct EllispoidY
    {
        const btVector3 origin {400, 0, 50};
        const btTransform transform {btMatrix3x3::getIdentity().scaled(btVector3(1, 2, 1)), origin};
        btSphereShape shape {50};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        EllispoidY() { init(*this); }
    };

    struct TurnBackWall
    {
        const btVector3 origin {-1000, 0, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(1, 0, 0), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        TurnBackWall() { init(*this); }
    };

    struct TurnFrontWall
    {
        const btVector3 origin {0, 0, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(-1, 0, 0), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        TurnFrontWall() { init(*this); }
    };

    struct TurnLeftWall
    {
        const btVector3 origin {0, 0, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(0, -1, 0), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        TurnLeftWall() { init(*this); }
    };

    struct TurnRightWall
    {
        const btVector3 origin {0, -1000, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(0, 1, 0), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        TurnRightWall() { init(*this); }
    };

    struct TurnInside
    {
        const btVector3 origin {-1000, -1000, 500};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(940, 940, 500)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        TurnInside() { init(*this); }
    };

    struct UTurnInsideWall
    {
        const btVector3 origin {-1000, -60, 500};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btBoxShape shape {btVector3(940, 1, 500)};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        UTurnInsideWall() { init(*this); }
    };

    struct UTurnRightWall
    {
        const btVector3 origin {0, -121, 0};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        btStaticPlaneShape shape {btVector3(0, 1, 0), 0};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        UTurnRightWall() { init(*this); }
    };

    struct Mesh
    {
        const std::vector<std::vector<btVector3>> triangles {{
            {btVector3(-114.477508544921875, -146.597381591796875, -21.62689971923828125), btVector3(-73.4812164306640625, -143.31243896484375, 1.9177703857421875), btVector3(-44.60581207275390625, 4.470401763916015625, 62.26615142822265625)},
            {btVector3(-44.60581207275390625, 4.470401763916015625, 62.26615142822265625), btVector3(-167.7228240966796875, -48.69559478759765625, -21.62689971923828125), btVector3(-114.477508544921875, -146.597381591796875, -21.62689971923828125)},
            {btVector3(-73.4812164306640625, -143.31243896484375, 1.9177703857421875), btVector3(-13.20505523681640625, -65.31792449951171875, 44.1930084228515625), btVector3(-44.60581207275390625, 4.470401763916015625, 62.26615142822265625)},
            {btVector3(-9.643306732177734375, 19.6725254058837890625, 67.32411956787109375), btVector3(-44.60581207275390625, 4.470401763916015625, 62.26615142822265625), btVector3(-13.20505523681640625, -65.31792449951171875, 44.1930084228515625)},
            {btVector3(166.892181396484375, -62.2358245849609375, 54.0697479248046875), btVector3(233.0417938232421875, -0.644420623779296875, 9.42864227294921875), btVector3(146.439453125, 85.5523834228515625, 9.42864227294921875)},
            {btVector3(146.439453125, 85.5523834228515625, 9.42864227294921875), btVector3(111.245361328125, 75.8625946044921875, 83.74178314208984375), btVector3(166.892181396484375, -62.2358245849609375, 54.0697479248046875)},
            {btVector3(-167.7228240966796875, -48.69559478759765625, -21.62689971923828125), btVector3(-44.60581207275390625, 4.470401763916015625, 62.26615142822265625), btVector3(-193.2003021240234375, 119.9915008544921875, 28.84505462646484375)},
            {btVector3(-193.2003021240234375, 119.9915008544921875, 28.84505462646484375), btVector3(-225.829345703125, 83.18597412109375, -21.62689971923828125), btVector3(-167.7228240966796875, -48.69559478759765625, -21.62689971923828125)},
            {btVector3(-44.60581207275390625, 4.470401763916015625, 62.26615142822265625), btVector3(-9.643306732177734375, 19.6725254058837890625, 67.32411956787109375), btVector3(-143.5372314453125, 132.8759613037109375, 36.259674072265625)},
            {btVector3(-143.5372314453125, 132.8759613037109375, 36.259674072265625), btVector3(-193.2003021240234375, 119.9915008544921875, 28.84505462646484375), btVector3(-44.60581207275390625, 4.470401763916015625, 62.26615142822265625)},
            {btVector3(111.245361328125, 75.8625946044921875, 83.74178314208984375), btVector3(146.439453125, 85.5523834228515625, 9.42864227294921875), btVector3(82.9957733154296875, 146.936798095703125, 9.42864227294921875)},
            {btVector3(82.9957733154296875, 146.936798095703125, 9.42864227294921875), btVector3(76.13427734375, 108.4758453369140625, 79.62641143798828125), btVector3(111.245361328125, 75.8625946044921875, 83.74178314208984375)},
            {btVector3(-114.477508544921875, -150.4773712158203125, -59.69403839111328125), btVector3(-64.3871917724609375, -165.521484375, -59.69403839111328125), btVector3(-73.4812164306640625, -143.31243896484375, 1.9177703857421875)},
            {btVector3(-73.4812164306640625, -143.31243896484375, 1.9177703857421875), btVector3(-114.477508544921875, -146.597381591796875, -21.62689971923828125), btVector3(-114.477508544921875, -150.4773712158203125, -59.69403839111328125)},
            {btVector3(-64.3871917724609375, -165.521484375, -59.69403839111328125), btVector3(5.43251800537109375, -90.4968414306640625, -59.69403839111328125), btVector3(-73.4812164306640625, -143.31243896484375, 1.9177703857421875)},
            {btVector3(-13.20505523681640625, -65.31792449951171875, 44.1930084228515625), btVector3(-73.4812164306640625, -143.31243896484375, 1.9177703857421875), btVector3(5.43251800537109375, -90.4968414306640625, -59.69403839111328125)},
            {btVector3(204.1771240234375, -67.90195465087890625, -59.69403839111328125), btVector3(233.0417938232421875, -0.644435882568359375, -59.69403839111328125), btVector3(233.0417938232421875, -0.644420623779296875, 9.42864227294921875)},
            {btVector3(233.0417938232421875, -0.644420623779296875, 9.42864227294921875), btVector3(166.892181396484375, -62.2358245849609375, 54.0697479248046875), btVector3(204.1771240234375, -67.90195465087890625, -59.69403839111328125)},
            {btVector3(233.0417938232421875, -0.644435882568359375, -59.69403839111328125), btVector3(146.439453125, 85.5523681640625, -59.69403839111328125), btVector3(146.439453125, 85.5523834228515625, 9.42864227294921875)},
            {btVector3(146.439453125, 85.5523834228515625, 9.42864227294921875), btVector3(233.0417938232421875, -0.644420623779296875, 9.42864227294921875), btVector3(233.0417938232421875, -0.644435882568359375, -59.69403839111328125)},
            {btVector3(146.439453125, 85.5523681640625, -59.69403839111328125), btVector3(82.99576568603515625, 146.9367828369140625, -59.69403839111328125), btVector3(82.9957733154296875, 146.936798095703125, 9.42864227294921875)},
            {btVector3(82.9957733154296875, 146.936798095703125, 9.42864227294921875), btVector3(146.439453125, 85.5523834228515625, 9.42864227294921875), btVector3(146.439453125, 85.5523681640625, -59.69403839111328125)},
            {btVector3(82.99576568603515625, 146.9367828369140625, -59.69403839111328125), btVector3(-9.458038330078125, 134.382843017578125, -59.69403839111328125), btVector3(76.13427734375, 108.4758453369140625, 79.62641143798828125)},
            {btVector3(76.13427734375, 108.4758453369140625, 79.62641143798828125), btVector3(82.9957733154296875, 146.936798095703125, 9.42864227294921875), btVector3(82.99576568603515625, 146.9367828369140625, -59.69403839111328125)},
            {btVector3(-9.458038330078125, 134.382843017578125, -59.69403839111328125), btVector3(-59.33648681640625, 164.923309326171875, -59.69403839111328125), btVector3(-44.1344451904296875, 129.96099853515625, 49.08777618408203125)},
            {btVector3(-44.1344451904296875, 129.96099853515625, 49.08777618408203125), btVector3(76.13427734375, 108.4758453369140625, 79.62641143798828125), btVector3(-9.458038330078125, 134.382843017578125, -59.69403839111328125)},
            {btVector3(-206.0509033203125, 139.53753662109375, -59.69403839111328125), btVector3(-225.829345703125, 83.1859588623046875, -59.69403839111328125), btVector3(-225.829345703125, 83.18597412109375, -21.62689971923828125)},
            {btVector3(-225.829345703125, 83.18597412109375, -21.62689971923828125), btVector3(-193.2003021240234375, 119.9915008544921875, 28.84505462646484375), btVector3(-206.0509033203125, 139.53753662109375, -59.69403839111328125)},
            {btVector3(-225.829345703125, 83.1859588623046875, -59.69403839111328125), btVector3(-167.7228240966796875, -48.695587158203125, -59.69403839111328125), btVector3(-167.7228240966796875, -48.69559478759765625, -21.62689971923828125)},
            {btVector3(-167.7228240966796875, -48.69559478759765625, -21.62689971923828125), btVector3(-225.829345703125, 83.18597412109375, -21.62689971923828125), btVector3(-225.829345703125, 83.1859588623046875, -59.69403839111328125)},
            {btVector3(-167.7228240966796875, -48.695587158203125, -59.69403839111328125), btVector3(-114.477508544921875, -150.4773712158203125, -59.69403839111328125), btVector3(-114.477508544921875, -146.597381591796875, -21.62689971923828125)},
            {btVector3(-114.477508544921875, -146.597381591796875, -21.62689971923828125), btVector3(-167.7228240966796875, -48.69559478759765625, -21.62689971923828125), btVector3(-167.7228240966796875, -48.695587158203125, -59.69403839111328125)},
            {btVector3(78.02291107177734375, -56.20673370361328125, 59.3943634033203125), btVector3(166.892181396484375, -62.2358245849609375, 54.0697479248046875), btVector3(36.38812255859375, 49.786357879638671875, 98.27130889892578125)},
            {btVector3(111.245361328125, 75.8625946044921875, 83.74178314208984375), btVector3(36.38812255859375, 49.786357879638671875, 98.27130889892578125), btVector3(166.892181396484375, -62.2358245849609375, 54.0697479248046875)},
            {btVector3(36.38812255859375, 49.786357879638671875, 98.27130889892578125), btVector3(111.245361328125, 75.8625946044921875, 83.74178314208984375), btVector3(76.13427734375, 108.4758453369140625, 79.62641143798828125)},
            {btVector3(76.13427734375, 108.4758453369140625, 79.62641143798828125), btVector3(-44.1344451904296875, 129.96099853515625, 49.08777618408203125), btVector3(36.38812255859375, 49.786357879638671875, 98.27130889892578125)},
            {btVector3(75.41629791259765625, -74.87143707275390625, -59.69403839111328125), btVector3(204.1771240234375, -67.90195465087890625, -59.69403839111328125), btVector3(166.892181396484375, -62.2358245849609375, 54.0697479248046875)},
            {btVector3(166.892181396484375, -62.2358245849609375, 54.0697479248046875), btVector3(78.02291107177734375, -56.20673370361328125, 59.3943634033203125), btVector3(75.41629791259765625, -74.87143707275390625, -59.69403839111328125)},
            {btVector3(-59.33648681640625, 164.923309326171875, -59.69403839111328125), btVector3(-136.2525482177734375, 162.112640380859375, -59.69403839111328125), btVector3(-143.5372314453125, 132.8759613037109375, 36.259674072265625)},
            {btVector3(-143.5372314453125, 132.8759613037109375, 36.259674072265625), btVector3(-44.1344451904296875, 129.96099853515625, 49.08777618408203125), btVector3(-59.33648681640625, 164.923309326171875, -59.69403839111328125)},
            {btVector3(-13.20505523681640625, -65.31792449951171875, 44.1930084228515625), btVector3(78.02291107177734375, -56.20673370361328125, 59.3943634033203125), btVector3(-9.643306732177734375, 19.6725254058837890625, 67.32411956787109375)},
            {btVector3(36.38812255859375, 49.786357879638671875, 98.27130889892578125), btVector3(-9.643306732177734375, 19.6725254058837890625, 67.32411956787109375), btVector3(78.02291107177734375, -56.20673370361328125, 59.3943634033203125)},
            {btVector3(-9.643306732177734375, 19.6725254058837890625, 67.32411956787109375), btVector3(36.38812255859375, 49.786357879638671875, 98.27130889892578125), btVector3(-44.1344451904296875, 129.96099853515625, 49.08777618408203125)},
            {btVector3(-44.1344451904296875, 129.96099853515625, 49.08777618408203125), btVector3(-143.5372314453125, 132.8759613037109375, 36.259674072265625), btVector3(-9.643306732177734375, 19.6725254058837890625, 67.32411956787109375)},
            {btVector3(5.43251800537109375, -90.4968414306640625, -59.69403839111328125), btVector3(75.41629791259765625, -74.87143707275390625, -59.69403839111328125), btVector3(78.02291107177734375, -56.20673370361328125, 59.3943634033203125)},
            {btVector3(78.02291107177734375, -56.20673370361328125, 59.3943634033203125), btVector3(-13.20505523681640625, -65.31792449951171875, 44.1930084228515625), btVector3(5.43251800537109375, -90.4968414306640625, -59.69403839111328125)},
            {btVector3(-136.2525482177734375, 162.112640380859375, -59.69403839111328125), btVector3(-206.0509033203125, 139.53753662109375, -59.69403839111328125), btVector3(-193.2003021240234375, 119.9915008544921875, 28.84505462646484375)},
            {btVector3(-193.2003021240234375, 119.9915008544921875, 28.84505462646484375), btVector3(-143.5372314453125, 132.8759613037109375, 36.259674072265625), btVector3(-136.2525482177734375, 162.112640380859375, -59.69403839111328125)},
        }};
        const btVector3 origin {56358.9140625, -49133.828125, 520.4376220703125};
        const btTransform transform {
            btMatrix3x3 {
                0.01159584522247314453125, 0.93638515472412109375, -0.350782603025436401367188,
                -0.9893035888671875, 0.06175744533538818359375, 0.132152974605560302734375,
                0.145409524440765380859375, 0.34549808502197265625, 0.92708528041839599609375,
            },
            origin,
        };
        btTriangleMesh mesh = makeTriangleMesh(triangles);
        btBvhTriangleMeshShape child {&mesh, true};
        const btVector3 scaling {1.610000133514404296875, 1.610000133514404296875, 1.610000133514404296875};
        btScaledBvhTriangleMeshShape shape {&child, scaling};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Mesh() { init(*this); }
    };

    struct Mesh2
    {
        const std::vector<std::vector<btVector3>> triangles {{
            {btVector3(-245.40185546875, -107.0033111572265625, -87.27094268798828125), btVector3(-129.9500274658203125, -63.68837738037109375, -18.95238494873046875), btVector3(-72.8401641845703125, 29.562168121337890625, 24.33300018310546875)},
            {btVector3(-72.8401641845703125, 29.562168121337890625, 24.33300018310546875), btVector3(-266.35302734375, -37.80367279052734375, -87.27094268798828125), btVector3(-245.40185546875, -107.0033111572265625, -87.27094268798828125)},
            {btVector3(-129.9500274658203125, -63.68837738037109375, -18.95238494873046875), btVector3(-63.398223876953125, -44.517730712890625, -12.27741241455078125), btVector3(-72.8401641845703125, 29.562168121337890625, 24.33300018310546875)},
            {btVector3(-34.71561431884765625, 29.562168121337890625, 29.39096832275390625), btVector3(-72.8401641845703125, 29.562168121337890625, 24.33300018310546875), btVector3(-63.398223876953125, -44.517730712890625, -12.27741241455078125)},
            {btVector3(159.370941162109375, -72.2315673828125, -9.82831573486328125), btVector3(206.46435546875, -138.4351654052734375, -56.21540069580078125), btVector3(308.36669921875, -15.01595306396484375, -56.21540069580078125)},
            {btVector3(308.36669921875, -15.01595306396484375, -56.21540069580078125), btVector3(159.370941162109375, 29.562164306640625, 30.56577301025390625), btVector3(159.370941162109375, -72.2315673828125, -9.82831573486328125)},
            {btVector3(-266.35302734375, -37.80367279052734375, -87.27094268798828125), btVector3(-72.8401641845703125, 29.562168121337890625, 24.33300018310546875), btVector3(-159.6646575927734375, 129.527557373046875, -38.45085906982421875)},
            {btVector3(-159.6646575927734375, 129.527557373046875, -38.45085906982421875), btVector3(-168.52783203125, 159.794952392578125, -87.27094268798828125), btVector3(-266.35302734375, -37.80367279052734375, -87.27094268798828125)},
            {btVector3(-72.8401641845703125, 29.562168121337890625, 24.33300018310546875), btVector3(-34.71561431884765625, 29.562168121337890625, 29.39096832275390625), btVector3(-34.715610504150390625, 128.923736572265625, -11.595306396484375)},
            {btVector3(-34.715610504150390625, 128.923736572265625, -11.595306396484375), btVector3(-159.6646575927734375, 129.527557373046875, -38.45085906982421875), btVector3(-72.8401641845703125, 29.562168121337890625, 24.33300018310546875)},
            {btVector3(159.370941162109375, 29.562164306640625, 30.56577301025390625), btVector3(308.36669921875, -15.01595306396484375, -56.21540069580078125), btVector3(197.0830078125, 132.9785003662109375, -56.21540069580078125)},
            {btVector3(197.0830078125, 132.9785003662109375, -56.21540069580078125), btVector3(159.370941162109375, 128.923736572265625, 7.59090423583984375), btVector3(159.370941162109375, 29.562164306640625, 30.56577301025390625)},
            {btVector3(-245.40185546875, -107.003326416015625, -154.53173828125), btVector3(-129.9500274658203125, -117.751556396484375, -154.53173828125), btVector3(-129.9500274658203125, -63.68837738037109375, -18.95238494873046875)},
            {btVector3(-129.9500274658203125, -63.68837738037109375, -18.95238494873046875), btVector3(-245.40185546875, -107.0033111572265625, -87.27094268798828125), btVector3(-245.40185546875, -107.003326416015625, -154.53173828125)},
            {btVector3(-129.9500274658203125, -117.751556396484375, -154.53173828125), btVector3(-64.82010650634765625, -77.481231689453125, -154.53173828125), btVector3(-129.9500274658203125, -63.68837738037109375, -18.95238494873046875)},
            {btVector3(-63.398223876953125, -44.517730712890625, -12.27741241455078125), btVector3(-129.9500274658203125, -63.68837738037109375, -18.95238494873046875), btVector3(-64.82010650634765625, -77.481231689453125, -154.53173828125)},
            {btVector3(121.647796630859375, -138.4351806640625, -154.53173828125), btVector3(206.46435546875, -138.4351806640625, -154.53173828125), btVector3(206.46435546875, -138.4351654052734375, -56.21540069580078125)},
            {btVector3(206.46435546875, -138.4351654052734375, -56.21540069580078125), btVector3(159.370941162109375, -72.2315673828125, -9.82831573486328125), btVector3(121.647796630859375, -138.4351806640625, -154.53173828125)},
            {btVector3(206.46435546875, -138.4351806640625, -154.53173828125), btVector3(308.36669921875, -15.015960693359375, -154.53173828125), btVector3(308.36669921875, -15.01595306396484375, -56.21540069580078125)},
            {btVector3(308.36669921875, -15.01595306396484375, -56.21540069580078125), btVector3(206.46435546875, -138.4351654052734375, -56.21540069580078125), btVector3(206.46435546875, -138.4351806640625, -154.53173828125)},
            {btVector3(308.36669921875, -15.015960693359375, -154.53173828125), btVector3(197.0830078125, 132.978485107421875, -154.53173828125), btVector3(197.0830078125, 132.9785003662109375, -56.21540069580078125)},
            {btVector3(197.0830078125, 132.9785003662109375, -56.21540069580078125), btVector3(308.36669921875, -15.01595306396484375, -56.21540069580078125), btVector3(308.36669921875, -15.015960693359375, -154.53173828125)},
            {btVector3(197.0830078125, 132.978485107421875, -154.53173828125), btVector3(46.318695068359375, 169.0604095458984375, -154.53173828125), btVector3(159.370941162109375, 128.923736572265625, 7.59090423583984375)},
            {btVector3(159.370941162109375, 128.923736572265625, 7.59090423583984375), btVector3(197.0830078125, 132.9785003662109375, -56.21540069580078125), btVector3(197.0830078125, 132.978485107421875, -154.53173828125)},
            {btVector3(46.318695068359375, 169.0604095458984375, -154.53173828125), btVector3(9.7556304931640625, 169.2423248291015625, -154.53173828125), btVector3(9.75562953948974609375, 128.923736572265625, -9.7012939453125)},
            {btVector3(9.75562953948974609375, 128.923736572265625, -9.7012939453125), btVector3(159.370941162109375, 128.923736572265625, 7.59090423583984375), btVector3(46.318695068359375, 169.0604095458984375, -154.53173828125)},
            {btVector3(-72.8401641845703125, 169.4899444580078125, -154.53173828125), btVector3(-168.52783203125, 159.7949371337890625, -154.53173828125), btVector3(-168.52783203125, 159.794952392578125, -87.27094268798828125)},
            {btVector3(-168.52783203125, 159.794952392578125, -87.27094268798828125), btVector3(-159.6646575927734375, 129.527557373046875, -38.45085906982421875), btVector3(-72.8401641845703125, 169.4899444580078125, -154.53173828125)},
            {btVector3(-168.52783203125, 159.7949371337890625, -154.53173828125), btVector3(-266.35302734375, -37.803680419921875, -154.53173828125), btVector3(-266.35302734375, -37.80367279052734375, -87.27094268798828125)},
            {btVector3(-266.35302734375, -37.80367279052734375, -87.27094268798828125), btVector3(-168.52783203125, 159.794952392578125, -87.27094268798828125), btVector3(-168.52783203125, 159.7949371337890625, -154.53173828125)},
            {btVector3(-266.35302734375, -37.803680419921875, -154.53173828125), btVector3(-245.40185546875, -107.003326416015625, -154.53173828125), btVector3(-245.40185546875, -107.0033111572265625, -87.27094268798828125)},
            {btVector3(-245.40185546875, -107.0033111572265625, -87.27094268798828125), btVector3(-266.35302734375, -37.80367279052734375, -87.27094268798828125), btVector3(-266.35302734375, -37.803680419921875, -154.53173828125)},
            {btVector3(18.284912109375, -69.66729736328125, -10.55902862548828125), btVector3(159.370941162109375, -72.2315673828125, -9.82831573486328125), btVector3(18.28491973876953125, 29.5621662139892578125, 34.42610931396484375)},
            {btVector3(159.370941162109375, 29.562164306640625, 30.56577301025390625), btVector3(18.28491973876953125, 29.5621662139892578125, 34.42610931396484375), btVector3(159.370941162109375, -72.2315673828125, -9.82831573486328125)},
            {btVector3(18.28491973876953125, 29.5621662139892578125, 34.42610931396484375), btVector3(159.370941162109375, 29.562164306640625, 30.56577301025390625), btVector3(159.370941162109375, 128.923736572265625, 7.59090423583984375)},
            {btVector3(159.370941162109375, 128.923736572265625, 7.59090423583984375), btVector3(9.75562953948974609375, 128.923736572265625, -9.7012939453125), btVector3(18.28491973876953125, 29.5621662139892578125, 34.42610931396484375)},
            {btVector3(60.518802642822265625, -143.243988037109375, -154.53173828125), btVector3(121.647796630859375, -138.4351806640625, -154.53173828125), btVector3(159.370941162109375, -72.2315673828125, -9.82831573486328125)},
            {btVector3(159.370941162109375, -72.2315673828125, -9.82831573486328125), btVector3(18.284912109375, -69.66729736328125, -10.55902862548828125), btVector3(60.518802642822265625, -143.243988037109375, -154.53173828125)},
            {btVector3(9.7556304931640625, 169.2423248291015625, -154.53173828125), btVector3(-34.71561431884765625, 169.3756561279296875, -154.53173828125), btVector3(-34.715610504150390625, 128.923736572265625, -11.595306396484375)},
            {btVector3(-34.715610504150390625, 128.923736572265625, -11.595306396484375), btVector3(9.75562953948974609375, 128.923736572265625, -9.7012939453125), btVector3(9.7556304931640625, 169.2423248291015625, -154.53173828125)},
            {btVector3(-63.398223876953125, -44.517730712890625, -12.27741241455078125), btVector3(18.284912109375, -69.66729736328125, -10.55902862548828125), btVector3(-34.71561431884765625, 29.562168121337890625, 29.39096832275390625)},
            {btVector3(18.28491973876953125, 29.5621662139892578125, 34.42610931396484375), btVector3(-34.71561431884765625, 29.562168121337890625, 29.39096832275390625), btVector3(18.284912109375, -69.66729736328125, -10.55902862548828125)},
            {btVector3(-34.71561431884765625, 29.562168121337890625, 29.39096832275390625), btVector3(18.28491973876953125, 29.5621662139892578125, 34.42610931396484375), btVector3(9.75562953948974609375, 128.923736572265625, -9.7012939453125)},
            {btVector3(9.75562953948974609375, 128.923736572265625, -9.7012939453125), btVector3(-34.715610504150390625, 128.923736572265625, -11.595306396484375), btVector3(-34.71561431884765625, 29.562168121337890625, 29.39096832275390625)},
            {btVector3(-64.82010650634765625, -77.481231689453125, -154.53173828125), btVector3(60.518802642822265625, -143.243988037109375, -154.53173828125), btVector3(18.284912109375, -69.66729736328125, -10.55902862548828125)},
            {btVector3(18.284912109375, -69.66729736328125, -10.55902862548828125), btVector3(-63.398223876953125, -44.517730712890625, -12.27741241455078125), btVector3(-64.82010650634765625, -77.481231689453125, -154.53173828125)},
            {btVector3(-34.71561431884765625, 169.3756561279296875, -154.53173828125), btVector3(-72.8401641845703125, 169.4899444580078125, -154.53173828125), btVector3(-159.6646575927734375, 129.527557373046875, -38.45085906982421875)},
            {btVector3(-159.6646575927734375, 129.527557373046875, -38.45085906982421875), btVector3(-34.715610504150390625, 128.923736572265625, -11.595306396484375), btVector3(-34.71561431884765625, 169.3756561279296875, -154.53173828125)},
        }};
        const btVector3 origin {55885.75, -49099.9296875, 576.158203125};
        const btTransform transform {
            btMatrix3x3 {
                -0.1230890750885009765625, -0.947373807430267333984375, 0.295519769191741943359375,
                0.990511953830718994140625, -0.09894287586212158203125, 0.0953752323985099792480469,
                -0.0611164346337318420410156, 0.304455488920211791992188, 0.950563848018646240234375,
            },
            origin,
        };
        btTriangleMesh mesh = makeTriangleMesh(triangles);
        btBvhTriangleMeshShape child {&mesh, true};
        const btVector3 scaling {1.32000005245208740234375, 1.32000005245208740234375, 1.32000005245208740234375};
        btScaledBvhTriangleMeshShape shape {&child, scaling};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Mesh2() { init(*this); }
    };

    struct HeightField
    {
        static const int heightStickWidth = 65;
        static const int heightStickLength = 65;
        const std::array<btScalar, heightStickWidth * heightStickLength> heightfieldData {{
            1080, 1024, 944, 856, 760, 672, 560, 448, 336, 232, 144, 72, 8, -40, -80, -128, -160, -184, -184, -176, -144, -80, 0, 96, 192, 288, 384, 496, 632, 760, 864, 936, 984, 1000, 1024, 1048, 1072, 1120, 1184, 1192, 1208, 1216, 1232, 1248, 1272, 1304, 1360, 1384, 1408, 1424, 1440, 1448, 1456, 1456, 1456, 1456, 1456, 1440, 1352, 1360, 1400, 1616, 1768, 1928, 2072,
            992, 936, 848, 752, 648, 528, 416, 304, 208, 128, 64, 16, -32, -72, -112, -144, -176, -192, -200, -168, -120, -56, 40, 136, 232, 328, 432, 552, 688, 824, 928, 1008, 1048, 1056, 1080, 1096, 1112, 1136, 1184, 1208, 1208, 1224, 1240, 1264, 1296, 1320, 1352, 1376, 1392, 1384, 1384, 1384, 1384, 1392, 1392, 1392, 1352, 1352, 1352, 1352, 1400, 1536, 1736, 1920, 2080,
            912, 848, 768, 664, 544, 408, 296, 184, 96, 32, -8, -40, -72, -104, -136, -168, -200, -200, -192, -160, -104, -32, 72, 176, 272, 368, 472, 600, 744, 872, 976, 1048, 1096, 1120, 1136, 1152, 1152, 1160, 1176, 1184, 1192, 1208, 1216, 1256, 1288, 1312, 1336, 1360, 1368, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1368, 1512, 1728, 1920, 2088,
            832, 768, 680, 576, 448, 320, 200, 96, 16, -32, -64, -88, -112, -128, -152, -176, -200, -200, -192, -160, -120, -16, 104, 216, 328, 376, 488, 648, 792, 912, 1016, 1088, 1128, 1152, 1176, 1184, 1184, 1176, 1176, 1176, 1184, 1192, 1208, 1232, 1256, 1280, 1312, 1336, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1368, 1400, 1552, 1736, 1968, 2104,
            760, 680, 592, 488, 376, 240, 128, 24, -48, -88, -112, -128, -136, -152, -160, -168, -168, -160, -144, -104, -56, 40, 152, 272, 352, 424, 568, 704, 840, 968, 1072, 1144, 1176, 1192, 1176, 1184, 1184, 1184, 1184, 1184, 1184, 1192, 1200, 1216, 1248, 1272, 1304, 1344, 1352, 1352, 1352, 1352, 1352, 1352, 1352, 1344, 1352, 1352, 1352, 1376, 1416, 1584, 1792, 1968, 2136,
            696, 584, 504, 408, 288, 168, 56, -32, -104, -144, -160, -168, -168, -168, -160, -144, -120, -104, -72, -32, 8, 104, 216, 344, 408, 504, 640, 776, 872, 984, 1080, 1152, 1184, 1200, 1184, 1176, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1208, 1240, 1272, 1312, 1352, 1344, 1352, 1352, 1352, 1352, 1344, 1344, 1344, 1344, 1344, 1352, 1376, 1448, 1592, 1800, 2000, 2144,
            592, 480, 400, 312, 208, 104, 0, -88, -152, -192, -200, -200, -192, -176, -144, -104, -64, -32, 8, 48, 104, 184, 288, 400, 456, 544, 704, 792, 896, 992, 1080, 1144, 1176, 1208, 1184, 1176, 1176, 1184, 1184, 1184, 1184, 1184, 1184, 1208, 1240, 1272, 1328, 1360, 1360, 1352, 1352, 1352, 1352, 1344, 1344, 1344, 1344, 1344, 1352, 1392, 1464, 1584, 1792, 2000, 2152,
            488, 400, 320, 232, 144, 48, -40, -136, -192, -224, -240, -232, -208, -168, -120, -64, -8, 32, 80, 128, 184, 264, 352, 440, 520, 632, 744, 832, 904, 984, 1064, 1120, 1160, 1184, 1184, 1176, 1176, 1184, 1184, 1184, 1184, 1184, 1184, 1208, 1240, 1296, 1352, 1408, 1408, 1368, 1336, 1336, 1328, 1336, 1344, 1336, 1336, 1336, 1360, 1392, 1464, 1592, 1776, 1984, 2168,
            408, 328, 248, 168, 96, 24, -64, -152, -232, -264, -272, -264, -232, -176, -104, -32, 32, 80, 128, 184, 248, 320, 408, 488, 568, 672, 752, 840, 912, 984, 1056, 1112, 1144, 1176, 1176, 1176, 1176, 1184, 1184, 1184, 1184, 1184, 1192, 1208, 1240, 1288, 1376, 1472, 1448, 1392, 1336, 1328, 1328, 1328, 1336, 1328, 1328, 1328, 1336, 1376, 1456, 1584, 1768, 1960, 2144,
            336, 264, 192, 128, 72, 0, -88, -176, -256, -304, -312, -304, -256, -192, -112, -24, 48, 112, 168, 224, 288, 368, 440, 512, 592, 688, 760, 840, 912, 992, 1056, 1120, 1152, 1168, 1176, 1176, 1184, 1184, 1184, 1184, 1176, 1184, 1192, 1208, 1232, 1272, 1360, 1440, 1456, 1408, 1336, 1328, 1328, 1328, 1312, 1312, 1312, 1320, 1320, 1360, 1472, 1592, 1752, 1936, 2120,
            272, 208, 152, 112, 56, -16, -112, -200, -280, -344, -360, -336, -288, -216, -120, -24, 56, 128, 184, 256, 320, 392, 464, 528, 600, 680, 760, 840, 920, 984, 1048, 1120, 1152, 1168, 1176, 1184, 1184, 1176, 1184, 1176, 1176, 1184, 1192, 1208, 1224, 1248, 1304, 1368, 1400, 1360, 1328, 1320, 1320, 1296, 1288, 1296, 1304, 1312, 1320, 1368, 1472, 1600, 1744, 1904, 2064,
            216, 160, 120, 88, 40, -40, -136, -224, -312, -376, -384, -368, -320, -240, -136, -32, 56, 128, 192, 264, 328, 400, 464, 528, 584, 656, 744, 832, 904, 984, 1056, 1112, 1144, 1168, 1176, 1176, 1176, 1176, 1176, 1176, 1176, 1184, 1184, 1200, 1216, 1232, 1240, 1296, 1360, 1336, 1320, 1304, 1296, 1288, 1288, 1296, 1296, 1312, 1352, 1408, 1496, 1608, 1736, 1872, 1992,
            152, 120, 96, 72, 24, -56, -160, -264, -336, -392, -400, -376, -328, -248, -144, -40, 56, 128, 192, 256, 336, 408, 472, 528, 560, 624, 720, 816, 912, 1000, 1072, 1120, 1152, 1176, 1184, 1184, 1184, 1176, 1176, 1168, 1168, 1176, 1176, 1184, 1200, 1208, 1224, 1272, 1312, 1352, 1312, 1272, 1280, 1288, 1288, 1288, 1296, 1296, 1336, 1408, 1496, 1608, 1720, 1832, 1920,
            112, 88, 72, 56, 0, -88, -184, -296, -376, -392, -400, -376, -336, -256, -160, -48, 72, 144, 192, 240, 320, 392, 488, 520, 528, 576, 704, 816, 920, 1016, 1088, 1136, 1160, 1176, 1184, 1184, 1176, 1176, 1176, 1168, 1160, 1168, 1168, 1176, 1184, 1192, 1208, 1240, 1272, 1288, 1272, 1248, 1256, 1272, 1296, 1288, 1288, 1296, 1312, 1392, 1488, 1592, 1696, 1792, 1872,
            80, 72, 48, 16, -24, -120, -216, -312, -384, -400, -400, -392, -352, -280, -176, -48, 64, 136, 176, 208, 288, 376, 440, 480, 512, 568, 704, 824, 936, 1032, 1096, 1144, 1168, 1184, 1184, 1184, 1168, 1168, 1160, 1160, 1160, 1160, 1160, 1168, 1176, 1184, 1192, 1200, 1224, 1240, 1224, 1224, 1232, 1264, 1264, 1272, 1280, 1288, 1288, 1368, 1464, 1568, 1664, 1760, 1832,
            64, 48, 24, -16, -80, -160, -240, -320, -376, -392, -392, -376, -344, -280, -184, -80, 24, 104, 176, 200, 264, 344, 408, 464, 520, 592, 712, 848, 944, 1032, 1104, 1160, 1184, 1200, 1184, 1168, 1160, 1152, 1152, 1152, 1152, 1152, 1152, 1160, 1168, 1176, 1184, 1184, 1192, 1192, 1192, 1192, 1200, 1240, 1256, 1256, 1256, 1256, 1264, 1336, 1432, 1536, 1640, 1720, 1792,
            40, 24, -8, -64, -120, -192, -264, -328, -376, -384, -376, -368, -336, -272, -192, -96, 0, 72, 152, 216, 264, 336, 400, 464, 544, 648, 768, 864, 952, 1048, 1136, 1192, 1216, 1216, 1200, 1176, 1160, 1152, 1144, 1144, 1136, 1136, 1144, 1152, 1160, 1168, 1176, 1176, 1184, 1184, 1184, 1192, 1200, 1208, 1232, 1240, 1232, 1200, 1224, 1296, 1400, 1504, 1592, 1680, 1752,
            0, -24, -64, -112, -168, -224, -288, -336, -376, -376, -376, -352, -312, -256, -176, -104, -8, 88, 168, 240, 304, 360, 416, 472, 584, 688, 800, 896, 960, 1032, 1144, 1208, 1232, 1232, 1216, 1200, 1176, 1152, 1136, 1128, 1128, 1128, 1136, 1144, 1152, 1160, 1168, 1168, 1176, 1184, 1184, 1184, 1184, 1200, 1200, 1200, 1192, 1200, 1216, 1272, 1344, 1464, 1552, 1648, 1712,
            -48, -88, -128, -176, -224, -272, -320, -368, -384, -384, -376, -344, -296, -232, -144, -64, 32, 120, 208, 288, 352, 408, 456, 512, 608, 720, 824, 904, 944, 1064, 1152, 1216, 1248, 1256, 1240, 1216, 1184, 1152, 1120, 1112, 1120, 1120, 1120, 1128, 1144, 1144, 1152, 1168, 1168, 1176, 1184, 1184, 1184, 1192, 1192, 1192, 1192, 1192, 1192, 1232, 1312, 1408, 1504, 1600, 1664,
            -120, -152, -192, -232, -272, -320, -352, -384, -392, -392, -368, -312, -248, -184, -104, -16, 72, 160, 256, 336, 400, 448, 488, 536, 624, 728, 832, 904, 944, 1080, 1184, 1248, 1272, 1304, 1256, 1216, 1176, 1128, 1104, 1088, 1096, 1104, 1104, 1112, 1128, 1136, 1144, 1160, 1168, 1176, 1184, 1184, 1184, 1192, 1200, 1200, 1200, 1192, 1192, 1192, 1264, 1352, 1448, 1536, 1608,
            -184, -208, -240, -280, -312, -344, -376, -392, -400, -392, -352, -288, -208, -144, -64, 24, 112, 200, 288, 368, 432, 472, 512, 552, 632, 728, 816, 880, 944, 1056, 1152, 1224, 1272, 1264, 1272, 1216, 1160, 1104, 1072, 1056, 1056, 1056, 1072, 1080, 1104, 1120, 1136, 1152, 1168, 1176, 1192, 1192, 1192, 1200, 1208, 1208, 1208, 1200, 1192, 1192, 1192, 1304, 1392, 1472, 1560,
            -232, -256, -288, -320, -344, -368, -384, -400, -400, -384, -336, -264, -184, -112, -32, 56, 136, 224, 304, 376, 440, 480, 520, 560, 616, 696, 768, 840, 920, 1008, 1088, 1152, 1200, 1216, 1200, 1168, 1112, 1056, 1024, 1008, 1008, 1008, 1016, 1048, 1080, 1104, 1120, 1136, 1168, 1168, 1184, 1184, 1192, 1200, 1208, 1216, 1208, 1200, 1192, 1192, 1216, 1264, 1328, 1400, 1472,
            -272, -288, -320, -344, -368, -392, -400, -400, -400, -376, -328, -256, -184, -96, -16, 72, 152, 240, 304, 376, 424, 464, 512, 552, 600, 664, 736, 808, 888, 968, 1040, 1096, 1128, 1152, 1144, 1112, 1064, 1000, 960, 944, 976, 952, 944, 976, 1048, 1080, 1136, 1144, 1144, 1152, 1160, 1176, 1184, 1192, 1200, 1208, 1200, 1192, 1192, 1192, 1192, 1232, 1280, 1328, 1384,
            -296, -312, -344, -368, -392, -408, -416, -408, -400, -376, -328, -256, -176, -96, -8, 80, 168, 240, 296, 352, 400, 440, 488, 520, 568, 624, 696, 776, 864, 936, 1008, 1056, 1080, 1088, 1080, 1056, 1016, 952, 920, 880, 888, 872, 872, 920, 976, 1024, 1112, 1136, 1120, 1128, 1128, 1136, 1152, 1168, 1192, 1184, 1192, 1184, 1192, 1192, 1192, 1216, 1240, 1272, 1312,
            -312, -328, -360, -392, -424, -432, -432, -424, -400, -376, -336, -272, -192, -112, -16, 80, 160, 224, 272, 320, 360, 400, 448, 496, 536, 592, 664, 752, 840, 928, 992, 1032, 1048, 1048, 1040, 1008, 968, 920, 880, 880, 880, 872, 880, 888, 936, 976, 1008, 1112, 1088, 1088, 1096, 1104, 1120, 1144, 1160, 1176, 1184, 1184, 1192, 1192, 1192, 1200, 1216, 1232, 1264,
            -336, -352, -384, -424, -440, -456, -448, -432, -408, -384, -352, -296, -224, -136, -40, 56, 136, 200, 240, 280, 312, 360, 408, 456, 512, 568, 648, 728, 824, 920, 984, 1024, 1032, 1024, 1000, 968, 920, 888, 880, 888, 880, 880, 880, 880, 912, 944, 976, 1008, 1040, 1072, 1064, 1072, 1064, 1056, 1024, 1160, 1184, 1184, 1192, 1192, 1192, 1192, 1200, 1216, 1232,
            -360, -376, -408, -440, -464, -472, -456, -440, -416, -392, -368, -320, -248, -168, -72, 16, 104, 160, 200, 232, 264, 304, 368, 432, 496, 560, 632, 712, 792, 872, 952, 992, 1008, 1016, 984, 936, 880, 880, 880, 880, 880, 880, 880, 880, 880, 904, 928, 960, 992, 1024, 1064, 1040, 1024, 1040, 1064, 1104, 1168, 1192, 1192, 1192, 1192, 1192, 1192, 1200, 1216,
            -376, -400, -424, -456, -472, -472, -464, -448, -400, -400, -384, -344, -288, -208, -112, -16, 72, 144, 168, 192, 224, 272, 344, 416, 480, 552, 616, 680, 752, 824, 880, 936, 960, 992, 960, 936, 904, 888, 880, 880, 880, 880, 880, 880, 880, 880, 888, 904, 936, 952, 952, 968, 992, 1016, 1040, 1056, 1096, 1088, 1192, 1192, 1192, 1192, 1192, 1192, 1208,
            -384, -400, -424, -448, -464, -464, -448, -432, -392, -384, -376, -360, -304, -240, -160, -72, 16, 88, 120, 152, 192, 248, 328, 400, 472, 536, 600, 656, 712, 760, 808, 864, 896, 920, 944, 912, 904, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 896, 904, 944, 952, 960, 1016, 1048, 1056, 1064, 1088, 1144, 1176, 1192, 1192, 1192, 1192, 1192,
            -376, -392, -408, -424, -432, -432, -432, -408, -376, -368, -376, -360, -312, -256, -192, -112, -40, 32, 80, 128, 176, 232, 312, 392, 464, 528, 576, 624, 656, 688, 720, 760, 832, 848, 864, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 872, 864, 864, 888, 936, 976, 1032, 1040, 1040, 1048, 1064, 1072, 1144, 1192, 1192, 1192, 1192, 1192,
            -352, -360, -376, -392, -400, -400, -400, -384, -368, -360, -352, -336, -312, -264, -208, -136, -64, 0, 56, 112, 160, 232, 304, 376, 440, 496, 544, 568, 592, 608, 624, 640, 672, 728, 784, 824, 856, 880, 880, 880, 888, 880, 880, 880, 880, 880, 880, 872, 864, 864, 864, 864, 880, 880, 952, 1000, 1024, 1040, 1064, 1112, 1136, 1184, 1184, 1184, 1200,
            -312, -312, -320, -344, -360, -368, -368, -360, -352, -344, -336, -328, -304, -264, -216, -152, -88, -16, 40, 104, 160, 224, 288, 344, 400, 448, 480, 504, 520, 528, 520, 520, 544, 592, 680, 752, 816, 880, 880, 888, 888, 880, 880, 880, 880, 880, 880, 872, 864, 864, 864, 872, 872, 880, 880, 1016, 992, 1016, 1040, 1056, 1088, 1112, 1128, 1160, 1168,
            -272, -264, -280, -288, -328, -344, -352, -344, -336, -328, -320, -312, -296, -272, -224, -176, -104, -40, 32, 88, 152, 216, 256, 304, 344, 384, 408, 424, 440, 440, 424, 400, 400, 472, 560, 672, 768, 864, 880, 896, 920, 912, 888, 880, 880, 880, 880, 880, 880, 872, 872, 872, 880, 880, 880, 880, 1000, 992, 1016, 1032, 1040, 1048, 1064, 1080, 1120,
            -232, -216, -232, -288, -320, -336, -344, -336, -328, -320, -320, -312, -296, -280, -248, -200, -144, -72, 0, 56, 120, 176, 208, 248, 280, 304, 328, 344, 352, 352, 344, 328, 312, 360, 472, 592, 704, 824, 880, 920, 936, 928, 904, 888, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 976, 968, 976, 1000, 1024, 1032, 1048, 1080,
            -208, -216, -240, -288, -328, -344, -344, -336, -320, -312, -304, -312, -304, -296, -272, -240, -192, -128, -56, 0, 56, 112, 160, 176, 200, 224, 240, 256, 264, 264, 264, 264, 264, 304, 392, 512, 632, 760, 856, 928, 952, 952, 928, 904, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 888, 928, 952, 984, 1024, 1024, 1080,
            -192, -224, -264, -312, -344, -360, -352, -336, -320, -312, -304, -312, -320, -312, -304, -280, -248, -200, -136, -72, -24, 24, 64, 104, 128, 144, 152, 176, 184, 192, 192, 208, 224, 272, 344, 456, 584, 696, 800, 880, 920, 936, 920, 904, 880, 872, 872, 872, 872, 872, 880, 880, 880, 880, 880, 880, 880, 880, 880, 880, 920, 960, 1008, 1048, 1104,
            -184, -232, -288, -336, -376, -400, -376, -352, -328, -312, -320, -320, -328, -336, -336, -328, -312, -280, -232, -176, -120, -72, -24, 16, 48, 48, 64, 80, 96, 104, 120, 152, 192, 240, 320, 416, 544, 640, 728, 776, 832, 864, 872, 872, 880, 880, 872, 872, 872, 872, 880, 880, 880, 880, 880, 880, 880, 872, 880, 880, 904, 952, 1016, 1056, 1104,
            -176, -248, -312, -368, -408, -424, -400, -368, -336, -328, -320, -328, -344, -360, -376, -376, -376, -360, -336, -296, -248, -192, -152, -104, -72, -64, -40, 0, 32, 48, 64, 104, 160, 200, 256, 400, 488, 560, 648, 712, 736, 768, 800, 832, 856, 864, 848, 856, 864, 872, 880, 880, 880, 880, 880, 872, 872, 872, 872, 880, 896, 936, 1000, 1048, 1096,
            -168, -256, -336, -392, -432, -440, -424, -392, -352, -336, -328, -336, -352, -376, -408, -424, -432, -432, -424, -400, -360, -320, -272, -232, -184, -152, -112, -64, -24, 8, 32, 40, 112, 192, 224, 304, 392, 464, 528, 592, 632, 672, 712, 728, 760, 808, 824, 840, 864, 880, 888, 880, 888, 880, 880, 872, 872, 872, 872, 880, 896, 936, 1024, 1040, 1072,
            -152, -256, -352, -416, -456, -472, -456, -408, -360, -336, -320, -328, -360, -392, -424, -456, -472, -480, -488, -480, -456, -424, -384, -344, -288, -224, -168, -104, -56, -16, 40, 96, 144, 184, 208, 224, 288, 360, 440, 480, 496, 552, 584, 632, 680, 712, 784, 816, 864, 912, 896, 888, 880, 872, 872, 872, 872, 864, 864, 864, 888, 936, 1008, 1008, 1040,
            -144, -272, -352, -424, -464, -472, -456, -424, -360, -328, -312, -328, -360, -392, -432, -464, -496, -520, -544, -552, -544, -528, -488, -432, -368, -288, -208, -128, -56, 8, 72, 144, 160, 184, 192, 192, 192, 232, 216, 216, 256, 296, 400, 520, 576, 656, 720, 768, 808, 880, 888, 880, 888, 872, 872, 864, 856, 840, 832, 832, 864, 904, 944, 976, 1008,
            -120, -256, -352, -424, -456, -472, -448, -408, -352, -320, -304, -320, -352, -384, -424, -472, -504, -536, -560, -584, -600, -592, -568, -512, -440, -352, -240, -136, -40, 40, 112, 160, 160, 168, 176, 176, 168, 168, 208, 192, 176, 184, 312, 392, 472, 552, 624, 648, 672, 744, 840, 848, 832, 872, 872, 832, 808, 792, 784, 784, 808, 848, 888, 936, 984,
            -88, -240, -352, -432, -472, -472, -440, -384, -328, -296, -288, -312, -336, -376, -424, -464, -504, -536, -568, -600, -632, -640, -616, -560, -496, -408, -272, -144, -24, 96, 152, 152, 168, 176, 176, 176, 160, 168, 168, 168, 168, 168, 232, 296, 392, 432, 496, 464, 480, 624, 680, 744, 768, 800, 800, 784, 760, 744, 728, 728, 728, 768, 832, 896, 968,
            -64, -208, -328, -408, -448, -456, -416, -360, -304, -280, -280, -296, -328, -368, -408, -448, -496, -528, -568, -608, -632, -648, -640, -600, -536, -432, -312, -160, -24, 104, 160, 160, 168, 176, 184, 176, 176, 168, 168, 168, 160, 160, 160, 240, 328, 336, 336, 400, 432, 528, 704, 720, 704, 728, 728, 712, 688, 672, 672, 656, 648, 704, 784, 864, 952,
            -8, -176, -280, -368, -416, -416, -384, -336, -288, -248, -256, -280, -312, -352, -392, -432, -472, -520, -560, -592, -624, -648, -648, -616, -560, -464, -344, -192, -40, 104, 136, 160, 168, 176, 184, 184, 184, 176, 176, 168, 168, 168, 168, 208, 272, 360, 456, 520, 552, 584, 616, 640, 640, 664, 656, 640, 624, 616, 608, 600, 592, 632, 744, 848, 952,
            88, -120, -224, -312, -368, -376, -352, -304, -264, -232, -240, -272, -296, -336, -368, -408, -456, -496, -536, -576, -608, -632, -640, -624, -576, -488, -368, -216, -64, 104, 160, 160, 168, 168, 176, 176, 184, 184, 176, 168, 176, 176, 176, 192, 240, 328, 416, 512, 528, 528, 544, 560, 576, 592, 592, 536, 576, 576, 560, 552, 544, 600, 712, 824, 944,
            192, -8, -168, -272, -344, -352, -320, -280, -248, -224, -232, -264, -288, -320, -352, -384, -424, -464, -504, -544, -584, -616, -632, -624, -584, -504, -392, -240, -64, 96, 160, 160, 160, 160, 168, 168, 176, 168, 176, 168, 176, 192, 176, 168, 240, 320, 392, 480, 528, 528, 528, 528, 520, 528, 528, 544, 528, 528, 528, 528, 536, 600, 696, 816, 944,
            296, 112, -72, -192, -272, -304, -288, -256, -240, -232, -232, -248, -280, -304, -336, -368, -400, -432, -472, -512, -552, -584, -608, -608, -584, -512, -400, -248, -80, 104, 160, 160, 160, 160, 160, 160, 160, 160, 160, 168, 176, 176, 176, 168, 232, 312, 384, 464, 496, 528, 528, 528, 528, 520, 528, 528, 528, 528, 528, 528, 544, 576, 672, 792, 928,
            376, 224, 56, -120, -256, -256, -248, -240, -232, -224, -232, -240, -264, -288, -312, -344, -368, -400, -432, -472, -512, -544, -576, -584, -568, -496, -392, -240, -56, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 168, 168, 168, 160, 224, 304, 376, 440, 488, 528, 528, 528, 528, 528, 528, 528, 528, 528, 528, 528, 544, 568, 648, 776, 912,
            472, 368, 192, 24, -120, -224, -216, -216, -216, -224, -232, -240, -264, -280, -304, -320, -344, -368, -392, -424, -464, -496, -528, -544, -528, -480, -376, -224, -48, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 224, 288, 360, 424, 488, 528, 528, 528, 528, 528, 528, 528, 528, 528, 528, 528, 544, 560, 640, 760, 896,
            528, 448, 296, 136, -8, -120, -168, -192, -200, -208, -224, -248, -264, -280, -296, -320, -320, -336, -352, -384, -408, -440, -472, -488, -480, -432, -328, -176, 8, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 216, 280, 336, 408, 520, 528, 528, 528, 528, 528, 528, 528, 528, 528, 528, 528, 536, 568, 624, 752, 896,
            568, 504, 368, 224, 80, -16, -96, -144, -168, -192, -216, -248, -272, -288, -304, -312, -312, -320, -328, -328, -344, -368, -392, -408, -400, -352, -248, -96, 24, 128, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 200, 256, 320, 392, 520, 528, 528, 528, 528, 520, 520, 520, 520, 520, 528, 528, 536, 552, 656, 776, 912,
            584, 520, 408, 280, 152, 40, -16, -72, -128, -176, -216, -248, -280, -296, -312, -312, -312, -304, -296, -280, -272, -280, -304, -320, -312, -304, -192, -56, 32, 160, 160, 160, 160, 160, 160, 160, 168, 168, 168, 168, 168, 168, 160, 160, 176, 232, 312, 424, 472, 512, 520, 528, 528, 512, 512, 512, 520, 520, 528, 528, 544, 592, 704, 824, 960,
            616, 544, 440, 328, 200, 72, 32, 16, -80, -152, -208, -256, -288, -312, -328, -328, -312, -296, -264, -232, -200, -192, -192, -200, -200, -168, -104, -8, 72, 144, 160, 160, 160, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 168, 184, 240, 328, 408, 480, 536, 552, 560, 536, 504, 496, 496, 504, 512, 528, 528, 568, 656, 768, 888, 1024,
            632, 568, 472, 360, 232, 88, 56, 40, -48, -136, -216, -272, -312, -336, -344, -344, -320, -288, -240, -192, -128, -104, -96, -96, -88, -64, -32, 16, 72, 96, 152, 176, 184, 176, 176, 168, 168, 168, 168, 168, 168, 176, 176, 176, 192, 248, 336, 424, 496, 552, 568, 592, 560, 520, 488, 488, 496, 512, 528, 576, 648, 744, 856, 976, 1120,
            648, 584, 496, 376, 232, 96, 40, 40, -40, -136, -224, -288, -328, -360, -368, -360, -328, -280, -216, -152, -72, -40, -16, 0, 16, 16, 32, 56, 112, 40, 136, 224, 224, 200, 176, 168, 168, 168, 168, 168, 176, 176, 184, 176, 176, 264, 368, 456, 512, 560, 584, 584, 560, 528, 480, 488, 496, 528, 552, 608, 728, 832, 944, 1064, 1200,
            656, 600, 504, 376, 232, 88, 40, 32, -32, -152, -248, -312, -360, -384, -392, -376, -336, -280, -200, -112, -8, 40, 64, 96, 96, 128, 168, 200, 176, 152, 296, 296, 296, 256, 224, 176, 168, 176, 168, 168, 168, 184, 176, 184, 200, 296, 400, 488, 544, 576, 592, 584, 552, 520, 472, 480, 496, 536, 600, 704, 816, 920, 1016, 1136, 1272,
            664, 600, 496, 368, 200, 56, 24, -16, -80, -192, -280, -344, -384, -408, -416, -392, -352, -280, -192, -88, 16, 72, 136, 136, 176, 232, 296, 328, 344, 360, 480, 432, 368, 336, 288, 224, 184, 184, 176, 168, 176, 176, 184, 192, 224, 344, 448, 512, 560, 592, 600, 584, 544, 496, 472, 480, 504, 552, 616, 744, 880, 984, 1080, 1200, 1328,
            664, 592, 472, 336, 176, 32, 0, -64, -136, -240, -328, -376, -416, -432, -440, -416, -360, -280, -184, -72, 56, 112, 168, 192, 248, 328, 392, 424, 480, 488, 528, 456, 448, 416, 352, 264, 208, 192, 184, 176, 176, 176, 200, 256, 328, 416, 496, 560, 584, 608, 608, 584, 536, 488, 472, 480, 512, 560, 632, 784, 904, 1024, 1136, 1240, 1360,
            648, 568, 472, 304, 128, 16, -48, -128, -200, -304, -376, -416, -448, -472, -472, -440, -392, -304, -184, -64, 56, 128, 168, 232, 304, 384, 448, 480, 520, 520, 544, 528, 512, 456, 416, 320, 232, 184, 176, 176, 176, 192, 240, 320, 408, 488, 568, 584, 608, 616, 608, 576, 528, 480, 456, 464, 504, 552, 656, 800, 944, 1040, 1152, 1256, 1376,
            632, 536, 424, 248, 80, -48, -120, -184, -264, -352, -416, -456, -480, -496, -496, -464, -408, -320, -192, -72, 56, 128, 184, 264, 352, 424, 472, 496, 520, 520, 528, 528, 520, 480, 416, 304, 232, 192, 184, 176, 184, 200, 264, 360, 456, 536, 592, 624, 632, 632, 624, 576, 520, 480, 464, 456, 496, 568, 648, 792, 936, 1032, 1136, 1256, 1384,
            600, 488, 360, 184, 24, -96, -192, -240, -320, -408, -456, -480, -504, -512, -504, -472, -416, -328, -208, -80, 48, 136, 192, 304, 400, 464, 496, 512, 520, 528, 536, 536, 528, 448, 344, 248, 192, 168, 168, 176, 184, 216, 312, 408, 504, 576, 632, 664, 672, 656, 624, 576, 520, 488, 464, 464, 504, 552, 656, 776, 896, 1016, 1112, 1232, 1376,
            560, 440, 296, 120, -32, -144, -232, -288, -376, -432, -480, -504, -520, -520, -504, -472, -416, -328, -216, -88, 48, 168, 256, 360, 456, 536, 536, 536, 520, 528, 544, 552, 520, 416, 312, 264, 168, 160, 160, 160, 184, 264, 352, 456, 544, 616, 672, 704, 704, 688, 640, 584, 536, 488, 472, 472, 504, 552, 648, 752, 856, 968, 1080, 1208, 1360,
            520, 392, 240, 64, -80, -208, -288, -328, -408, -464, -504, -520, -528, -528, -504, -472, -416, -328, -216, -88, 64, 216, 432, 432, 512, 536, 536, 528, 528, 528, 544, 552, 512, 424, 344, 280, 232, 168, 168, 160, 192, 304, 400, 496, 584, 648, 696, 720, 720, 696, 656, 592, 536, 488, 472, 480, 504, 544, 632, 728, 824, 928, 1048, 1184, 1328,
            504, 360, 200, 24, -120, -248, -336, -384, -440, -488, -512, -536, -536, -528, -504, -464, -408, -320, -216, -96, 80, 256, 416, 408, 512, 528, 528, 528, 528, 528, 544, 536, 512, 448, 376, 320, 272, 208, 160, 176, 240, 352, 440, 528, 608, 672, 712, 720, 712, 688, 640, 584, 536, 488, 464, 472, 496, 544, 616, 704, 800, 904, 1016, 1152, 1296,
        }};
        const btScalar heightScale = 1;
        const btScalar minHeight;
        const btScalar maxHeight;
        const int upAxis = 2;
        const PHY_ScalarType heightDataType = PHY_FLOAT;
        const bool flipQuadEdges = false;
        btHeightfieldTerrainShape shape;
        const btVector3 origin {53248, -53248, 760};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        const btVector3 scaling {128, 128, 1};
        btCollisionObject object;

        HeightField()
            : minHeight(*std::min_element(heightfieldData.begin(), heightfieldData.end())),
              maxHeight(*std::max_element(heightfieldData.begin(), heightfieldData.end())),
              shape(btHeightfieldTerrainShape{
                  heightStickWidth,
                  heightStickLength,
                  heightfieldData.data(),
                  heightScale,
                  minHeight,
                  maxHeight,
                  upAxis,
                  heightDataType,
                  flipQuadEdges
              })
        {
            shape.setLocalScaling(scaling);
            init(*this);
        }
    };

    struct HeightField2
    {
        static const int heightStickWidth = 65;
        static const int heightStickLength = 65;
        const std::array<btScalar, heightStickWidth * heightStickLength> heightfieldData {{
            504, 360, 200, 24, -120, -248, -336, -384, -440, -488, -512, -536, -536, -528, -504, -464, -408, -320, -216, -96, 80, 256, 416, 408, 512, 528, 528, 528, 528, 528, 544, 536, 512, 448, 376, 320, 272, 208, 160, 176, 240, 352, 440, 528, 608, 672, 712, 720, 712, 688, 640, 584, 536, 488, 464, 472, 496, 544, 616, 704, 800, 904, 1016, 1152, 1296,
            504, 344, 176, 16, -136, -272, -312, -408, -456, -496, -520, -536, -544, -536, -504, -472, -416, -320, -224, -104, 80, 256, 440, 432, 504, 536, 528, 528, 536, 536, 544, 544, 528, 488, 440, 392, 344, 304, 272, 272, 344, 424, 496, 576, 640, 688, 704, 712, 688, 656, 616, 568, 512, 472, 456, 464, 488, 528, 600, 688, 784, 880, 992, 1120, 1256,
            504, 336, 176, 24, -104, -216, -328, -424, -456, -496, -528, -536, -536, -528, -496, -448, -392, -304, -200, -112, 56, 208, 384, 480, 488, 536, 528, 536, 552, 552, 552, 552, 552, 528, 496, 464, 440, 416, 392, 408, 456, 520, 576, 632, 672, 688, 704, 688, 656, 616, 576, 528, 480, 448, 440, 448, 480, 528, 592, 680, 776, 872, 976, 1088, 1224,
            504, 352, 216, 104, 8, -120, -296, -424, -448, -488, -512, -528, -528, -504, -480, -432, -352, -264, -168, -56, 72, 192, 312, 392, 504, 536, 536, 536, 552, 560, 552, 560, 568, 552, 560, 520, 512, 512, 544, 560, 576, 624, 648, 672, 688, 688, 680, 648, 608, 568, 528, 480, 440, 424, 424, 440, 472, 528, 600, 688, 784, 872, 968, 1072, 1192,
            528, 400, 280, 192, 128, 40, -128, -296, -408, -440, -480, -496, -496, -480, -448, -392, -312, -208, -96, 32, 112, 200, 296, 400, 496, 536, 528, 536, 544, 544, 544, 544, 552, 552, 552, 544, 552, 576, 616, 656, 696, 680, 704, 712, 704, 696, 664, 616, 568, 520, 472, 440, 416, 408, 416, 432, 472, 536, 616, 704, 800, 888, 976, 1064, 1176,
            560, 448, 344, 296, 256, 144, 48, -176, -304, -400, -448, -472, -472, -456, -416, -360, -272, -160, -32, 104, 160, 224, 312, 408, 512, 544, 528, 528, 528, 528, 528, 536, 536, 536, 536, 536, 560, 608, 656, 704, 728, 728, 728, 728, 712, 680, 640, 584, 528, 480, 416, 416, 400, 400, 416, 440, 480, 544, 632, 720, 824, 920, 1000, 1088, 1192,
            616, 504, 440, 384, 328, 256, 136, -32, -192, -328, -400, -440, -440, -424, -392, -320, -224, -104, 16, 128, 176, 240, 320, 400, 504, 520, 528, 528, 528, 528, 528, 528, 528, 528, 536, 544, 568, 608, 664, 712, 736, 728, 728, 720, 696, 664, 624, 568, 520, 472, 440, 408, 400, 400, 416, 448, 496, 568, 648, 744, 848, 952, 1048, 1136, 1240,
            688, 592, 520, 464, 408, 336, 232, 72, -96, -248, -352, -400, -416, -400, -360, -296, -192, -72, 32, 112, 176, 240, 312, 392, 456, 536, 528, 528, 528, 528, 528, 528, 536, 536, 536, 536, 560, 600, 648, 680, 704, 704, 696, 688, 672, 648, 616, 576, 528, 480, 440, 416, 400, 408, 424, 464, 512, 584, 672, 776, 888, 1008, 1112, 1208, 1296,
            760, 672, 600, 536, 472, 400, 296, 144, -40, -200, -328, -392, -408, -392, -344, -272, -168, -40, 48, 120, 184, 248, 312, 376, 440, 496, 520, 528, 528, 528, 528, 536, 536, 536, 536, 536, 544, 584, 624, 656, 672, 672, 664, 664, 656, 640, 608, 576, 536, 480, 440, 424, 416, 416, 448, 480, 536, 608, 704, 808, 928, 1056, 1168, 1264, 1344,
            808, 720, 656, 584, 512, 424, 320, 168, -16, -184, -312, -376, -392, -384, -336, -264, -160, -32, 64, 128, 184, 240, 288, 344, 400, 480, 512, 536, 536, 536, 536, 536, 536, 536, 536, 536, 544, 568, 600, 624, 632, 632, 640, 640, 640, 624, 600, 576, 528, 472, 440, 424, 424, 440, 464, 512, 568, 640, 728, 832, 952, 1072, 1192, 1296, 1392,
            808, 744, 680, 600, 520, 416, 296, 136, -48, -208, -320, -376, -392, -384, -336, -272, -168, -32, 56, 128, 176, 216, 248, 288, 336, 400, 448, 472, 480, 536, 536, 512, 536, 544, 536, 536, 544, 552, 568, 584, 592, 600, 616, 624, 624, 616, 592, 560, 504, 464, 440, 432, 432, 456, 488, 536, 600, 672, 752, 856, 976, 1096, 1216, 1320, 1424,
            816, 736, 672, 592, 496, 392, 248, 72, -104, -248, -360, -400, -416, -400, -360, -288, -200, -72, 24, 96, 144, 176, 208, 232, 256, 280, 320, 312, 328, 384, 360, 432, 400, 480, 496, 536, 512, 520, 520, 520, 544, 568, 592, 608, 608, 592, 568, 528, 480, 456, 440, 440, 448, 472, 512, 560, 624, 696, 784, 880, 992, 1112, 1232, 1344, 1456,
            784, 712, 632, 544, 448, 320, 160, -24, -184, -304, -400, -424, -432, -408, -368, -312, -232, -128, -24, 48, 88, 128, 152, 176, 184, 184, 184, 192, 184, 192, 128, 96, 128, 296, 416, 408, 464, 432, 432, 456, 488, 528, 560, 584, 584, 552, 528, 488, 456, 440, 440, 440, 456, 488, 528, 584, 656, 728, 808, 904, 1016, 1136, 1256, 1368, 1480,
            744, 664, 576, 480, 368, 224, 48, -112, -248, -352, -440, -456, -448, -432, -392, -344, -272, -176, -88, -16, 32, 56, 72, 80, 88, 96, 96, 96, 88, 72, 56, 80, 64, 88, 232, 296, 312, 320, 344, 376, 424, 472, 512, 536, 544, 512, 480, 456, 440, 432, 432, 448, 464, 504, 552, 608, 688, 760, 840, 928, 1040, 1160, 1280, 1392, 1512,
            688, 608, 520, 416, 288, 136, -32, -176, -296, -400, -440, -456, -456, -440, -416, -368, -296, -216, -128, -64, -24, -8, 0, 8, 8, 8, 16, 24, 24, 24, 24, 24, 56, 80, 128, 184, 208, 216, 256, 304, 360, 416, 456, 480, 480, 472, 440, 424, 424, 424, 432, 456, 480, 520, 568, 640, 720, 800, 872, 968, 1072, 1192, 1312, 1432, 1552,
            632, 552, 464, 368, 248, 56, -104, -248, -360, -408, -440, -448, -448, -432, -408, -368, -304, -232, -152, -104, -72, -56, -48, -48, -48, -56, -48, -40, -24, -8, 16, 32, 24, 40, 72, 104, 128, 144, 176, 232, 296, 352, 400, 424, 424, 416, 408, 408, 416, 424, 440, 464, 496, 536, 592, 664, 744, 824, 912, 1008, 1112, 1240, 1360, 1488, 1616,
            592, 504, 408, 304, 176, -8, -152, -280, -360, -424, -432, -432, -424, -408, -384, -352, -296, -224, -160, -112, -88, -80, -80, -88, -88, -96, -88, -80, -72, -56, -32, -24, -8, 0, 24, 48, 64, 64, 128, 176, 248, 304, 352, 376, 384, 384, 392, 400, 416, 432, 456, 480, 512, 560, 616, 688, 776, 864, 960, 1056, 1168, 1288, 1416, 1552, 1696,
            568, 480, 376, 256, 128, -40, -192, -312, -376, -408, -416, -416, -400, -376, -344, -304, -256, -200, -144, -104, -88, -80, -88, -96, -96, -104, -104, -104, -96, -80, -64, -56, -48, -32, -16, 8, 40, 88, 144, 192, 240, 272, 320, 344, 360, 368, 384, 400, 424, 448, 472, 504, 544, 592, 656, 736, 816, 912, 1000, 1104, 1216, 1344, 1480, 1632, 1784,
            552, 464, 344, 248, 120, -96, -216, -320, -384, -408, -392, -376, -344, -312, -272, -248, -184, -136, -96, -64, -56, -56, -56, -72, -80, -104, -104, -104, -96, -88, -80, -72, -56, -48, -32, 0, 48, 112, 168, 224, 264, 280, 312, 336, 352, 368, 392, 416, 440, 472, 496, 536, 576, 632, 712, 784, 872, 960, 1056, 1160, 1272, 1400, 1552, 1704, 1872,
            568, 464, 352, 248, 72, -112, -240, -336, -384, -392, -360, -320, -264, -208, -160, -120, -72, -40, -16, 0, 16, 16, 0, -16, -32, -64, -72, -80, -72, -72, -64, -64, -56, -40, -16, 24, 80, 152, 224, 264, 280, 304, 328, 344, 368, 384, 408, 432, 464, 496, 528, 576, 632, 696, 768, 848, 928, 1016, 1104, 1208, 1328, 1456, 1608, 1776, 1952,
            544, 448, 344, 224, 32, -128, -280, -384, -392, -368, -328, -256, -176, -96, -24, 40, 64, 72, 80, 96, 104, 104, 88, 64, 32, 0, -16, -24, -24, -24, -24, -40, -32, 16, 40, 80, 144, 200, 280, 296, 320, 336, 352, 368, 384, 408, 432, 456, 488, 520, 568, 616, 680, 752, 832, 912, 984, 1064, 1152, 1248, 1376, 1512, 1664, 1840, 2016,
            544, 456, 320, 176, 0, -160, -288, -384, -384, -336, -264, -168, -64, 40, 168, 224, 216, 184, 184, 192, 200, 192, 168, 144, 120, 88, 72, 56, 48, 40, 24, 48, 64, 96, 128, 168, 216, 272, 312, 336, 360, 376, 392, 400, 416, 432, 456, 488, 512, 552, 608, 672, 744, 824, 904, 976, 1048, 1120, 1208, 1312, 1424, 1568, 1728, 1896, 2080,
            544, 440, 296, 112, -56, -192, -296, -368, -368, -320, -216, -72, 56, 184, 280, 320, 304, 280, 272, 288, 296, 288, 264, 240, 216, 184, 168, 152, 136, 128, 136, 144, 168, 192, 224, 256, 296, 328, 368, 384, 400, 408, 424, 432, 448, 464, 488, 520, 552, 600, 664, 744, 824, 904, 976, 1040, 1112, 1184, 1272, 1376, 1496, 1624, 1776, 1952, 2136,
            520, 400, 240, 40, -120, -240, -312, -352, -344, -272, -120, 40, 184, 280, 384, 424, 408, 360, 336, 344, 352, 352, 344, 328, 312, 288, 256, 240, 224, 224, 232, 240, 264, 288, 312, 336, 360, 392, 416, 432, 440, 448, 456, 464, 480, 504, 528, 560, 608, 664, 744, 824, 904, 984, 1048, 1120, 1184, 1264, 1344, 1448, 1568, 1704, 1848, 2008, 2192,
            504, 328, 128, -56, -184, -288, -352, -360, -328, -200, -40, 128, 272, 392, 496, 528, 504, 448, 416, 408, 408, 408, 400, 392, 376, 352, 336, 320, 312, 304, 312, 328, 344, 368, 392, 408, 424, 448, 464, 472, 472, 480, 488, 496, 512, 536, 576, 616, 672, 744, 832, 920, 1000, 1064, 1128, 1192, 1264, 1344, 1432, 1536, 1656, 1784, 1928, 2080, 2248,
            416, 216, 16, -152, -272, -320, -360, -352, -288, -160, 32, 256, 344, 480, 568, 576, 560, 504, 464, 456, 440, 440, 440, 440, 424, 416, 400, 392, 384, 392, 392, 400, 416, 432, 440, 456, 472, 488, 504, 504, 504, 504, 512, 520, 544, 584, 640, 696, 768, 848, 936, 1016, 1088, 1152, 1208, 1280, 1352, 1432, 1528, 1624, 1736, 1864, 2000, 2152, 2304,
            312, 104, -72, -192, -296, -360, -384, -360, -296, -168, 72, 248, 400, 520, 576, 568, 576, 536, 512, 496, 480, 480, 472, 464, 464, 464, 456, 456, 456, 464, 464, 464, 472, 480, 496, 504, 512, 528, 544, 544, 536, 536, 536, 552, 576, 640, 712, 792, 872, 960, 1032, 1104, 1168, 1232, 1296, 1360, 1440, 1528, 1616, 1720, 1832, 1952, 2080, 2224, 2376,
            224, -8, -120, -232, -320, -376, -384, -352, -272, -152, 88, 280, 432, 536, 560, 592, 600, 600, 576, 552, 528, 504, 496, 496, 496, 504, 512, 512, 512, 520, 528, 528, 528, 536, 536, 544, 560, 576, 576, 584, 576, 576, 576, 592, 632, 712, 800, 896, 992, 1064, 1128, 1184, 1240, 1304, 1376, 1448, 1536, 1624, 1712, 1816, 1920, 2032, 2160, 2296, 2440,
            168, -56, -152, -256, -344, -384, -392, -352, -272, -104, 96, 296, 432, 512, 584, 640, 672, 680, 656, 624, 584, 560, 544, 536, 536, 536, 552, 560, 568, 576, 584, 584, 584, 584, 584, 592, 600, 616, 616, 624, 624, 624, 632, 656, 712, 784, 896, 1008, 1096, 1152, 1200, 1248, 1312, 1384, 1456, 1544, 1632, 1720, 1816, 1912, 2008, 2120, 2240, 2376, 2520,
            160, -16, -160, -272, -352, -392, -392, -352, -280, -160, 96, 280, 424, 520, 616, 704, 760, 776, 752, 704, 656, 616, 584, 576, 576, 584, 592, 608, 616, 624, 632, 632, 624, 624, 632, 632, 648, 656, 664, 672, 680, 696, 712, 752, 808, 896, 1000, 1104, 1168, 1216, 1264, 1312, 1384, 1456, 1544, 1640, 1728, 1824, 1912, 2000, 2096, 2200, 2328, 2464, 2608,
            176, -32, -152, -272, -344, -384, -384, -352, -280, -160, 72, 264, 408, 536, 656, 760, 840, 872, 848, 792, 736, 688, 648, 640, 640, 648, 656, 672, 680, 680, 680, 680, 680, 680, 680, 688, 704, 712, 728, 744, 760, 776, 808, 856, 920, 984, 1096, 1176, 1224, 1272, 1320, 1384, 1456, 1536, 1632, 1728, 1824, 1920, 2000, 2088, 2176, 2288, 2416, 2560, 2704,
            208, 0, -136, -248, -328, -368, -368, -336, -272, -160, 72, 272, 400, 544, 680, 808, 904, 944, 928, 880, 824, 776, 736, 720, 720, 728, 744, 744, 752, 752, 744, 736, 736, 736, 744, 752, 768, 784, 808, 832, 864, 912, 960, 1016, 1064, 1136, 1200, 1248, 1288, 1336, 1384, 1456, 1528, 1616, 1720, 1816, 1912, 2000, 2096, 2168, 2256, 2368, 2504, 2656, 2784,
            240, 40, -144, -224, -296, -336, -336, -304, -296, -136, 72, 256, 400, 552, 696, 824, 928, 984, 984, 936, 904, 864, 840, 832, 824, 832, 840, 840, 840, 832, 824, 808, 808, 816, 824, 840, 856, 872, 896, 928, 984, 1040, 1120, 1184, 1216, 1256, 1296, 1336, 1368, 1416, 1464, 1528, 1600, 1696, 1792, 1888, 1984, 2088, 2160, 2240, 2336, 2448, 2592, 2728, 2872,
            264, 64, -96, -216, -288, -328, -320, -280, -200, -96, 112, 272, 416, 560, 696, 824, 920, 992, 1016, 1008, 976, 960, 944, 936, 936, 944, 952, 952, 944, 928, 912, 904, 888, 904, 928, 944, 960, 984, 1008, 1048, 1112, 1192, 1272, 1304, 1336, 1368, 1400, 1432, 1464, 1496, 1544, 1600, 1672, 1760, 1848, 1944, 2048, 2144, 2224, 2304, 2392, 2528, 2672, 2824, 2936,
            272, 80, -80, -200, -272, -296, -280, -224, -128, -8, 168, 320, 456, 576, 696, 808, 904, 976, 1024, 1048, 1048, 1048, 1048, 1048, 1048, 1056, 1064, 1064, 1056, 1040, 1024, 1008, 1000, 1008, 1024, 1056, 1080, 1096, 1120, 1160, 1232, 1320, 1368, 1400, 1440, 1480, 1504, 1528, 1560, 1584, 1616, 1664, 1728, 1816, 1904, 2000, 2104, 2200, 2288, 2344, 2440, 2600, 2760, 2896, 3024,
            248, 64, -88, -192, -248, -272, -240, -168, -72, 56, 232, 392, 504, 608, 704, 800, 888, 968, 1032, 1080, 1104, 1128, 1136, 1144, 1152, 1160, 1168, 1176, 1168, 1160, 1144, 1128, 1120, 1112, 1144, 1192, 1232, 1256, 1264, 1304, 1360, 1408, 1448, 1488, 1536, 1568, 1600, 1624, 1648, 1664, 1688, 1720, 1768, 1840, 1920, 2008, 2112, 2208, 2296, 2384, 2520, 2696, 2848, 3000, 3096,
            200, 24, -112, -200, -240, -248, -208, -128, -16, 120, 304, 456, 552, 640, 720, 800, 880, 968, 1056, 1128, 1176, 1192, 1216, 1232, 1240, 1256, 1264, 1272, 1288, 1272, 1264, 1248, 1240, 1256, 1288, 1328, 1360, 1400, 1392, 1416, 1464, 1480, 1520, 1568, 1616, 1648, 1680, 1712, 1728, 1736, 1752, 1768, 1800, 1856, 1928, 2016, 2120, 2224, 2336, 2472, 2640, 2800, 2960, 3072, 3152,
            136, -24, -144, -208, -248, -232, -176, -88, 32, 176, 360, 496, 584, 672, 744, 816, 896, 984, 1072, 1152, 1208, 1248, 1280, 1304, 1320, 1336, 1352, 1368, 1376, 1376, 1368, 1360, 1360, 1368, 1400, 1456, 1496, 1504, 1480, 1496, 1520, 1544, 1576, 1624, 1680, 1712, 1752, 1784, 1800, 1800, 1808, 1816, 1832, 1864, 1944, 2040, 2160, 2288, 2440, 2600, 2776, 2936, 3072, 3168, 3240,
            80, -72, -176, -240, -256, -232, -160, -56, 80, 224, 400, 528, 632, 720, 784, 848, 928, 1016, 1104, 1176, 1240, 1280, 1320, 1344, 1368, 1384, 1416, 1440, 1456, 1464, 1464, 1456, 1456, 1464, 1480, 1528, 1536, 1536, 1536, 1544, 1560, 1584, 1624, 1672, 1720, 1768, 1808, 1840, 1856, 1856, 1856, 1856, 1872, 1888, 1952, 2048, 2200, 2368, 2552, 2752, 2928, 3104, 3240, 3304, 3328,
            16, -128, -232, -280, -280, -240, -152, -24, 120, 296, 440, 600, 720, 776, 840, 904, 968, 1048, 1136, 1208, 1272, 1328, 1360, 1384, 1408, 1440, 1464, 1496, 1520, 1536, 1544, 1536, 1536, 1536, 1560, 1544, 1544, 1544, 1560, 1576, 1592, 1616, 1656, 1704, 1760, 1800, 1848, 1880, 1896, 1904, 1904, 1912, 1920, 1944, 2008, 2120, 2280, 2480, 2728, 2936, 3072, 3224, 3344, 3440, 3464,
            -48, -192, -280, -320, -312, -248, -144, 0, 160, 344, 528, 680, 808, 856, 904, 952, 1016, 1088, 1176, 1256, 1328, 1376, 1408, 1432, 1456, 1488, 1512, 1552, 1576, 1600, 1608, 1608, 1592, 1576, 1568, 1544, 1544, 1552, 1568, 1592, 1616, 1640, 1672, 1720, 1776, 1824, 1872, 1904, 1928, 1944, 1952, 1960, 1976, 2008, 2080, 2200, 2368, 2584, 2816, 3032, 3216, 3352, 3488, 3552, 3616,
            -120, -248, -328, -352, -328, -256, -136, 16, 192, 400, 592, 752, 880, 952, 992, 1032, 1072, 1144, 1224, 1304, 1376, 1432, 1472, 1496, 1512, 1536, 1568, 1600, 1632, 1648, 1656, 1656, 1632, 1608, 1576, 1544, 1536, 1552, 1576, 1600, 1624, 1648, 1680, 1728, 1776, 1832, 1880, 1920, 1952, 1976, 2000, 2016, 2048, 2080, 2152, 2272, 2456, 2672, 2904, 3120, 3288, 3424, 3536, 3592, 3656,
            -192, -304, -368, -384, -352, -264, -136, 32, 224, 456, 640, 800, 936, 1032, 1088, 1112, 1152, 1208, 1288, 1368, 1440, 1496, 1544, 1568, 1584, 1600, 1616, 1640, 1672, 1696, 1696, 1688, 1656, 1616, 1584, 1552, 1544, 1560, 1584, 1608, 1632, 1648, 1680, 1720, 1776, 1832, 1888, 1936, 1976, 2016, 2048, 2080, 2120, 2160, 2232, 2368, 2552, 2760, 2984, 3200, 3360, 3480, 3576, 3624, 3680,
            -272, -360, -408, -408, -368, -272, -136, 40, 240, 488, 688, 840, 968, 1112, 1168, 1192, 1216, 1280, 1352, 1424, 1504, 1568, 1608, 1632, 1648, 1656, 1672, 1688, 1712, 1728, 1728, 1704, 1664, 1632, 1584, 1560, 1552, 1568, 1592, 1616, 1632, 1648, 1672, 1712, 1768, 1832, 1896, 1952, 2008, 2064, 2112, 2160, 2200, 2256, 2336, 2488, 2664, 2872, 3080, 3264, 3416, 3528, 3616, 3656, 3704,
            -336, -416, -448, -440, -384, -280, -136, 48, 248, 480, 664, 832, 976, 1096, 1168, 1232, 1288, 1344, 1416, 1488, 1568, 1640, 1680, 1704, 1712, 1720, 1720, 1728, 1744, 1760, 1760, 1728, 1680, 1624, 1584, 1560, 1560, 1568, 1600, 1624, 1640, 1656, 1664, 1704, 1760, 1824, 1896, 1976, 2056, 2128, 2192, 2248, 2296, 2368, 2480, 2616, 2792, 2992, 3192, 3360, 3496, 3592, 3656, 3696, 3736,
            -384, -464, -496, -480, -416, -304, -152, 40, 248, 480, 648, 808, 960, 1072, 1168, 1256, 1320, 1376, 1456, 1536, 1640, 1712, 1752, 1760, 1776, 1776, 1784, 1784, 1784, 1776, 1768, 1728, 1664, 1608, 1576, 1560, 1552, 1568, 1600, 1632, 1664, 1680, 1688, 1696, 1760, 1832, 1920, 2016, 2120, 2216, 2296, 2344, 2408, 2504, 2608, 2752, 2928, 3128, 3320, 3488, 3600, 3672, 3712, 3744, 3776,
            -424, -496, -528, -512, -432, -320, -160, 0, 248, 432, 616, 776, 928, 1048, 1160, 1264, 1328, 1400, 1472, 1568, 1680, 1752, 1784, 1816, 1832, 1856, 1856, 1856, 1832, 1800, 1760, 1704, 1640, 1600, 1568, 1544, 1544, 1560, 1592, 1632, 1672, 1704, 1728, 1728, 1752, 1824, 1952, 2088, 2216, 2344, 2432, 2488, 2552, 2632, 2744, 2896, 3080, 3280, 3472, 3616, 3712, 3760, 3784, 3800, 3832,
            -456, -544, -576, -552, -472, -344, -184, 0, 208, 400, 576, 744, 904, 1040, 1160, 1256, 1336, 1416, 1488, 1568, 1672, 1752, 1816, 1864, 1912, 1944, 1960, 1944, 1912, 1864, 1800, 1720, 1648, 1568, 1552, 1528, 1512, 1536, 1584, 1640, 1696, 1728, 1752, 1768, 1816, 1920, 2064, 2208, 2352, 2496, 2592, 2640, 2680, 2760, 2880, 3032, 3224, 3424, 3608, 3752, 3824, 3848, 3848, 3848, 3872,
            -448, -536, -568, -552, -472, -344, -184, 0, 184, 360, 552, 736, 896, 1048, 1176, 1280, 1368, 1448, 1512, 1568, 1640, 1760, 1864, 1952, 2024, 2072, 2088, 2080, 2024, 1952, 1864, 1760, 1656, 1576, 1536, 1520, 1528, 1552, 1584, 1648, 1696, 1736, 1768, 1816, 1904, 2040, 2192, 2336, 2472, 2616, 2688, 2744, 2800, 2880, 2992, 3152, 3336, 3544, 3720, 3856, 3912, 3920, 3888, 3872, 3896,
            -448, -496, -560, -536, -464, -336, -192, 0, 184, 336, 552, 744, 920, 1080, 1224, 1336, 1440, 1520, 1576, 1632, 1688, 1824, 1936, 2048, 2136, 2200, 2216, 2200, 2144, 2056, 1952, 1840, 1728, 1592, 1560, 1568, 1584, 1608, 1624, 1680, 1720, 1776, 1824, 1904, 2032, 2176, 2328, 2464, 2592, 2704, 2792, 2848, 2904, 2976, 3088, 3240, 3424, 3616, 3792, 3904, 3952, 3936, 3904, 3880, 3888,
            -368, -464, -496, -496, -472, -344, -176, 16, 192, 392, 592, 800, 984, 1160, 1304, 1432, 1536, 1616, 1672, 1744, 1824, 1928, 2040, 2144, 2256, 2312, 2336, 2312, 2248, 2152, 2040, 1928, 1816, 1712, 1656, 1656, 1672, 1656, 1704, 1760, 1800, 1872, 1944, 2040, 2168, 2312, 2456, 2584, 2704, 2832, 2920, 2984, 2984, 3040, 3152, 3296, 3472, 3648, 3800, 3904, 3936, 3912, 3880, 3856, 3864,
            -320, -408, -456, -456, -432, -304, -144, 40, 232, 440, 656, 872, 1080, 1256, 1416, 1560, 1672, 1744, 1808, 1880, 1960, 2048, 2152, 2248, 2352, 2408, 2432, 2400, 2336, 2240, 2120, 2008, 1896, 1808, 1744, 1760, 1720, 1752, 1816, 1880, 1928, 2000, 2088, 2192, 2312, 2448, 2576, 2688, 2792, 2896, 2968, 3016, 3024, 3080, 3176, 3312, 3472, 3632, 3760, 3848, 3872, 3856, 3824, 3808, 3816,
            -280, -368, -408, -416, -384, -264, -104, 80, 280, 496, 720, 944, 1168, 1360, 1520, 1672, 1792, 1888, 1952, 2024, 2096, 2168, 2256, 2336, 2424, 2480, 2488, 2464, 2392, 2304, 2192, 2080, 1976, 1872, 1784, 1768, 1824, 1888, 1960, 2024, 2088, 2168, 2256, 2360, 2464, 2576, 2688, 2784, 2888, 2960, 3008, 3032, 3048, 3088, 3176, 3288, 3424, 3568, 3680, 3752, 3776, 3768, 3744, 3736, 3744,
            -248, -328, -376, -376, -352, -232, -64, 128, 328, 544, 776, 1024, 1256, 1472, 1648, 1800, 1936, 2016, 2104, 2160, 2216, 2280, 2344, 2408, 2480, 2520, 2504, 2496, 2440, 2352, 2248, 2144, 2056, 1968, 1880, 1928, 1976, 2040, 2104, 2192, 2264, 2352, 2440, 2536, 2632, 2720, 2816, 2896, 2968, 3016, 3040, 3048, 3032, 3080, 3152, 3248, 3360, 3472, 3576, 3640, 3664, 3664, 3656, 3656, 3656,
            -232, -312, -344, -352, -320, -176, -16, 168, 368, 592, 824, 1072, 1312, 1528, 1712, 1888, 2040, 2128, 2216, 2280, 2336, 2384, 2424, 2464, 2504, 2528, 2528, 2504, 2456, 2392, 2304, 2224, 2152, 2080, 2048, 2088, 2128, 2184, 2248, 2328, 2416, 2512, 2616, 2712, 2800, 2880, 2952, 3024, 3072, 3104, 3104, 3088, 3064, 3048, 3128, 3200, 3288, 3384, 3464, 3528, 3560, 3568, 3568, 3568, 3592,
            -240, -304, -336, -328, -272, -144, 16, 208, 408, 632, 864, 1112, 1352, 1584, 1784, 1952, 2112, 2232, 2336, 2408, 2472, 2480, 2496, 2520, 2536, 2544, 2544, 2520, 2480, 2432, 2368, 2312, 2264, 2216, 2224, 2248, 2272, 2312, 2376, 2448, 2544, 2648, 2760, 2872, 2960, 3040, 3104, 3160, 3200, 3208, 3200, 3160, 3120, 3088, 3080, 3160, 3224, 3296, 3368, 3432, 3464, 3480, 3480, 3488, 3520,
            -248, -304, -328, -304, -248, -136, 48, 240, 448, 672, 904, 1144, 1384, 1624, 1824, 2000, 2160, 2296, 2392, 2472, 2528, 2536, 2544, 2552, 2560, 2568, 2560, 2544, 2520, 2488, 2448, 2408, 2376, 2368, 2368, 2376, 2408, 2424, 2472, 2552, 2656, 2776, 2904, 3024, 3120, 3192, 3232, 3272, 3304, 3304, 3280, 3224, 3176, 3136, 3112, 3120, 3160, 3200, 3304, 3360, 3384, 3400, 3408, 3416, 3440,
            -256, -296, -304, -288, -224, -112, 64, 264, 480, 712, 944, 1192, 1432, 1664, 1864, 2056, 2232, 2376, 2480, 2536, 2584, 2568, 2576, 2584, 2584, 2584, 2584, 2576, 2560, 2544, 2520, 2512, 2496, 2480, 2504, 2496, 2488, 2496, 2544, 2624, 2728, 2848, 2976, 3088, 3192, 3272, 3336, 3384, 3368, 3360, 3328, 3280, 3232, 3184, 3144, 3104, 3152, 3192, 3248, 3296, 3320, 3336, 3344, 3336, 3328,
            -256, -288, -296, -264, -200, -80, 80, 296, 520, 752, 992, 1232, 1472, 1696, 1896, 2072, 2240, 2376, 2480, 2536, 2576, 2592, 2592, 2592, 2600, 2608, 2608, 2608, 2600, 2608, 2592, 2576, 2560, 2568, 2552, 2536, 2528, 2544, 2592, 2672, 2768, 2888, 3016, 3128, 3232, 3312, 3376, 3408, 3432, 3416, 3376, 3320, 3288, 3240, 3176, 3128, 3192, 3208, 3248, 3272, 3288, 3288, 3272, 3232, 3200,
            -264, -272, -264, -232, -152, -48, 104, 328, 560, 800, 1040, 1288, 1512, 1712, 1912, 2072, 2232, 2368, 2472, 2536, 2584, 2592, 2600, 2600, 2616, 2624, 2632, 2648, 2640, 2640, 2632, 2608, 2608, 2584, 2560, 2552, 2560, 2584, 2624, 2688, 2792, 2912, 3032, 3144, 3240, 3328, 3384, 3408, 3432, 3432, 3432, 3392, 3328, 3264, 3200, 3224, 3216, 3232, 3256, 3272, 3264, 3240, 3192, 3136, 3088,
            -264, -256, -240, -208, -128, -24, 168, 376, 608, 848, 1088, 1320, 1536, 1744, 1920, 2088, 2240, 2376, 2456, 2520, 2568, 2584, 2592, 2608, 2624, 2632, 2640, 2656, 2664, 2648, 2672, 2648, 2600, 2584, 2568, 2576, 2592, 2608, 2632, 2696, 2792, 2912, 3024, 3136, 3248, 3336, 3392, 3416, 3440, 3464, 3456, 3424, 3344, 3280, 3208, 3240, 3240, 3248, 3272, 3280, 3248, 3192, 3128, 3056, 3008,
            -264, -240, -208, -160, -64, 48, 216, 432, 656, 888, 1128, 1352, 1592, 1768, 1920, 2080, 2224, 2344, 2440, 2504, 2544, 2576, 2592, 2608, 2624, 2632, 2648, 2648, 2656, 2656, 2648, 2624, 2600, 2592, 2576, 2584, 2592, 2616, 2640, 2696, 2784, 2880, 2992, 3112, 3224, 3320, 3384, 3432, 3464, 3472, 3456, 3384, 3320, 3248, 3176, 3240, 3240, 3264, 3288, 3312, 3256, 3160, 3080, 3024, 2992,
            -264, -224, -176, -96, 0, 128, 296, 496, 704, 920, 1136, 1344, 1560, 1752, 1920, 2072, 2216, 2336, 2432, 2488, 2536, 2576, 2600, 2616, 2632, 2640, 2648, 2648, 2648, 2640, 2624, 2600, 2584, 2584, 2576, 2584, 2600, 2624, 2656, 2704, 2760, 2832, 2928, 3040, 3152, 3256, 3336, 3408, 3456, 3456, 3416, 3352, 3296, 3216, 3184, 3192, 3224, 3248, 3272, 3344, 3240, 3136, 3056, 3000, 2976,
            -256, -208, -144, -56, 56, 192, 360, 552, 744, 944, 1144, 1336, 1528, 1704, 1872, 2040, 2184, 2296, 2408, 2488, 2536, 2584, 2624, 2640, 2656, 2656, 2656, 2648, 2640, 2624, 2608, 2584, 2560, 2560, 2584, 2600, 2608, 2632, 2664, 2704, 2744, 2800, 2864, 2944, 3048, 3160, 3256, 3368, 3448, 3472, 3432, 3368, 3272, 3208, 3184, 3184, 3240, 3288, 3328, 3336, 3248, 3112, 3032, 2968, 2960,
            -248, -184, -104, 0, 120, 256, 408, 592, 776, 960, 1136, 1312, 1496, 1664, 1824, 1984, 2128, 2264, 2368, 2464, 2536, 2592, 2640, 2672, 2688, 2688, 2672, 2648, 2632, 2608, 2584, 2560, 2544, 2560, 2592, 2616, 2608, 2656, 2680, 2704, 2728, 2760, 2800, 2864, 2960, 3072, 3184, 3304, 3400, 3416, 3360, 3296, 3232, 3176, 3168, 3184, 3232, 3272, 3304, 3296, 3184, 3088, 3000, 2952, 2944,
        }};
        const btScalar heightScale = 1;
        const btScalar minHeight;
        const btScalar maxHeight;
        const int upAxis = 2;
        const PHY_ScalarType heightDataType = PHY_FLOAT;
        const bool flipQuadEdges = false;
        btHeightfieldTerrainShape shape;
        const btVector3 origin {53248, -45056, 1688};
        const btTransform transform {btMatrix3x3::getIdentity(), origin};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        const btVector3 scaling {128, 128, 1};
        btCollisionObject object;

        HeightField2()
            : minHeight(*std::min_element(heightfieldData.begin(), heightfieldData.end())),
              maxHeight(*std::max_element(heightfieldData.begin(), heightfieldData.end())),
              shape(btHeightfieldTerrainShape{
                  heightStickWidth,
                  heightStickLength,
                  heightfieldData.data(),
                  heightScale,
                  minHeight,
                  maxHeight,
                  upAxis,
                  heightDataType,
                  flipQuadEdges
              })
        {
            shape.setLocalScaling(scaling);
            init(*this);
        }
    };

    struct Player
    {
        const btVector3 halfExtents {29.279994964599609375, 28.4799976348876953125, 66.5};
        const btVector3 origin {56607.9765625, -49244.609375, 726.5640869140625};
        const btTransform transform {
            btMatrix3x3(
                0.2631151676177978515625, -0.964764416217803955078125, -0,
                0.964764416217803955078125, 0.2631151676177978515625, 0,
                0, -0, 1
            ),
            origin
        };
        btCapsuleShapeZ shape {halfExtents.x(), 2 * halfExtents.z() - 2 * halfExtents.x()};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_KINEMATIC_OBJECT;
        btCollisionObject object;

        Player() { init(*this); }
    };

    struct Mesh3
    {
        const std::vector<std::vector<btVector3>> triangles {{
            {btVector3(3.165980815887451171875, -13.70164394378662109375, -89.81629180908203125), btVector3(3.6294574737548828125, 19.6229343414306640625, -0.18219268321990966796875), btVector3(9.03495025634765625, 16.5020732879638671875, -0.182192966341972351074219)},
            {btVector3(3.165980815887451171875, -13.70164394378662109375, -89.81629180908203125), btVector3(-7.08998012542724609375, -7.78036403656005859375, -89.81629180908203125), btVector3(3.6294574737548828125, 19.6229343414306640625, -0.18219268321990966796875)},
            {btVector3(-7.08998012542724609375, -7.78036403656005859375, -89.81629180908203125), btVector3(3.6294574737548828125, 13.38121128082275390625, -0.182193234562873840332031), btVector3(3.6294574737548828125, 19.6229343414306640625, -0.18219268321990966796875)},
            {btVector3(-7.08998012542724609375, -7.78036403656005859375, -89.81629180908203125), btVector3(-7.0899791717529296875, -19.6229267120361328125, -89.81629180908203125), btVector3(3.6294574737548828125, 13.38121128082275390625, -0.182193234562873840332031)},
            {btVector3(-7.0899791717529296875, -19.6229267120361328125, -89.81629180908203125), btVector3(9.03495025634765625, 16.5020732879638671875, -0.182192966341972351074219), btVector3(3.6294574737548828125, 13.38121128082275390625, -0.182193234562873840332031)},
            {btVector3(-7.0899791717529296875, -19.6229267120361328125, -89.81629180908203125), btVector3(3.165980815887451171875, -13.70164394378662109375, -89.81629180908203125), btVector3(9.03495025634765625, 16.5020732879638671875, -0.182192966341972351074219)},
            {btVector3(-5.458113193511962890625, 4.863862514495849609375, 89.8333587646484375), btVector3(-7.778857707977294921875, 6.2037448883056640625, 89.8333587646484375), btVector3(-9.03495025634765625, 4.7086391448974609375, 89.81629180908203125)},
            {btVector3(-7.778857707977294921875, 6.2037448883056640625, 89.8333587646484375), btVector3(-7.778858184814453125, 3.5239799022674560546875, 89.8333587646484375), btVector3(-9.03495025634765625, 4.7086391448974609375, 89.81629180908203125)},
            {btVector3(-7.778858184814453125, 3.5239799022674560546875, 89.8333587646484375), btVector3(-5.458113193511962890625, 4.863862514495849609375, 89.8333587646484375), btVector3(-9.03495025634765625, 4.7086391448974609375, 89.81629180908203125)},
            {btVector3(-5.458113193511962890625, 4.863862514495849609375, 89.8333587646484375), btVector3(3.6294574737548828125, 19.6229343414306640625, -0.18219268321990966796875), btVector3(-7.778857707977294921875, 6.2037448883056640625, 89.8333587646484375)},
            {btVector3(9.03495025634765625, 16.5020732879638671875, -0.182192966341972351074219), btVector3(3.6294574737548828125, 19.6229343414306640625, -0.18219268321990966796875), btVector3(-5.458113193511962890625, 4.863862514495849609375, 89.8333587646484375)},
            {btVector3(-7.778857707977294921875, 6.2037448883056640625, 89.8333587646484375), btVector3(3.6294574737548828125, 13.38121128082275390625, -0.182193234562873840332031), btVector3(-7.778858184814453125, 3.5239799022674560546875, 89.8333587646484375)},
            {btVector3(3.6294574737548828125, 19.6229343414306640625, -0.18219268321990966796875), btVector3(3.6294574737548828125, 13.38121128082275390625, -0.182193234562873840332031), btVector3(-7.778857707977294921875, 6.2037448883056640625, 89.8333587646484375)},
            {btVector3(-7.778858184814453125, 3.5239799022674560546875, 89.8333587646484375), btVector3(9.03495025634765625, 16.5020732879638671875, -0.182192966341972351074219), btVector3(-5.458113193511962890625, 4.863862514495849609375, 89.8333587646484375)},
            {btVector3(3.6294574737548828125, 13.38121128082275390625, -0.182193234562873840332031), btVector3(9.03495025634765625, 16.5020732879638671875, -0.182192966341972351074219), btVector3(-7.778858184814453125, 3.5239799022674560546875, 89.8333587646484375)},
        }};
        const btVector3 origin {55718.6875, -48041.33203125, 495.931671142578125};
        const btTransform transform {
            btMatrix3x3 {
                0.825336039066314697265625, -0.56464183330535888671875, -0,
                0.56464183330535888671875, 0.825336039066314697265625, 0,
                0, -0, 1
            },
            origin,
        };
        btTriangleMesh mesh = makeTriangleMesh(triangles);
        btBvhTriangleMeshShape child {&mesh, true};
        const btVector3 scaling {1, 1, 1};
        btScaledBvhTriangleMeshShape shape {&child, scaling};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Mesh3() { init(*this); }
    };

    struct Mesh4
    {
        const std::vector<std::vector<btVector3>> triangles {{
            {btVector3(127.2141571044921875, -144.489776611328125, -67.22026824951171875), btVector3(127.2141571044921875, -144.489776611328125, -9.3186511993408203125), btVector3(-14.818561553955078125, -114.1361236572265625, 9.71700382232666015625)},
            {btVector3(-14.818561553955078125, -114.1361236572265625, 9.71700382232666015625), btVector3(-14.818546295166015625, -114.1361083984375, -72.82059478759765625), btVector3(127.2141571044921875, -144.489776611328125, -67.22026824951171875)},
            {btVector3(231.712982177734375, 112.0338134765625, -81.81510162353515625), btVector3(94.68686676025390625, 186.737396240234375, -87.415435791015625), btVector3(94.68686676025390625, 186.737396240234375, -6.17350006103515625)},
            {btVector3(94.68686676025390625, 186.737396240234375, -6.17350006103515625), btVector3(231.7129669189453125, 112.033782958984375, -20.698131561279296875), btVector3(231.712982177734375, 112.0338134765625, -81.81510162353515625)},
            {btVector3(127.2141571044921875, -144.489776611328125, -67.22026824951171875), btVector3(-14.818546295166015625, -114.1361083984375, -72.82059478759765625), btVector3(94.68686676025390625, 186.737396240234375, -87.415435791015625)},
            {btVector3(94.68686676025390625, 186.737396240234375, -87.415435791015625), btVector3(231.712982177734375, 112.0338134765625, -81.81510162353515625), btVector3(127.2141571044921875, -144.489776611328125, -67.22026824951171875)},
            {btVector3(-14.818561553955078125, -114.1361236572265625, 9.71700382232666015625), btVector3(127.2141571044921875, -144.489776611328125, -9.3186511993408203125), btVector3(231.7129669189453125, 112.033782958984375, -20.698131561279296875)},
            {btVector3(231.7129669189453125, 112.033782958984375, -20.698131561279296875), btVector3(94.68686676025390625, 186.737396240234375, -6.17350006103515625), btVector3(-14.818561553955078125, -114.1361236572265625, 9.71700382232666015625)},
            {btVector3(127.2141571044921875, -144.489776611328125, -9.3186511993408203125), btVector3(127.2141571044921875, -144.489776611328125, -67.22026824951171875), btVector3(231.712982177734375, 112.0338134765625, -81.81510162353515625)},
            {btVector3(231.712982177734375, 112.0338134765625, -81.81510162353515625), btVector3(231.7129669189453125, 112.033782958984375, -20.698131561279296875), btVector3(127.2141571044921875, -144.489776611328125, -9.3186511993408203125)},
            {btVector3(261.976165771484375, -247.282928466796875, -199.272430419921875), btVector3(261.97613525390625, -247.28289794921875, -69.9124908447265625), btVector3(26.63442230224609375, -285.659881591796875, -71.75213623046875)},
            {btVector3(26.63442230224609375, -285.659881591796875, -71.75213623046875), btVector3(58.30145263671875, -285.659881591796875, -239.4290618896484375), btVector3(261.976165771484375, -247.282928466796875, -199.272430419921875)},
            {btVector3(252.56353759765625, 91.8106536865234375, -199.2724151611328125), btVector3(24.5088253021240234375, 107.3464813232421875, -239.429046630859375), btVector3(-7.1582050323486328125, 107.34649658203125, -73.49704742431640625)},
            {btVector3(-7.1582050323486328125, 107.34649658203125, -73.49704742431640625), btVector3(252.5635223388671875, 91.8106231689453125, -75.74108123779296875), btVector3(252.56353759765625, 91.8106536865234375, -199.2724151611328125)},
            {btVector3(261.976165771484375, -247.282928466796875, -199.272430419921875), btVector3(58.30145263671875, -285.659881591796875, -239.4290618896484375), btVector3(24.5088253021240234375, 107.3464813232421875, -239.429046630859375)},
            {btVector3(24.5088253021240234375, 107.3464813232421875, -239.429046630859375), btVector3(252.56353759765625, 91.8106536865234375, -199.2724151611328125), btVector3(261.976165771484375, -247.282928466796875, -199.272430419921875)},
            {btVector3(26.63442230224609375, -285.659881591796875, -71.75213623046875), btVector3(261.97613525390625, -247.28289794921875, -69.9124908447265625), btVector3(252.5635223388671875, 91.8106231689453125, -75.74108123779296875)},
            {btVector3(252.5635223388671875, 91.8106231689453125, -75.74108123779296875), btVector3(-7.1582050323486328125, 107.34649658203125, -73.49704742431640625), btVector3(26.63442230224609375, -285.659881591796875, -71.75213623046875)},
            {btVector3(261.97613525390625, -247.28289794921875, -69.9124908447265625), btVector3(261.976165771484375, -247.282928466796875, -199.272430419921875), btVector3(252.56353759765625, 91.8106536865234375, -199.2724151611328125)},
            {btVector3(252.56353759765625, 91.8106536865234375, -199.2724151611328125), btVector3(252.5635223388671875, 91.8106231689453125, -75.74108123779296875), btVector3(261.97613525390625, -247.28289794921875, -69.9124908447265625)},
            {btVector3(70.37933349609375, -321.66650390625, -22.208202362060546875), btVector3(70.3793182373046875, -310.325531005859375, 49.395694732666015625), btVector3(-153.392242431640625, -336.590118408203125, 76.0716400146484375)},
            {btVector3(-153.392242431640625, -336.590118408203125, 76.0716400146484375), btVector3(-153.392242431640625, -352.286041259765625, -23.028697967529296875), btVector3(70.37933349609375, -321.66650390625, -22.208202362060546875)},
            {btVector3(70.379302978515625, -172.317138671875, -45.86280059814453125), btVector3(-187.1848602294921875, -157.8486328125, -53.8245697021484375), btVector3(-187.18487548828125, -142.152679443359375, 45.275787353515625)},
            {btVector3(-187.18487548828125, -142.152679443359375, 45.275787353515625), btVector3(70.3793182373046875, -160.976226806640625, 25.7410869598388671875), btVector3(70.379302978515625, -172.317138671875, -45.86280059814453125)},
            {btVector3(70.37933349609375, -321.66650390625, -22.208202362060546875), btVector3(-153.392242431640625, -352.286041259765625, -23.028697967529296875), btVector3(-187.1848602294921875, -157.8486328125, -53.8245697021484375)},
            {btVector3(-187.1848602294921875, -157.8486328125, -53.8245697021484375), btVector3(70.379302978515625, -172.317138671875, -45.86280059814453125), btVector3(70.37933349609375, -321.66650390625, -22.208202362060546875)},
            {btVector3(-153.392242431640625, -336.590118408203125, 76.0716400146484375), btVector3(70.3793182373046875, -310.325531005859375, 49.395694732666015625), btVector3(70.3793182373046875, -160.976226806640625, 25.7410869598388671875)},
            {btVector3(70.3793182373046875, -160.976226806640625, 25.7410869598388671875), btVector3(-187.18487548828125, -142.152679443359375, 45.275787353515625), btVector3(-153.392242431640625, -336.590118408203125, 76.0716400146484375)},
            {btVector3(70.3793182373046875, -310.325531005859375, 49.395694732666015625), btVector3(70.37933349609375, -321.66650390625, -22.208202362060546875), btVector3(70.379302978515625, -172.317138671875, -45.86280059814453125)},
            {btVector3(70.379302978515625, -172.317138671875, -45.86280059814453125), btVector3(70.3793182373046875, -160.976226806640625, 25.7410869598388671875), btVector3(70.3793182373046875, -310.325531005859375, 49.395694732666015625)},
            {btVector3(-87.05802154541015625, -80.8120880126953125, 129.385986328125), btVector3(-87.05803680419921875, -80.8120880126953125, 226.8372650146484375), btVector3(-228.86474609375, -98.93280029296875, 242.0215301513671875)},
            {btVector3(-228.86474609375, -98.93280029296875, 242.0215301513671875), btVector3(-228.86474609375, -98.93280029296875, 123.78565216064453125), btVector3(-87.05802154541015625, -80.8120880126953125, 129.385986328125)},
            {btVector3(-87.05805206298828125, 105.68819427490234375, 129.3860015869140625), btVector3(-262.657379150390625, 114.700714111328125, 123.78565216064453125), btVector3(-262.657379150390625, 114.7007293701171875, 242.02154541015625)},
            {btVector3(-262.657379150390625, 114.7007293701171875, 242.02154541015625), btVector3(-87.05806732177734375, 105.68820953369140625, 226.8372802734375), btVector3(-87.05805206298828125, 105.68819427490234375, 129.3860015869140625)},
            {btVector3(-87.05802154541015625, -80.8120880126953125, 129.385986328125), btVector3(-228.86474609375, -98.93280029296875, 123.78565216064453125), btVector3(-262.657379150390625, 114.700714111328125, 123.78565216064453125)},
            {btVector3(-262.657379150390625, 114.700714111328125, 123.78565216064453125), btVector3(-87.05805206298828125, 105.68819427490234375, 129.3860015869140625), btVector3(-87.05802154541015625, -80.8120880126953125, 129.385986328125)},
            {btVector3(-228.86474609375, -98.93280029296875, 242.0215301513671875), btVector3(-87.05803680419921875, -80.8120880126953125, 226.8372650146484375), btVector3(-87.05806732177734375, 105.68820953369140625, 226.8372802734375)},
            {btVector3(-87.05806732177734375, 105.68820953369140625, 226.8372802734375), btVector3(-262.657379150390625, 114.7007293701171875, 242.02154541015625), btVector3(-228.86474609375, -98.93280029296875, 242.0215301513671875)},
            {btVector3(-87.05803680419921875, -80.8120880126953125, 226.8372650146484375), btVector3(-87.05802154541015625, -80.8120880126953125, 129.385986328125), btVector3(-87.05805206298828125, 105.68819427490234375, 129.3860015869140625)},
            {btVector3(-87.05805206298828125, 105.68819427490234375, 129.3860015869140625), btVector3(-87.05806732177734375, 105.68820953369140625, 226.8372802734375), btVector3(-87.05803680419921875, -80.8120880126953125, 226.8372650146484375)},
            {btVector3(19.9618892669677734375, -232.823455810546875, 9.30895233154296875), btVector3(19.9618740081787109375, -232.823455810546875, 61.761646270751953125), btVector3(-130.46112060546875, -253.700408935546875, 76.80644989013671875)},
            {btVector3(-130.46112060546875, -253.700408935546875, 76.80644989013671875), btVector3(-125.48846435546875, -253.700408935546875, -4.35540771484375), btVector3(19.9618892669677734375, -232.823455810546875, 9.30895233154296875)},
            {btVector3(19.9618701934814453125, 47.853260040283203125, 80.03176116943359375), btVector3(-159.2810821533203125, 47.96441650390625, 74.431396484375), btVector3(-164.2537384033203125, 27.0286102294921875, 154.099822998046875)},
            {btVector3(-164.2537384033203125, 27.0286102294921875, 154.099822998046875), btVector3(19.9618625640869140625, 25.65235137939453125, 133.169219970703125), btVector3(19.9618701934814453125, 47.853260040283203125, 80.03176116943359375)},
            {btVector3(19.9618892669677734375, -232.823455810546875, 9.30895233154296875), btVector3(-125.48846435546875, -253.700408935546875, -4.35540771484375), btVector3(-159.2810821533203125, 47.96441650390625, 74.431396484375)},
            {btVector3(-159.2810821533203125, 47.96441650390625, 74.431396484375), btVector3(19.9618701934814453125, 47.853260040283203125, 80.03176116943359375), btVector3(19.9618892669677734375, -232.823455810546875, 9.30895233154296875)},
            {btVector3(-130.46112060546875, -253.700408935546875, 76.80644989013671875), btVector3(19.9618740081787109375, -232.823455810546875, 61.761646270751953125), btVector3(19.9618625640869140625, 25.65235137939453125, 133.169219970703125)},
            {btVector3(19.9618625640869140625, 25.65235137939453125, 133.169219970703125), btVector3(-164.2537384033203125, 27.0286102294921875, 154.099822998046875), btVector3(-130.46112060546875, -253.700408935546875, 76.80644989013671875)},
            {btVector3(19.9618740081787109375, -232.823455810546875, 61.761646270751953125), btVector3(19.9618892669677734375, -232.823455810546875, 9.30895233154296875), btVector3(19.9618701934814453125, 47.853260040283203125, 80.03176116943359375)},
            {btVector3(19.9618701934814453125, 47.853260040283203125, 80.03176116943359375), btVector3(19.9618625640869140625, 25.65235137939453125, 133.169219970703125), btVector3(19.9618740081787109375, -232.823455810546875, 61.761646270751953125)},
            {btVector3(111.757293701171875, 46.82357025146484375, -23.50151824951171875), btVector3(111.75730133056640625, 46.82360076904296875, 41.27971649169921875), btVector3(-120.8619232177734375, 26.180267333984375, 59.787322998046875)},
            {btVector3(-120.8619232177734375, 26.180267333984375, 59.787322998046875), btVector3(-120.8619232177734375, 26.180267333984375, -29.10186004638671875), btVector3(111.757293701171875, 46.82357025146484375, -23.50151824951171875)},
            {btVector3(97.6928253173828125, 218.0892333984375, -23.50151824951171875), btVector3(-154.6544952392578125, 233.6251220703125, -29.10186004638671875), btVector3(-154.654510498046875, 233.6251220703125, 59.78733062744140625)},
            {btVector3(-154.654510498046875, 233.6251220703125, 59.78733062744140625), btVector3(97.69283294677734375, 218.0892486572265625, 41.27972412109375), btVector3(97.6928253173828125, 218.0892333984375, -23.50151824951171875)},
            {btVector3(111.757293701171875, 46.82357025146484375, -23.50151824951171875), btVector3(-120.8619232177734375, 26.180267333984375, -29.10186004638671875), btVector3(-154.6544952392578125, 233.6251220703125, -29.10186004638671875)},
            {btVector3(-154.6544952392578125, 233.6251220703125, -29.10186004638671875), btVector3(97.6928253173828125, 218.0892333984375, -23.50151824951171875), btVector3(111.757293701171875, 46.82357025146484375, -23.50151824951171875)},
            {btVector3(-120.8619232177734375, 26.180267333984375, 59.787322998046875), btVector3(111.75730133056640625, 46.82360076904296875, 41.27971649169921875), btVector3(97.69283294677734375, 218.0892486572265625, 41.27972412109375)},
            {btVector3(97.69283294677734375, 218.0892486572265625, 41.27972412109375), btVector3(-154.654510498046875, 233.6251220703125, 59.78733062744140625), btVector3(-120.8619232177734375, 26.180267333984375, 59.787322998046875)},
            {btVector3(111.75730133056640625, 46.82360076904296875, 41.27971649169921875), btVector3(111.757293701171875, 46.82357025146484375, -23.50151824951171875), btVector3(97.6928253173828125, 218.0892333984375, -23.50151824951171875)},
            {btVector3(97.6928253173828125, 218.0892333984375, -23.50151824951171875), btVector3(97.69283294677734375, 218.0892486572265625, 41.27972412109375), btVector3(111.75730133056640625, 46.82360076904296875, 41.27971649169921875)},
            {btVector3(162.4515228271484375, 169.8447265625, -74.99315643310546875), btVector3(162.4515228271484375, 186.15289306640625, -18.56806182861328125), btVector3(-58.764862060546875, 171.739044189453125, 17.144008636474609375)},
            {btVector3(-58.764862060546875, 171.739044189453125, 17.144008636474609375), btVector3(-58.764862060546875, 139.24249267578125, -73.67574310302734375), btVector3(162.4515228271484375, 169.8447265625, -74.99315643310546875)},
            {btVector3(153.033050537109375, 317.18017578125, -104.923583984375), btVector3(-92.5574798583984375, 331.058074951171875, -113.87520599365234375), btVector3(-92.5574951171875, 363.554595947265625, -35.65110015869140625)},
            {btVector3(-92.5574951171875, 363.554595947265625, -35.65110015869140625), btVector3(153.0330657958984375, 333.48834228515625, -52.58312225341796875), btVector3(153.033050537109375, 317.18017578125, -104.923583984375)},
            {btVector3(162.4515228271484375, 169.8447265625, -74.99315643310546875), btVector3(-58.764862060546875, 139.24249267578125, -73.67574310302734375), btVector3(-92.5574798583984375, 331.058074951171875, -113.87520599365234375)},
            {btVector3(-92.5574798583984375, 331.058074951171875, -113.87520599365234375), btVector3(153.033050537109375, 317.18017578125, -104.923583984375), btVector3(162.4515228271484375, 169.8447265625, -74.99315643310546875)},
            {btVector3(-58.764862060546875, 171.739044189453125, 17.144008636474609375), btVector3(162.4515228271484375, 186.15289306640625, -18.56806182861328125), btVector3(153.0330657958984375, 333.48834228515625, -52.58312225341796875)},
            {btVector3(153.0330657958984375, 333.48834228515625, -52.58312225341796875), btVector3(-92.5574951171875, 363.554595947265625, -35.65110015869140625), btVector3(-58.764862060546875, 171.739044189453125, 17.144008636474609375)},
            {btVector3(162.4515228271484375, 186.15289306640625, -18.56806182861328125), btVector3(162.4515228271484375, 169.8447265625, -74.99315643310546875), btVector3(153.033050537109375, 317.18017578125, -104.923583984375)},
            {btVector3(153.033050537109375, 317.18017578125, -104.923583984375), btVector3(153.0330657958984375, 333.48834228515625, -52.58312225341796875), btVector3(162.4515228271484375, 186.15289306640625, -18.56806182861328125)},
        }};
        const btVector3 origin {55537.421875, -47893.875, 430.579010009765625};
        const btTransform transform {
            btMatrix3x3 {
                0.848353207111358642578125, -0.358678400516510009765625, -0.389418423175811767578125,
                0.310398101806640625, 0.93282854557037353515625, -0.182986229658126831054688,
                0.4288938045501708984375, 0.0343622341752052307128906, 0.9027011394500732421875
            },
            origin,
        };
        btTriangleMesh mesh = makeTriangleMesh(triangles);
        btBvhTriangleMeshShape child {&mesh, true};
        const btVector3 scaling {1, 1, 1};
        btScaledBvhTriangleMeshShape shape {&child, scaling};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_STATIC_OBJECT;
        btCollisionObject object;

        Mesh4() { init(*this); }
    };

    struct Player2
    {
        const btVector3 halfExtents {29.279994964599609375, 28.4799976348876953125, 66.5};
        const btVector3 origin {55193.90625, -48007.45703125, 636.29437255859375};
        const btTransform transform {
            btMatrix3x3(
                0.002278387546539306640625, 0.9999973773956298828125, 0,
                -0.9999973773956298828125, 0.002278387546539306640625, -0,
                -0, 0, 1
            ),
            origin
        };
        btCapsuleShapeZ shape {halfExtents.x(), 2 * halfExtents.z() - 2 * halfExtents.x()};
        const btCollisionObject::CollisionFlags flags = btCollisionObject::CF_KINEMATIC_OBJECT;
        btCollisionObject object;

        Player2() { init(*this); }
    };
}

#endif // OPENMW_TEST_SUITE_MWMECHANICS_COLLISIONOBJECTS_HPP
