#include "headcache.hpp"

#include <algorithm>
#include <cstring>
#include <istream>
#include <limits>
#include <stdexcept>

#include <components/files/streamwithbuffer.hpp>
#include <components/vfs/manager.hpp>

namespace MWSound
{
    namespace
    {
        // Both buffers implement only the block-read and seek subset of the
        // std::streambuf protocol that FFmpegDecoder uses (read, gcount,
        // seekg, tellg); single-character access is unsupported by design.

        // Per-file ceiling. Measured stream init consumption tops out under
        // 100KB across vanilla and heavy audio mods once embedded pictures are
        // out of the way; files whose init reads more than this are not cached.
        constexpr std::streamoff sMaxHeadBytes = 256 * 1024;

        // Reads in the lower half of the file extend the prefix; reads in the
        // upper half accumulate a suffix range, so both regions can be cached
        // and served next time.
        class RecordingBuf final : public std::streambuf
        {
        public:
            explicit RecordingBuf(Files::IStreamPtr inner)
                : mInner(std::move(inner))
            {
            }

            std::streamoff prefixEnd() const { return mPrefixEnd; }
            std::streamoff suffixStart() const { return mSuffixStart; }

        protected:
            std::streamsize xsgetn(char* s, std::streamsize n) override
            {
                mInner->clear();
                mInner->read(s, n);
                const std::streamsize got = mInner->gcount();
                const std::streamoff begin = mPos;
                mPos += got;
                if (mFileSize > 0 && begin >= mFileSize / 2)
                    mSuffixStart = std::min(mSuffixStart, begin);
                else
                    mPrefixEnd = std::max(mPrefixEnd, mPos);
                return got;
            }

            std::streampos seekoff(std::streamoff off, std::ios_base::seekdir dir, std::ios_base::openmode) override
            {
                mInner->clear();
                mInner->seekg(off, dir);
                if (!*mInner)
                    return std::streampos(std::streamoff(-1));
                mPos = mInner->tellg();
                if (dir == std::ios_base::end)
                    mFileSize = std::max(mFileSize, mPos - off);
                setg(nullptr, nullptr, nullptr);
                return mPos;
            }

            std::streampos seekpos(std::streampos pos, std::ios_base::openmode mode) override
            {
                return seekoff(static_cast<std::streamoff>(pos), std::ios_base::beg, mode);
            }

        private:
            Files::IStreamPtr mInner;
            std::streamoff mPos = 0;
            std::streamoff mPrefixEnd = 0;
            std::streamoff mSuffixStart = std::numeric_limits<std::streamoff>::max();
            std::streamoff mFileSize = 0;
        };

        class HeadBuf final : public std::streambuf
        {
        public:
            HeadBuf(std::shared_ptr<const HeadBuffer>&& buffer, const VFS::Manager& vfs)
                : mBuffer(std::move(buffer))
                , mVfs(vfs)
            {
            }

        protected:
            std::streamsize xsgetn(char* s, std::streamsize n) override
            {
                std::streamsize done = 0;
                while (done < n && mPos < mBuffer->mFileSize)
                {
                    const std::streamoff headSize = static_cast<std::streamoff>(mBuffer->mHead.size());
                    if (mPos < headSize)
                    {
                        const std::streamsize take = std::min<std::streamsize>(n - done, headSize - mPos);
                        std::memcpy(s + done, mBuffer->mHead.data() + mPos, take);
                        done += take;
                        mPos += take;
                        continue;
                    }
                    if (!mBuffer->mSuffix.empty() && mPos >= mBuffer->mSuffixStart)
                    {
                        const std::streamoff inSuffix = mPos - mBuffer->mSuffixStart;
                        const std::streamsize take = std::min<std::streamsize>(
                            n - done, static_cast<std::streamoff>(mBuffer->mSuffix.size()) - inSuffix);
                        if (take <= 0)
                            break;
                        std::memcpy(s + done, mBuffer->mSuffix.data() + inSuffix, take);
                        done += take;
                        mPos += take;
                        continue;
                    }
                    if (mInner == nullptr)
                        mInner = mVfs.get(mBuffer->mName);
                    mInner->clear();
                    mInner->seekg(mPos);
                    const std::streamsize want = !mBuffer->mSuffix.empty() && mPos < mBuffer->mSuffixStart
                        ? std::min<std::streamsize>(n - done, mBuffer->mSuffixStart - mPos)
                        : n - done;
                    mInner->read(s + done, want);
                    const std::streamsize got = mInner->gcount();
                    if (got <= 0)
                        break;
                    done += got;
                    mPos += got;
                }
                return done;
            }

