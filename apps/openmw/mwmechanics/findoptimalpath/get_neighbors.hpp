#ifndef OPENMW_MWMECHANICS_FINDOPTIMALPATH_GETNEIGHBORS_H
#define OPENMW_MWMECHANICS_FINDOPTIMALPATH_GETNEIGHBORS_H

#include "debug.hpp"
#include "common.hpp"

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>

#include <sstream>

namespace MWMechanics
{
    namespace FindOptimalPath
    {
        class GetNeighbors
        {
        public:
            using Candidates = std::vector<std::pair<btVector3, bool>>;

            GetNeighbors(
                    const btVector3& source,
                    const btVector3& destination,
                    const btVector3& goal,
                    const Collision& collision,
                    btScalar minStep,
                    btScalar maxStep,
                    btScalar horizontalMargin,
                    btScalar verticalMargin,
                    bool allowFly)
                : mSource(source)
                , mDestination(destination)
                , mGoal(goal)
                , mCollision(collision)
                , mMinStep(minStep)
                , mMaxStep(maxStep)
                , mHorizontalMargin(horizontalMargin)
                , mVerticalMargin(verticalMargin)
                , mAllowFly(allowFly)
                , mDirection((mDestination - mSource).normalized())
            {
            }

            Candidates perform() const
            {
                return getShapeCandidates(mCollision.mObject->getCollisionShape(), mCollision.mObject->getWorldTransform());
            }

        private:
            const btVector3& mSource;
            const btVector3& mDestination;
            const btVector3& mGoal;
            const Collision& mCollision;
            const btScalar mMinStep;
            const btScalar mMaxStep;
            const btScalar mHorizontalMargin;
            const btScalar mVerticalMargin;
            const bool mAllowFly;
            const btVector3 mDirection;

            Candidates getShapeCandidates(const btCollisionShape* shape, const btTransform& transform) const
            {
                if (shape->isCompound())
                {
                    return getCompoundShapeCandidates(*static_cast<const btCompoundShape*>(shape), transform);
                }
                else if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
                {
                    return getShapeWithRadiusCandidates(*static_cast<const btSphereShape*>(shape), transform);
                }
                else if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE && static_cast<const btCapsuleShape*>(shape)->getUpAxis() == 2)
                {
                    return getShapeWithRadiusCandidates(*static_cast<const btCapsuleShapeZ*>(shape), transform);
                }
                else if (shape->getShapeType() == TERRAIN_SHAPE_PROXYTYPE)
                {
                    return getHeightfieldTerrainShapeCandidates(*static_cast<const btHeightfieldTerrainShape*>(shape), transform);
                }
                else if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE)
                {
                    return getSingleShapeCandidates(*static_cast<const btBoxShape*>(shape), transform);
                }
                else if (shape->getShapeType() == STATIC_PLANE_PROXYTYPE)
                {
                    return getStaticPlaneShapeCandidates(*static_cast<const btStaticPlaneShape*>(shape), transform);
                }
                else if (shape->isConcave())
                {
                    return getSingleShapeCandidates(*static_cast<const btConcaveShape*>(shape), transform);
                }
                else
                {
                    std::ostringstream stream;
                    stream << "GetNeighbors::getSingleShapeCandidates is not implemented for shape type: " << shape->getShapeType();
                    throw std::logic_error(stream.str());
                }
            }

            Candidates getCompoundShapeCandidates(const btCompoundShape& shape, const btTransform& transform) const {
                Candidates result;
                // TODO: select one
                for (int i = 0, num = shape.getNumChildShapes(); i < num; ++i)
                {
                    const auto sub = getShapeCandidates(shape.getChildShape(i), transform * shape.getChildTransform(i));
                    std::copy(sub.begin(), sub.end(), std::back_inserter(result));
                }
                return result;
            }

            template <class ShapeWithRadius>
            Candidates getShapeWithRadiusCandidates(const ShapeWithRadius& shape, const btTransform& transform) const
            {
                return getTangentPointsCandidates(transform.getOrigin(), shape.getRadius());
            }

            btVector3 adjustDestination(const btVector3& destination) const
            {
                const auto toDestination = destination - mSource;
                const auto increaseFactor = mMinStep / toDestination.norm();
                return mSource + toDestination * (btScalar(1) + increaseFactor);
            }

            Candidates getHeightfieldTerrainShapeCandidates(const btHeightfieldTerrainShape& shape, const btTransform& transform) const
            {
                const auto length = std::min(mMaxStep, std::max(mMinStep, btScalar(2) * mHorizontalMargin));
                return getSingleShapeCandidatesImpl(shape, transform, length);
            }

            Candidates getStaticPlaneShapeCandidates(const btStaticPlaneShape& shape, const btTransform& transform) const
            {
                return getSingleShapeCandidatesImpl(shape, transform, mMinStep);
            }

            template <class Shape>
            Candidates getSingleShapeCandidates(const Shape& shape, const btTransform& transform) const
            {
                btVector3 aabbMin;
                btVector3 aabbMax;
                shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
                const auto aabbSide = getAabbSide(transform.invXform(mCollision.mNormal));
                switch (aabbSide)
                {
                    case AabbSide::X:
                        return getSingleShapeCandidatesImpl(
                            shape, transform,
                            limitLength(std::min(aabbMax.y() - aabbMin.y(), aabbMax.z() - aabbMin.z()) * btScalar(0.5))
                        );
                    case AabbSide::Y:
                        return getSingleShapeCandidatesImpl(
                            shape, transform,
                            limitLength(std::min(aabbMax.x() - aabbMin.x(), aabbMax.z() - aabbMin.z()) * btScalar(0.5))
                        );
                    case AabbSide::Z:
                        return getSingleShapeCandidatesImpl(
                            shape, transform,
                            limitLength(std::min(aabbMax.x() - aabbMin.x(), aabbMax.y() - aabbMin.y()) * btScalar(0.5))
                        );
                }

                throw std::logic_error("Unhandled AabbSide type " + std::to_string(int(aabbSide)));
            }

