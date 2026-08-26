#ifndef GAME_SOUND_HEADCACHE_H
#define GAME_SOUND_HEADCACHE_H

#include <cstddef>
#include <functional>
#include <ios>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <components/files/istreamptr.hpp>
#include <components/vfs/pathutil.hpp>

namespace VFS
{
    class Manager;
}

namespace MWSound
{
    // The bytes cached for one file: a prefix, plus a suffix when stream init
    // also read near the end (mp3 estimates duration from the tail). The name
    // travels with them because a stream serving the ranges opens the file
    // itself when something reads into the gap between them.
    struct HeadBuffer
    {
        VFS::Path::Normalized mName;
        std::vector<char> mHead;
        std::vector<char> mSuffix;
        std::streamoff mSuffixStart;
        std::streamoff mFileSize;

        HeadBuffer(VFS::Path::Normalized&& name, std::vector<char>&& head, std::vector<char>&& suffix,
            std::streamoff suffixStart, std::streamoff fileSize)
            : mName(std::move(name))
            , mHead(std::move(head))
            , mSuffix(std::move(suffix))
            , mSuffixStart(suffixStart)
            , mFileSize(fileSize)
        {
        }
    };

    // Serves the cached ranges from memory. The file is opened lazily on the
    // first read into the gap between them, which happens on the streaming
    // thread during a refill, or never when the ranges cover the whole file.
    Files::IStreamPtr makeHeadStream(std::shared_ptr<const HeadBuffer>&& buffer, const VFS::Manager& vfs);

    // Passes through to the real stream and remembers which ranges stream init
    // read, which is what HeadCache::insert sizes an entry from.
    Files::IStreamPtr makeRecordingStream(Files::IStreamPtr&& impl);

    // Keeps the first bytes of recently started sounds in memory so that a
    // sound start does not wait for storage on the thread starting it: stream
    // init is served from the cached head and the first disk read happens
    // later, during the streaming thread's refill (#4880). Storage waking from
    // a low power state costs 10-50ms on NVMe and seconds on spun-down drives,
    // paid by the first read after idle; opening a file costs nothing.
    //
    // Cached ranges are learned per file from what stream init consumed.
    // Ranges that would exceed a fixed ceiling are not cached. When the ranges
    // cover a whole file (most voice files), replays never touch the
    // filesystem.
    class HeadCache
    {
    public:
        // maxBytes: total budget for cached heads; entries are evicted least
        // recently used. A zero budget caches nothing.
        explicit HeadCache(const VFS::Manager& vfs, std::size_t maxBytes);

        // The bytes cached for the file, or nullptr when it has none. Entries
        // are shared with the streams serving them, so eviction cannot
        // invalidate a stream in use.
        std::shared_ptr<const HeadBuffer> lookup(VFS::Path::NormalizedView name);

        // Captures the head of a stream from makeRecordingStream, once stream
        // init has succeeded on it. Throws when handed any other stream.
        void insert(VFS::Path::NormalizedView name, const std::istream& stream);

    private:
        using LruIt = std::list<std::shared_ptr<const HeadBuffer>>::iterator;

        void insert(VFS::Path::NormalizedView name, std::vector<char>&& head, std::vector<char>&& suffix,
            std::streamoff suffixStart, std::streamoff fileSize);

        const VFS::Manager& mVfs;
        const std::size_t mMaxBytes;
        std::mutex mMutex;
        // The buffer lives in the list so a lookup reaches it through the map
        // once; the map value is the list iterator that keeps it there.
        std::list<std::shared_ptr<const HeadBuffer>> mLru;
        std::unordered_map<VFS::Path::Normalized, LruIt, VFS::Path::Hash, std::equal_to<>> mEntries;
        std::size_t mBytes = 0;
    };
}

#endif
