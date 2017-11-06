#ifndef OPENMW_MWMECHANICS_FINDOPTIMALPATH_ANY_ANGLE_VISITOR_H
#define OPENMW_MWMECHANICS_FINDOPTIMALPATH_ANY_ANGLE_VISITOR_H

#include "open_set.hpp"
#include "get_neighbors.hpp"
#include "debug.hpp"
#include "common.hpp"

#include "../../mwphysics/closestcollision.hpp"

#include <queue>
#include <unordered_set>

namespace MWMechanics
{
    namespace FindOptimalPath
    {
        struct EmptyFilter
        {
            bool shouldFilter(const Transition&) const { return false; }
            void addTransitionCollision(const Transition&, const Collision&) const {}
            void reset() const {}
        };

        template <typename Filter = EmptyFilter>
        class AnyAngleVisitor
        {
        public:
            AnyAngleVisitor(btCollisionWorld& collisionWorld, const btCollisionObject& actor,
                            const btVector3& initial, const btVector3& goal, const FindOptimalPathConfig& config,
                            Filter filter = Filter())
                : mCollisionWorld(collisionWorld)
                , mActor(actor)
                , mInitial(initial)
                , mGoal(goal)
                , mConfig(config)
                , mFilter(filter)
            {
            }

            std::vector<Transition> apply(const Transition& transition)
            {
                std::queue<Transition> transitions({transition});
                std::vector<Transition> result;

                while (!transitions.empty())
                {
                    const auto transition = transitions.front();
                    transitions.pop();
                    if (mFilter.shouldFilter(transition))
                    {
                        DEBUG_LOG << "filter\n";
                        mFiltered.push(transition);
                        continue;
                    }
                    for (const auto& newTransition : handleTransition(transition))
                    {
                        if (newTransition.mState == Transition::State::Proposed
                                || newTransition.mState == Transition::State::SubTraced
                                || newTransition.mState == Transition::State::Traced)
                        {
                            result.push_back(newTransition);
                        }
                        else
                        {
                            transitions.push(newTransition);
                        }
                    }
                }

                return result;
            }

            Transition makeInitialTransition()
            {
                return Transition {
                    getNextTransitionId(),
                    0,
                    getPriority(mInitial),
                    0,
                    0,
                    mInitial,
                    mGoal,
                    mInitialInt,
                    mGoalInt,
                    Transition::State::Proposed,
                };
            }

            btScalar getPriority(const btVector3& position) const
            {
                return position.distance(mGoal);
            }

            btScalar getPriority(btScalar tentativeCost, const btVector3& destination) const
            {
                return tentativeCost + destination.distance(mGoal);
            }

            btScalar getPriority(btScalar cost, const btVector3& source, const btVector3& destination) const
            {
                return getPriority(getTentativeCost(cost, source, destination), destination);
            }

            int getNextTransitionId()
            {
                return ++mTransitionCounter;
            }

            PointInt makePointInt(const btVector3& position) const
            {
                const auto denominator = getDenominator(position);
                return PointInt(mToLocal(position) / denominator) * denominator;
            }

            btScalar getMaxDistance() const
            {
                return mMaxDistance;
            }

            OpenSet getMore()
            {
                mFilter.reset();
                return std::move(mFiltered);
            }

        private:
            enum class CheckCollision
            {
                NotFound,
                Found,
                Undefined,
            };

            btCollisionWorld& mCollisionWorld;
            const btCollisionObject& mActor;
            const btVector3& mInitial;
            const btVector3& mGoal;
            const FindOptimalPathConfig& mConfig;
            Filter mFilter;
            const btVector3 mGoalAtInitialZ {mGoal.x(), mGoal.y(), mInitial.z()};
            const btVector3 mDirectionToGoal = (mGoal - mInitial).normalized();
            const btTransform mToLocal {btQuaternion(btVector3(0, 0, 1), normalizeAngle(std::atan2(mDirectionToGoal.y(), mDirectionToGoal.x()))), mGoalAtInitialZ};
            const PointInt mInitialInt = makePointInt(mInitial);
            const PointInt mGoalInt = makePointInt(mGoal);
            const btScalar mMaxStep = mInitial.distance(mGoal);
            const btScalar mHorizontalMargin = std::max(mConfig.mActorHalfExtents.x(), mConfig.mActorHalfExtents.y()) * mConfig.mHorizontalMarginFactor;
            const btScalar mVerticalMargin = mConfig.mActorHalfExtents.z() + 1;
            const btVector3 mToGround {0, 0, -5e2};
            OpenSet mFiltered;

