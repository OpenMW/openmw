#ifndef OPENMW_MWMECHANICS_FINDOPTIMALPATH_COMMON_HPP
#define OPENMW_MWMECHANICS_FINDOPTIMALPATH_COMMON_HPP

#include "../findoptimalpath.hpp"

#include "../../mwphysics/collisiontype.hpp"
#include "../../mwphysics/closestcollision.hpp"

#include <BulletCollision/CollisionShapes/btConcaveShape.h>

#include <osg/Math>

#include <array>
#include <numeric>
#include <stdexcept>
#include <string>

namespace MWMechanics
{
    namespace FindOptimalPath
    {
        using MWPhysics::Collision;

        inline std::pair<btVector3, btVector3> getTangentPoints(btScalar radius, const btVector3& position, const btVector3& source)
        {
            const auto hypot = position.distance(source);
            const auto far_cathetus = radius;
            if (hypot <= far_cathetus)
            {
                throw std::logic_error("hypot < far_cathetus: " + std::to_string(hypot) + " < " + std::to_string(far_cathetus));
            }
            const auto near_cathetus = std::sqrt((hypot * hypot) - (far_cathetus * far_cathetus));
            const auto sin = far_cathetus / hypot;
            const auto angle = std::asin(sin);
            const auto toCircle = (position - source).normalized();
            const auto toLeftTangent = source + toCircle.rotate(btVector3(0, 0, 1), angle) * near_cathetus;
            const auto toRightTangent = source + toCircle.rotate(btVector3(0, 0, 1), -angle) * near_cathetus;
            return {toLeftTangent, toRightTangent};
        }

        enum class AabbSide
        {
            X = 0,
            Y = 1,
            Z = 2,
        };

        inline AabbSide getAabbSide(const btVector3& normal)
        {
            std::array<btScalar, 3> values {{normal.x(), normal.y(), normal.z()}};
            for (auto& v : values)
            {
                v = std::abs(v);
            }
            return AabbSide(std::max_element(values.begin(), values.end()) - values.begin());
        }

        static const auto maxSlopeCos = std::cos(osg::DegreesToRadians(btScalar(60)));

        inline bool isWalkableSlope(btScalar cos)
        {
            return cos > maxSlopeCos;
        }

        inline bool isWalkableSlope(const btVector3& normal)
        {
            return isWalkableSlope(normal.z());
        }

        static const auto minSlopeCos = -std::cos(osg::DegreesToRadians(btScalar(80)));

        inline bool isRoof(btScalar cos)
        {
            return cos < minSlopeCos;
        }

        inline bool isRoof(const btVector3& normal)
        {
            return isRoof(normal.z());
        }

        inline btVector3 projectToPlane(const btVector3& point, const btVector3& normal)
        {
            return point - point.dot(normal) * normal;
        }

        class LessByDistanceTo
        {
        public:
            LessByDistanceTo(const btVector3& point)
                : mPoint(point) {}

            bool operator ()(const btVector3& lhs, const btVector3& rhs) const
            {
                return lhs.distance(mPoint) < rhs.distance(mPoint);
            }

        private:
            const btVector3& mPoint;
        };

        class GetCentroid : public btTriangleCallback
        {
        public:
            btVector3 perform(const btConcaveShape& shape, const btVector3& aabbMin, const btVector3& aabbMax)
            {
                mCount = 0;
                mSum = btVector3(0, 0, 0);
                shape.processAllTriangles(this, aabbMin, aabbMax);
                return mSum / btScalar(mCount);
            }

        private:
            std::size_t mCount = 0;
            btVector3 mSum;

            void processTriangle(btVector3* triangle, int, int) override final
            {
                for (std::size_t i = 0; i < 3; ++i) {
                    mSum += triangle[i];
                }
                ++mCount;
            }
        };

        class GetFarestPoint : public btTriangleCallback
        {
        public:
            GetFarestPoint(const btVector3& point)
                : mPoint(point) {}

            const btVector3& perform(const btConcaveShape& shape, const btVector3& aabbMin, const btVector3& aabbMax)
            {
                mFarest = mPoint;
                shape.processAllTriangles(this, aabbMin, aabbMax);
                return mFarest;
            }

        private:
            const btVector3& mPoint;
            btVector3 mFarest;

            void processTriangle(btVector3* triangle, int, int) override final
            {
                const LessByDistanceTo less(mPoint);
                const auto localFarest = std::max_element(triangle, triangle + 3, less);
                if (!less(*localFarest, mFarest))
                {
                    mFarest = *localFarest;
                }
            }
        };

        inline btScalar normalizeAngle(btScalar value)
        {
            if (value > osg::PI)
            {
                return value - std::round(value * btScalar(0.5) * btScalar(1 / osg::PI)) * btScalar(2) * btScalar(osg::PI);
            }
            if (value < -osg::PI)
            {
                return value + std::round(std::abs(value) * btScalar(0.5) * btScalar(1 / osg::PI)) * btScalar(2) * btScalar(osg::PI);
            }
            return value;
        }

        class PointInt
        {
        public:
            PointInt(const btVector3& value)
                : mX(std::lround(value.x()))
                , mY(std::lround(value.y()))
                , mZ(std::lround(value.z()))
            {
            }

            PointInt(long x, long y, long z)
                : mX(x)
                , mY(y)
                , mZ(z)
            {
            }

            long x() const { return mX; }
            long y() const { return mY; }
            long z() const { return mZ; }

            bool operator ==(const PointInt& other) const
            {
                return mX == other.mX && mY == other.mY && mZ == other.mZ;
            }

            bool operator !=(const PointInt& other) const
            {
                return !(*this == other);
            }

            bool operator <(const PointInt& other) const
            {
                return std::make_tuple(mX, mY, mZ) < std::make_tuple(other.mX, other.mY, other.mZ);
            }

