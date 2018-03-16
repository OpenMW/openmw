#ifndef OPENMW_MWMECHANICS_FINDOPTIMALPATH_ALGORITHM_H
#define OPENMW_MWMECHANICS_FINDOPTIMALPATH_ALGORITHM_H

#include "open_set.hpp"
#include "debug.hpp"
#include "common.hpp"

#include <map>
#include <sstream>
#include <unordered_set>
#include <set>

namespace MWMechanics
{
    namespace FindOptimalPath
    {
        using MWMechanics::FindOptimalPathConfig;
        using MWMechanics::OptimalPath;

        class ClosedSet
        {
        public:
            void insert(const Transition& transition)
            {
                mTransitions.insert({transition.mSourceInt, transition.mDestinationInt});
            }

            bool contains(const Transition& transition) const
            {
                return mTransitions.count({transition.mSourceInt, transition.mDestinationInt}) > 0;
            }

            void clear()
            {
                mTransitions.clear();
            }

        private:
            std::set<std::pair<PointInt, PointInt>> mTransitions;
        };

        template <class Visitor>
        class Algorithm
        {
        public:
            Algorithm(const FindOptimalPathConfig& config, Visitor& visitor)
                : mConfig(config)
                , mVisitor(visitor)
            {
            }

            OptimalPath perform()
            {
                mOpenSet.clear();
                mCosts.clear();
                mCameFrom.clear();

                const auto initial = mVisitor.makeInitialTransition();

                JSON_LOG << withType(initial, "initial") << '\n';

                mCosts.insert({initial.mSourceInt, 0});
                push(initial);

                std::size_t iterations = 0;
                Transition final = initial;

                while (true)
                {
                    if (mOpenSet.empty())
                    {
                        mOpenSet = mVisitor.getMore();

                        DEBUG_LOG << "more " << mOpenSet.size() << '\n';

                        if (mOpenSet.empty())
                        {
                            break;
                        }
                    }

                    const Transition transition = mOpenSet.top();
                    DEBUG_LOG << "pop"
                        << " " << transition
                        << " distance: " << transition.mSource.distance(initial.mDestination)
                        << " iterations: " << iterations
                        << '\n';
                    JSON_LOG << withType(transition, "pop") << '\n';

                    if (transition.mSource != initial.mSource && !mCameFrom.count(transition.mSource))
                    {
                        throw std::logic_error("Dangling transition");
                    }

                    if (transition.mSource.distance(initial.mDestination) <= mVisitor.getMaxDistance())
                    {
                        final = transition;
                        DEBUG_LOG << "reach goal\n";
                        break;
                    }

                    const auto distanceDifference = final.mSource.distance(initial.mDestination)
                            - transition.mSource.distance(initial.mDestination);
                    if (distanceDifference >= btScalar(0.5))
                    {
                        final = transition;
                    }

                    if (iterations++ >= mConfig.mMaxIterations)
                    {
                        DEBUG_LOG << "max iterations\n";
                        break;
                    }

                    mOpenSet.pop();
                    mClosedSet.insert(transition);

                    if (transition.mDepth >= mConfig.mMaxDepth)
                    {
                        DEBUG_LOG << "max depth\n";
                        continue;
                    }

                    for (const auto& newTransition : mVisitor.apply(transition))
                    {
                        if (newTransition.mState != Transition::State::Traced
                            && newTransition.mState != Transition::State::SubTraced)
                        {
                            push(newTransition);
                            continue;
                        }
                        if (const auto applyResult = applyTransition(newTransition, initial.mSource, initial.mDestination))
                        {
                            if (newTransition.mState == Transition::State::Traced)
                            {
                                push(applyResult.get());
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                JSON_LOG << withType(final, "final") << '\n';
                DEBUG_LOG << "final"
                    << " " << final
                    << " distance: " << final.mSource.distance(initial.mDestination)
                    << " iterations: " << iterations
                    << '\n';

                return OptimalPath {iterations > mConfig.mMaxIterations, iterations,
                        reconstructPath(initial.mSource, final.mSource, mCameFrom)};
            }

        private:
            enum class CheckCollision
            {
                NotFound,
                Found,
                Undefined,
            };

            const FindOptimalPathConfig& mConfig;
            Visitor& mVisitor;
            const btScalar mHorizontalMargin = std::max(mConfig.mActorHalfExtents.x(), mConfig.mActorHalfExtents.y())
                    * mConfig.mHorizontalMarginFactor;

            OpenSet mOpenSet;
            ClosedSet mClosedSet;
            std::map<PointInt, btScalar> mCosts;
            std::map<btVector3, btVector3, PointLess> mCameFrom;
            std::unordered_set<const btCollisionObject*> mOutput;

            boost::optional<Transition> applyTransition(const Transition& transition, const btVector3& initial, const btVector3& goal)
            {
                if (transition.mSource != initial && !mCameFrom.count(transition.mSource))
                {
                    DEBUG_LOG << "ignore apply " << transition << " reason: no path to source\n";
                    return boost::none;
                }

                const auto otherCost = mCosts.find(transition.mDestinationInt);
                const auto cost = getTentativeCost(transition.mCost, transition.mSource, transition.mDestination);

                if (otherCost == mCosts.end())
                {
                    mCosts.insert({transition.mDestinationInt, cost});
                }
                else if (otherCost->second > cost)
                {
                    otherCost->second = cost;
                }
                else
                {
                    DEBUG_LOG << "ignore apply " << transition << " reason: expensive cost\n";
                    return boost::none;
                }

                {
                    const auto direction = (transition.mDestination - transition.mSource).normalized();
                    DEBUG_LOG << "apply"
                        << " " << transition
                        << " depth: " << transition.mDepth
                        << " angle: " << direction.dot(btVector3(direction.x(), direction.y(), 0))
                        << '\n';
                }
                JSON_LOG << withType(transition, "apply") << '\n';

                mCameFrom[transition.mDestination] = transition.mSource;

                return Transition {
                    mVisitor.getNextTransitionId(),
                    transition.mId,
                    mVisitor.getPriority(cost, transition.mDestination, goal),
                    cost,
                    transition.mDepth + 1,
                    transition.mDestination,
                    goal,
                    transition.mDestinationInt,
                    mVisitor.makePointInt(goal),
                    Transition::State::Proposed,
                };
            }

            std::vector<btVector3> reconstructPath(const btVector3& source, const btVector3& final,
                    const std::map<btVector3, btVector3, PointLess>& cameFrom) const
            {
                std::vector<btVector3> result;
                result.reserve(cameFrom.size());
                auto position = final;
                while (position != source)
                {
                    result.push_back(position);
                    try
                    {
                        position = cameFrom.at(position);
                    }
                    catch (const std::exception& e)
                    {
                        std::ostringstream stream;
                        stream << "Path is broken: position "
                            << std::setprecision(std::numeric_limits<btScalar>::digits)
                            << "(" << position.x() << ", " << position.y() << ", " << position.z() << ")"
                            << " not found";
                        throw std::logic_error(stream.str());
                    }
                }
                std::reverse(result.begin(), result.end());
                return result;
            }

            void push(const Transition& transition)
            {
                if (!mClosedSet.contains(transition))
                {
                    mOpenSet.push(transition);
                }
                else
                {
                    DEBUG_LOG << "ignore push " << transition << " reason: already in closed" << '\n';
                }
            }
        };
    }
}

#endif // OPENMW_MWMECHANICS_FINDOPTIMALPATH_ALGORITHM_H