            int mTransitionCounter = 0;
            btScalar mMaxDistance = 1;
            std::unordered_set<const btCollisionObject*> mOutput;

            std::vector<Transition> handleTransition(const Transition& transition)
            {
                switch (transition.mState)
                {
                    case Transition::State::Proposed:
                        return onTransitionProposed(transition);
                    case Transition::State::WithoutCollisions:
                        return onTransitionWithoutCollisions(transition);
                    case Transition::State::WithGroundedDestination:
                        return onTransitionWithGroundedDestination(transition);
                    case Transition::State::SubTraced:
                    case Transition::State::Traced:
                        return {transition};
                }

                throw std::invalid_argument("Unhandled Transition::State: " + std::to_string(int(transition.mState)));
            }

            std::vector<Transition> onTransitionProposed(const Transition& transition)
            {
                if (const auto collision = getClosestCollisionWithStepUp(transition.mSource, transition.mDestination))
                {
                    return handleCollision(collision.get(), transition);
                }

                return {withNextState(transition)};
            }

            std::vector<Transition> onTransitionWithoutCollisions(const Transition& transition)
            {
                const auto groundedDestination = groundDestination(transition);

                if (groundedDestination.mType == GroundedDestination::Type::Undefined)
                {
                    return {};
                }

                if (groundedDestination.mType == GroundedDestination::Type::Defined)
                {
                    return {
                        Transition {
                            transition.mId,
                            transition.mParentId,
                            getPriority(transition.mCost, transition.mSource, groundedDestination.mPosition),
                            transition.mCost,
                            transition.mDepth,
                            transition.mSource,
                            groundedDestination.mPosition,
                            transition.mSourceInt,
                            makePointInt(groundedDestination.mPosition),
                            Transition::State::WithGroundedDestination,
                        }
                    };
                }

                return {withNextState(transition)};
            }

            std::vector<Transition> onTransitionWithGroundedDestination(const Transition& transition)
            {
                return trace(transition);
            }

            std::vector<Transition> handleCollision(const Collision& collision, const Transition& transition)
            {
                if (mOutput.insert(collision.mObject).second)
                {
                    JSON_LOG << withType(*collision.mObject) << '\n';
                }

                JSON_LOG << withType(collision) << '\n';
                DEBUG_LOG << "collision"
                    << " point: " << collision.mPoint
                    << " normal: " << collision.mNormal
                    << " end: " << collision.mEnd
                    << " fraction: " << collision.mFraction
                    << " distance: " << getDistance(transition, collision.mPoint, mConfig.mActorHalfExtents.z())
                    << " shape: " << BroadphaseNativeTypes(collision.mObject->getCollisionShape()->getShapeType())
#ifdef OUTPUT_COLLISION_OBJECTS
                    << " ptr: " << collision.mObject->getCollisionShape()
#endif
                    << '\n';

                mFilter.addTransitionCollision(transition, collision);

                if (transition.mDestination == mGoal)
                {
                    btCollisionObject actorCopy = mActor;
                    actorCopy.setWorldTransform(btTransform(btMatrix3x3::getIdentity(), mGoal));
                    ContactResultCallback contactResultCallback;
                    contactResultCallback.m_collisionFilterGroup = actorCopy.getBroadphaseHandle()->m_collisionFilterGroup;
                    contactResultCallback.m_collisionFilterMask = actorCopy.getBroadphaseHandle()->m_collisionFilterMask;
                    auto objectCopy = *collision.mObject;
                    mCollisionWorld.contactPairTest(&actorCopy, &objectCopy, contactResultCallback);

                    if (contactResultCallback.mFound)
                    {
                        mMaxDistance = mGoal.distance(collision.mEnd) + btScalar(1);
                        DEBUG_LOG << "occupied max_distance: " << mMaxDistance << '\n';
                        const auto shift = (transition.mDestination - transition.mSource).normalized() * btScalar(0.1);
                        const auto destination = collision.mEnd - shift;
                        const auto newTransition = withNewDestination(transition, destination, true);
                        return newTransition ? std::vector<Transition>({newTransition.get()}) : std::vector<Transition>();
                    }
                }

                std::vector<Transition> result;
                for (const std::pair<btVector3, bool>& destination : getNeighbors(transition, collision))
                {
                    if (const auto newTransition = withNewDestination(transition, destination.first, destination.second))
                    {
                        result.push_back(newTransition.get());
                    }
                }
                return result;
            }