            template <class Shape>
            Candidates getSingleShapeCandidatesImpl(const Shape& shape, const btTransform& transform, btScalar length) const
            {
                assert(length > std::numeric_limits<btScalar>::epsilon());
    #ifdef FIND_OPTIMAL_PATH_DEBUG_OUTPUT
                DEBUG_LOG << std::boolalpha << "walkable: " << isWalkableSlope(mCollision.mNormal)
                    << " roof: " << isRoof(mCollision.mNormal)
                    << " z: " << mCollision.mNormal.z() << '\n';
    #endif
                const auto base = mCollision.mEnd + mCollision.mNormal;
                btVector3 projectedUp;
                btVector3 projectedLeft;
                const btVector3 z(0, 0, 1);
                if (std::abs(mDirection.z()) <= std::abs(mDirection.y()))
                {
                    const auto left = z.cross(mDirection);
                    const auto up = mDirection.cross(left);
                    if (up.dot(mCollision.mNormal) <= left.dot(mCollision.mNormal))
                    {
                        projectedUp = projectToPlane(up, mCollision.mNormal).normalized();
                        projectedLeft = mCollision.mNormal.cross(projectedUp);
                    }
                    else
                    {
                        projectedLeft = projectToPlane(left, mCollision.mNormal).normalized();
                        projectedUp = projectedLeft.cross(mCollision.mNormal);
                    }
                }
                else
                {
                    const btVector3 y(0, 1, 0);
                    const auto up = mDirection.cross(y);
                    const auto left = up.cross(mDirection);
                    if (up.dot(mCollision.mNormal) <= left.dot(mCollision.mNormal))
                    {
                        projectedUp = projectToPlane(up, mCollision.mNormal).normalized();
                        projectedLeft = projectedUp.cross(mCollision.mNormal);
                    }
                    else
                    {
                        projectedLeft = projectToPlane(left, mCollision.mNormal).normalized();
                        projectedUp = projectedLeft.cross(mCollision.mNormal);
                    }
                }

                if (projectedUp.dot(z) < projectedLeft.dot(z))
                {
                    std::swap(projectedUp, projectedLeft);
                }

                Candidates result;

                if (mAllowFly || isWalkableSlope(mCollision.mNormal) || isRoof(mCollision.mNormal))
                {
                    DEBUG_LOG << "above: " << projectedUp << " length: " << length << '\n';
                    projectedUp *= length;
                    result.push_back({base + projectedUp, false});
                    result.push_back({base - projectedUp, false});
                    if (!mAllowFly) {
                        return result;
                    }
                }

                result.push_back({base, true});

                DEBUG_LOG << "front: " << projectedLeft << " length: " << length << '\n';
                projectedLeft *= length;

                if (std::abs(mCollision.mNormal.dot((mGoal - mCollision.mEnd).normalized())) < btScalar(0.1))
                {
                    const auto left = base + projectedLeft;
                    const auto right = base + projectedLeft;
                    result.push_back({left.distance(mGoal) < right.distance(mGoal) ? left : right, false});
                    const auto tangent = getTangentPointsCandidates(shape, transform);
                    std::copy(tangent.begin(), tangent.end(), std::back_inserter(result));
                } else {
                    result.push_back({base + projectedLeft, false});
                    result.push_back({base - projectedLeft, false});
                }

                return result;
            }

            Candidates getTangentPointsCandidates(const btBoxShape& shape, const btTransform& transform) const
            {
                btVector3 aabbMin;
                btVector3 aabbMax;
                shape.getAabb(transform, aabbMin, aabbMax);
                const auto center = (aabbMin + aabbMax) * btScalar(0.5);
                const auto radius = std::min((aabbMax - aabbMin).length() * btScalar(0.5), center.distance(mSource) - mHorizontalMargin);
                return getTangentPointsCandidates(center, radius);
            }

            Candidates getTangentPointsCandidates(const btConcaveShape& shape, const btTransform& transform) const
            {
                btVector3 aabbMin;
                btVector3 aabbMax;
                shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
                const auto center = GetCentroid().perform(shape, aabbMin, aabbMax);
                const auto farest = GetFarestPoint(center).perform(shape, aabbMin, aabbMax);
                const auto transformedCenter = transform(center);
                const auto radius = std::min(center.distance(farest), transformedCenter.distance(mSource) - mHorizontalMargin);
                return getTangentPointsCandidates(transformedCenter, radius);
            }

            Candidates getTangentPointsCandidates(const btVector3& center, btScalar radius) const
            {
                if (mSource.distance(center) - radius <= btScalar(1))
                {
                    return {};
                }
                const auto tangentPoints = getTangentPoints(radius, center, mSource);
                const auto getCandidate = [&] (const btVector3& tangent)
                {
                    const auto result = tangent + mHorizontalMargin * (tangent - center).normalized();
                    return btVector3(result.x(), result.y(), mCollision.mEnd.z());
                };
                return {
                    {adjustDestination(getCandidate(tangentPoints.first)), false},
                    {adjustDestination(getCandidate(tangentPoints.second)), false},
                };
            }

            btScalar limitLength(btScalar value) const
            {
                return std::max(mMinStep, std::min(mMaxStep, value));
            }
        };
    }
}

#endif // OPENMW_MWMECHANICS_FINDOPTIMALPATH_GETNEIGHBORS_H