            std::streampos seekoff(std::streamoff off, std::ios_base::seekdir dir, std::ios_base::openmode) override
            {
                std::streamoff base = 0;
                if (dir == std::ios_base::cur)
                    base = mPos;
                else if (dir == std::ios_base::end)
                    base = mBuffer->mFileSize;
                const std::streamoff target = base + off;
                // Reject seeks past the end like the constrained streams the
                // file is served from on a miss, so replays behave the same.
                if (target < 0 || target > mBuffer->mFileSize)
                    return std::streampos(std::streamoff(-1));
                mPos = target;
                setg(nullptr, nullptr, nullptr);
                return mPos;
            }

            std::streampos seekpos(std::streampos pos, std::ios_base::openmode mode) override
            {
                return seekoff(static_cast<std::streamoff>(pos), std::ios_base::beg, mode);
            }

        private:
            std::shared_ptr<const HeadBuffer> mBuffer;
            const VFS::Manager& mVfs;
            Files::IStreamPtr mInner;
            std::streamoff mPos = 0;
        };
    }

    Files::IStreamPtr makeHeadStream(std::shared_ptr<const HeadBuffer>&& buffer, const VFS::Manager& vfs)
    {
        return std::make_unique<Files::StreamWithBuffer<HeadBuf>>(std::make_unique<HeadBuf>(std::move(buffer), vfs));
    }

    Files::IStreamPtr makeRecordingStream(Files::IStreamPtr&& impl)
    {
        return std::make_unique<Files::StreamWithBuffer<RecordingBuf>>(std::make_unique<RecordingBuf>(std::move(impl)));
    }

    HeadCache::HeadCache(const VFS::Manager& vfs, std::size_t maxBytes)
        : mVfs(vfs)
        , mMaxBytes(maxBytes)
    {
    }

    std::shared_ptr<const HeadBuffer> HeadCache::lookup(VFS::Path::NormalizedView name)
    {
        const std::lock_guard lock(mMutex);
        const auto it = mEntries.find(name);
        if (it == mEntries.end())
            return nullptr;
        mLru.splice(mLru.begin(), mLru, it->second);
        return *it->second;
    }

    void HeadCache::insert(VFS::Path::NormalizedView name, const std::istream& stream)
    {
        const auto* const recording = dynamic_cast<const RecordingBuf*>(stream.rdbuf());
        if (recording == nullptr)
            throw std::invalid_argument("HeadCache::insert: stream is not from makeRecordingStream");
        const std::streamoff prefixEnd = recording->prefixEnd();
        if (prefixEnd <= 0 || prefixEnd > sMaxHeadBytes)
            return;

        // The file was just read by stream init, so this re-open and these
        // copies are served from the OS caches; storage is not touched again.
        Files::IStreamPtr fresh = mVfs.get(name);
        fresh->seekg(0, std::ios_base::end);
        const std::streamoff fileSize = fresh->tellg();
        if (fileSize <= 0)
            return;
        std::vector<char> head(static_cast<std::size_t>(std::min(prefixEnd, fileSize)));
        fresh->seekg(0);
        fresh->read(head.data(), head.size());
        if (fresh->gcount() != static_cast<std::streamsize>(head.size()))
            return;

        std::vector<char> suffix;
        std::streamoff suffixStart = fileSize;
        if (recording->suffixStart() < fileSize)
        {
            suffixStart = recording->suffixStart();
            if (fileSize - suffixStart > sMaxHeadBytes)
                return;
            suffix.resize(static_cast<std::size_t>(fileSize - suffixStart));
            fresh->clear();
            fresh->seekg(suffixStart);
            fresh->read(suffix.data(), suffix.size());
            if (fresh->gcount() != static_cast<std::streamsize>(suffix.size()))
                return;
        }

        insert(name, std::move(head), std::move(suffix), suffixStart, fileSize);
    }

    void HeadCache::insert(VFS::Path::NormalizedView name, std::vector<char>&& head, std::vector<char>&& suffix,
        std::streamoff suffixStart, std::streamoff fileSize)
    {
        const std::size_t bytes = head.size() + suffix.size();
        // Nothing is allocated until every way out has been taken: an entry
        // bigger than the whole budget can never fit, and evicting for it
        // would empty the cache for no purpose.
        if (bytes > mMaxBytes)
            return;
        const std::lock_guard lock(mMutex);
        if (mEntries.contains(name))
            return;
        while (!mLru.empty() && mBytes + bytes > mMaxBytes)
        {
            const HeadBuffer& evicted = *mLru.back();
            mBytes -= evicted.mHead.size() + evicted.mSuffix.size();
            mEntries.erase(evicted.mName);
            mLru.pop_back();
        }
        const LruIt lruIt = mLru.insert(mLru.begin(),
            std::make_shared<const HeadBuffer>(
                VFS::Path::Normalized(name), std::move(head), std::move(suffix), suffixStart, fileSize));
        mEntries.emplace((*lruIt)->mName, lruIt);
        mBytes += bytes;
    }
}
