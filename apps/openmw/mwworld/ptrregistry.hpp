#ifndef OPENMW_APPS_OPENMW_MWWORLD_PTRREGISTRY_H
#define OPENMW_APPS_OPENMW_MWWORLD_PTRREGISTRY_H

#include "ptr.hpp"

#include "components/esm3/cellref.hpp"

#include <unordered_map>

namespace MWWorld
{
    class PtrRegistry
    {
    public:
        std::size_t getRevision() const { return mRevision; }

        ESM::RefNum getLastGenerated() const { return mLastGenerated; }

        auto begin() const { return mIndex.cbegin(); }

        auto end() const { return mIndex.cend(); }

        Ptr getOrDefault(ESM::RefNum refNum) const
        {
            const auto it = mIndex.find(refNum);
            if (it != mIndex.end())
                return it->second;
            return Ptr();
        }

        void setLastGenerated(ESM::RefNum v) { mLastGenerated = v; }

        void clear()
        {
            mIndex.clear();
            mLastGenerated = ESM::RefNum{};
            ++mRevision;
        }

        void insert(const Ptr& ptr)
        {
            mIndex[ptr.getCellRef().getOrAssignRefNum(mLastGenerated)] = ptr;
            ++mRevision;
        }

        void remove(const Ptr& ptr)
        {
            mIndex.erase(ptr.getCellRef().getRefNum());
            ++mRevision;
        }

    private:
        std::size_t mRevision = 0;
        std::unordered_map<ESM::RefNum, Ptr> mIndex;
        ESM::RefNum mLastGenerated;
    };
}

#endif