            friend PointInt operator *(const PointInt& lhs, long rhs)
            {
                return PointInt(lhs.mX * rhs, lhs.mY * rhs, lhs.mZ * rhs);
            }

        private:
            long mX;
            long mY;
            long mZ;
        };

        struct Transition
        {
            enum class State
            {
                Proposed,
                WithoutCollisions,
                WithGroundedDestination,
                SubTraced,
                Traced,
            };

            int mId;
            int mParentId;
            btScalar mPriority;
            btScalar mCost;
            std::size_t mDepth;
            btVector3 mSource;
            btVector3 mDestination;
            PointInt mSourceInt;
            PointInt mDestinationInt;
            State mState;
        };

        struct PointLess
        {
            template <class T>
            bool operator ()(const T& lhs, const T& rhs) const
            {
                return std::make_tuple(lhs.x(), lhs.y(), lhs.z()) < std::make_tuple(rhs.x(), rhs.y(), rhs.z());
            }
        };

        struct PointPairLess
        {
            template <class T>
            bool operator ()(const std::pair<T, T>& lhs, const std::pair<T, T>& rhs) const
            {
                return PointLess()(lhs.first, rhs.first)
                        || (!PointLess()(rhs.first, lhs.first) && PointLess()(lhs.second, rhs.second));
            }
        };

        struct ContactResultCallback : public btCollisionWorld::ContactResultCallback
        {
            bool mFound = false;

            btScalar addSingleResult(btManifoldPoint& cp,
                    const btCollisionObjectWrapper* /*col0Wrap*/, int /*partId0*/, int /*index0*/,
                    const btCollisionObjectWrapper* /*col1Wrap*/, int /*partId1*/, int /*index1*/) override
            {
                mFound = cp.getDistance() <= btScalar(0);
                return btScalar(0);
            }
        };

        inline btScalar getDistanceToLine(const btVector3& begin, const btVector3& end, const btVector3& point)
        {
            if (begin == end) {
                return begin.distance(point);
            }
            return (point - begin).cross(point - end).length() / begin.distance(end);
        }

        inline btScalar getCathetusLength(btScalar hypotenuse, btScalar cathetus)
        {
            return std::sqrt(hypotenuse * hypotenuse - cathetus * cathetus);
        }

        inline btScalar getDistanceToLineSegment(const btVector3& begin, const btVector3& end, const btVector3& point)
        {
            const auto length = begin.distance(end);
            const auto distanceToBegin = point.distance(begin);
            const auto distanceToEnd = point.distance(end);
            const auto distance = getDistanceToLine(begin, end, point);
            const auto beginCathetus = getCathetusLength(distanceToBegin, distance);
            const auto endCathetus = getCathetusLength(distanceToEnd, distance);
            return std::abs(beginCathetus + endCathetus - length) <= 1e-3
                ? distance
                : std::min(distanceToBegin, distanceToEnd);
        }

        inline btScalar getDistance(const Transition& transition, const btVector3& point, btScalar halfExtentZ)
        {
            const auto distanceToLineSegment = getDistanceToLineSegment(transition.mSource, transition.mDestination, point);
            const auto distanceToVerticalAxis = getDistanceToLineSegment(
                btVector3(transition.mSource.x(), transition.mSource.y(), 0),
                btVector3(transition.mDestination.x(), transition.mDestination.y(), 0),
                btVector3(point.x(), point.y(), 0));
            const auto verticalAxisHeight = getCathetusLength(distanceToLineSegment, distanceToVerticalAxis);
            return verticalAxisHeight <= halfExtentZ
                ? distanceToVerticalAxis
                : std::hypot(distanceToVerticalAxis, verticalAxisHeight - halfExtentZ);
        }

        struct GreaterByPriorityAndLessById
        {
            bool operator ()(const Transition& lhs, const Transition& rhs) const
            {
                return std::make_pair(lhs.mPriority, -lhs.mId) > std::make_pair(rhs.mPriority, -rhs.mId);
            }
        };

        struct LessByPriorityAndId
        {
            bool operator ()(const Transition& lhs, const Transition& rhs) const
            {
                return std::make_pair(lhs.mPriority, lhs.mId) < std::make_pair(rhs.mPriority, rhs.mId);
            }
        };

        struct GroundedDestination
        {
            enum class Type
            {
                Same,
                Undefined,
                Defined,
            };

            Type mType;
            btVector3 mPosition;
        };

        struct TransitionCollision
        {
            Collision collision;
            Transition transition;
        };

        inline btScalar getTentativeCost(btScalar cost, const btVector3& source, const btVector3& destination)
        {
            return cost + source.distance(destination);
        }

        inline Transition::State getNextState(Transition::State value)
        {
            switch (value)
            {
                case Transition::State::Proposed:
                    return Transition::State::WithoutCollisions;
                case Transition::State::WithoutCollisions:
                    return Transition::State::WithGroundedDestination;
                case Transition::State::WithGroundedDestination:
                    return Transition::State::Traced;
                case Transition::State::SubTraced:
                    throw std::invalid_argument("Transition::State::SubTraced is last Transition::State");
                case Transition::State::Traced:
                    throw std::invalid_argument("Transition::State::Traced is last Transition::State");
            }

            throw std::invalid_argument("Unhandled Transition::State: " + std::to_string(int(value)));
        }

        inline Transition withNextState(const Transition& transition)
        {
            auto result = transition;
            result.mState = getNextState(transition.mState);
            return result;
        }

        inline Transition withTracedState(const Transition& transition)
        {
            auto result = transition;
            result.mState = Transition::State::Traced;
            return result;
        }
    }
}

#endif // OPENMW_MWMECHANICS_FINDOPTIMALPATH_COMMON_HPP
