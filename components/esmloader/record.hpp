#ifndef OPENMW_COMPONENTS_ESMLOADER_RECORD_H
#define OPENMW_COMPONENTS_ESMLOADER_RECORD_H

#include <components/esm3/loadcell.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace EsmLoader
{
    template <class T>
    struct Record
    {
        bool mDeleted;
        T mValue;

        template <class ... Args>
        explicit Record(bool deleted, Args&& ... args)
            : mDeleted(deleted)
            , mValue(std::forward<Args>(args) ...)
        {}
    };

    template <class T>
    using Records = std::vector<Record<T>>;

    template <class T, class GetKey>
    inline std::vector<T> prepareRecords(Records<T>& records, const GetKey& getKey)
    {
        const auto greaterByKey = [&] (const auto& l, const auto& r) { return getKey(r) < getKey(l); };
        const auto equalByKey = [&] (const auto& l, const auto& r) { return getKey(l) == getKey(r); };
        std::stable_sort(records.begin(), records.end(), greaterByKey);
        records.erase(std::unique(records.begin(), records.end(), equalByKey), records.end());
        std::reverse(records.begin(), records.end());
        std::vector<T> result;
        for (Record<T>& v : records)
            if (!v.mDeleted)
                result.emplace_back(std::move(v.mValue));
        return result;
    }
}

#endif
