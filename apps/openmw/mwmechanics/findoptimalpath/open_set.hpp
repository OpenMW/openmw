#ifndef OPENMW_MWMECHANICS_FINDOPTIMALPATH_OPENSET_H
#define OPENMW_MWMECHANICS_FINDOPTIMALPATH_OPENSET_H

#include "debug.hpp"

#include <map>
#include <set>

namespace MWMechanics
{
    namespace FindOptimalPath
    {
        class OpenSet
        {
        public:
            void push(const Transition& transition)
            {
                const auto pushed = mPushed.find({transition.mSourceInt, transition.mDestinationInt});
                if (pushed != mPushed.end())
                {
                    if (pushed->second->mPriority <= transition.mPriority)
                    {
                        DEBUG_LOG << "ignore " << transition << " reason: already pushed with not greater priority" << '\n';
                        return;
                    }
                    DEBUG_LOG << "repush " << transition << '\n';
                    JSON_LOG << withType(transition, "repush") << '\n';
                    mTransitions.erase(pushed->second);
                    pushed->second = mTransitions.insert(transition).first;
                }
                else
                {
                    DEBUG_LOG << "push " << transition << '\n';
                    JSON_LOG << withType(transition, "push") << '\n';
                    const auto it = mTransitions.insert(transition).first;
                    mPushed.insert({{transition.mSourceInt, transition.mDestinationInt}, it});
                }
            }

            const Transition& top() const
            {
                return *mTransitions.begin();
            }

            void pop()
            {
                mPushed.erase({mTransitions.begin()->mSourceInt, mTransitions.begin()->mDestinationInt});
                mTransitions.erase(mTransitions.begin());
            }

            bool empty() const
            {
                return mTransitions.empty();
            }

            void clear()
            {
                mTransitions.clear();
                mPushed.clear();
            }

            std::size_t size() const
            {
                return mTransitions.size();
            }

        private:
            using Transitions = std::set<Transition, LessByPriorityAndId>;

            Transitions mTransitions;
            std::map<std::pair<PointInt, PointInt>, Transitions::const_iterator> mPushed;
        };
    }
}

#endif // OPENMW_MWMECHANICS_FINDOPTIMALPATH_OPENSET_H