            GetNeighbors::Candidates getNeighbors(const Transition& transition, const Collision& collision) const
            {
                const auto minStep = btScalar(std::sqrt(2)) * getDenominator(collision.mEnd);
                return GetNeighbors(transition.mSource, transition.mDestination, mGoal, collision, minStep,
                                    mMaxStep, mHorizontalMargin, mVerticalMargin, mConfig.mAllowFly).perform();
            }

            GroundedDestination groundDestination(const Transition& transition)
            {
                if (mConfig.mAllowFly)
                {
                    return GroundedDestination {GroundedDestination::Type::Same, btVector3()};
                }

                const auto ground = getClosestCollision(transition.mDestination, transition.mDestination + mToGround);

                if (!ground)
                {
                    DEBUG_LOG << "ground is too far\n";
                    return GroundedDestination {GroundedDestination::Type::Undefined, btVector3()};
                }

                DEBUG_LOG << "collision ground"
                    << " point: " << ground->mPoint
                    << " normal: " << ground->mNormal
                    << " end: " << ground->mEnd
                    << " fraction: " << ground->mFraction
                    << " distance: " << getDistance(transition, ground->mPoint, mConfig.mActorHalfExtents.z())
                    << " shape: " << BroadphaseNativeTypes(ground->mObject->getCollisionShape()->getShapeType())
#ifdef OUTPUT_COLLISION_OBJECTS
                    << " ptr: " << ground->mObject->getCollisionShape()
#endif
                    << '\n';

                if (!isWalkableSlope(ground->mNormal))
                {
                    DEBUG_LOG << "ground slope is too steep\n";
                    return GroundedDestination {GroundedDestination::Type::Undefined, btVector3()};
                }

                const auto groundedDestination = ground->mEnd + btVector3(0, 0, 1);

                if (groundedDestination.z() >= transition.mDestination.z())
                {
                    return GroundedDestination {GroundedDestination::Type::Same, btVector3()};
                }
                DEBUG_LOG << "ground " << transition.mDestination << " => " << groundedDestination << ", normal: " << ground->mNormal << '\n';

                if (groundedDestination == transition.mSource)
                {
                    DEBUG_LOG << "zero transition\n";
                    return GroundedDestination {GroundedDestination::Type::Undefined, btVector3()};
                }

                return GroundedDestination {GroundedDestination::Type::Defined, groundedDestination};
            }

            std::vector<Transition> trace(const Transition& transition)
            {
                if (mConfig.mAllowFly)
                {
                    return {withNextState(transition)};
                }

                const auto path = transition.mDestination - transition.mSource;
                const auto length = path.length();
                const btScalar step = mConfig.mActorHalfExtents.x();

                if (length <= step)
                {
                    return {withNextState(transition)};
                }

                const auto count = std::ceil(length / step);
                Transition sub = transition;

                DEBUG_LOG << "trace path " << transition << " count: " << count << '\n';

                std::vector<Transition> result;

                for (long i = 0; i < count; ++i)
                {
                    sub.mParentId = sub.mId;
                    sub.mId = getNextTransitionId();

                    if (i < count - 1)
                    {
                        sub.mDestination = transition.mSource + path * btScalar(i + 1) / btScalar(count);
                        sub.mDestinationInt = makePointInt(sub.mDestination);
                        sub.mState = Transition::State::Proposed;
                        DEBUG_LOG << "trace " << i << " " << sub << '\n';
                        const auto groundedDestination = groundDestination(sub);
                        if (groundedDestination.mType == GroundedDestination::Type::Undefined)
                        {
                            break;
                        }
                        if (groundedDestination.mType == GroundedDestination::Type::Defined)
                        {
                            sub.mDestination = groundedDestination.mPosition;
                            sub.mDestinationInt = makePointInt(sub.mDestination);
                            sub.mPriority = getPriority(sub.mCost, sub.mSource, sub.mDestination);
                        }
                    }
                    else
                    {
                        sub.mDestination = transition.mDestination;
                        sub.mDestinationInt = makePointInt(sub.mDestination);
                        sub.mPriority = getPriority(transition.mCost, sub.mSource, sub.mDestination);
                        DEBUG_LOG << "trace " << i << " " << sub << '\n';
                    }

                    JSON_LOG << withType(sub, "trace") << '\n';

                    if (const auto collision = getClosestCollisionWithStepUp(sub.mSource, sub.mDestination))
                    {
                        DEBUG_LOG << "trace path end reason: found collision\n";
                        for (const auto newTransition : handleCollision(collision.get(), sub))
                        {
                             result.push_back(newTransition);
                        }
                        break;
                    }

                    sub.mState = (i == count - 1) ? Transition::State::Traced : Transition::State::SubTraced;

                    result.push_back(sub);

                    sub.mCost = getTentativeCost(sub.mCost, sub.mSource, sub.mDestination);
                    sub.mSource = sub.mDestination;
                    sub.mSourceInt = sub.mDestinationInt;
                    sub.mPriority = getPriority(sub.mCost, sub.mSource, sub.mDestination);
                }

                DEBUG_LOG << "trace finished" << '\n';

                return result;
            }

