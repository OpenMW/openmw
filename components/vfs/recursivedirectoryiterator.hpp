#ifndef OPENMW_COMPONENTS_VFS_RECURSIVEDIRECTORYITERATOR_H
#define OPENMW_COMPONENTS_VFS_RECURSIVEDIRECTORYITERATOR_H

#include <string>

#include "filemap.hpp"
#include "pathutil.hpp"

namespace VFS
{
    class RecursiveDirectoryIterator
    {
    public:
        RecursiveDirectoryIterator(FileMap::const_iterator it)
            : mIt(it)
        {
        }

        const Path::Normalized& operator*() const { return mIt->first; }

        const Path::Normalized* operator->() const { return &mIt->first; }

        RecursiveDirectoryIterator& operator++()
        {
            ++mIt;
            return *this;
        }

        friend bool operator==(const RecursiveDirectoryIterator& lhs, const RecursiveDirectoryIterator& rhs) = default;

    private:
        FileMap::const_iterator mIt;
    };

    class RecursiveDirectoryRange
    {
    public:
        RecursiveDirectoryRange(RecursiveDirectoryIterator first, RecursiveDirectoryIterator last)
            : mBegin(first)
            , mEnd(last)
        {
        }

        RecursiveDirectoryIterator begin() const { return mBegin; }

        RecursiveDirectoryIterator end() const { return mEnd; }

    private:
        RecursiveDirectoryIterator mBegin;
        RecursiveDirectoryIterator mEnd;
    };
}

#endif