            boost::optional<Transition> withNewDestination(const Transition& transition, const btVector3& destination, bool withoutCollision)
            {
                if (transition.mSource.distance(destination) <= btScalar(1))
                {
                    DEBUG_LOG << "ignore add " << transition << " new destination: " << destination << " reason: zero transition" << '\n';
                    return boost::none;
                }

                const auto destinationInt = makePointInt(destination);

                if (transition.mSourceInt == destinationInt)
                {
                    DEBUG_LOG << "ignore add " << transition << " reason: zero int transition" << '\n';
                    return boost::none;
                }

                const auto tentativeCost = getTentativeCost(transition.mCost, transition.mSource, destination);

                return Transition {
                    getNextTransitionId(),
                    transition.mId,
                    getPriority(tentativeCost, destination),
                    transition.mCost,
                    transition.mDepth,
                    transition.mSource,
                    destination,
                    transition.mSourceInt,
                    makePointInt(destination),
                    withoutCollision ? Transition::State::WithoutCollisions : Transition::State::Proposed,
                };
            }

            btScalar getDenominator(const btVector3& position) const
            {
                const auto distance = btScalar(mConfig.mSpaceScailingFactor) * position.distance(mGoal);
                return distance > btScalar(2) ? std::log2(distance) : btScalar(1);
            }

            boost::optional<Collision> getClosestCollision(const btVector3& source, const btVector3& destination)
            {
                return MWPhysics::getClosestCollision(mActor, source, destination, mCollisionWorld);
            }

            boost::optional<Collision> getClosestCollisionWithStepUp(const btVector3& source, const btVector3& destination)
            {
                return MWPhysics::getClosestCollisionWithStepUp(mActor, source, destination, mCollisionWorld);
            }
        };

        class HasNearCollisionFilter
        {
        public:
            HasNearCollisionFilter(const FindOptimalPathConfig& config)
                : mConfig(&config)
                , mNearCollisionMaxDistance(mConfig->mHasNearCollisionFilterFactor
                    * std::min({mConfig->mActorHalfExtents.x(), mConfig->mActorHalfExtents.y(), mConfig->mActorHalfExtents.z()}))
            {
            }

            bool shouldFilter(const Transition& transition) const
            {
                if (transition.mState != Transition::State::Proposed)
                {
                    return false;
                }

                const auto isNearCollisionPoint = [&] (const TransitionCollision& v)
                {
                    return getDistance(transition, v.collision.mPoint, mConfig->mActorHalfExtents.z()) <= mNearCollisionMaxDistance;
                };
                const auto it = std::find_if(mCollisions.begin(), mCollisions.end(), isNearCollisionPoint);

                if (it != mCollisions.end())
                {
                    DEBUG_LOG << "found near collision point " << it->collision.mPoint
                        << " distance: " << getDistance(transition, it->collision.mPoint, mConfig->mActorHalfExtents.z())
                        << " max_distance: " << mNearCollisionMaxDistance
                        << '\n';
                    return true;
                }

                return false;
            }

            void addTransitionCollision(const Transition& transition, const Collision& collision)
            {
                mCollisions.push_back(TransitionCollision {collision, transition});
            }

            void reset()
            {
                mCollisions.clear();
                mNearCollisionMaxDistance *= mConfig->mHasNearCollisionFilterReduceFactor;
            }

        private:
            const FindOptimalPathConfig* mConfig;
            btScalar mNearCollisionMaxDistance;
            std::vector<TransitionCollision> mCollisions;
        };
    }
}

#endif // OPENMW_MWMECHANICS_FINDOPTIMALPATH_ANY_ANGLE_VISITOR_H
