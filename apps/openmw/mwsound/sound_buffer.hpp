#ifndef GAME_SOUND_SOUND_BUFFER_H
#define GAME_SOUND_SOUND_BUFFER_H

#include <string>
#include <memory>
#include <list>
#include <unordered_map>

#include "sound_output.hpp"

#include <components/misc/objectpool.hpp>

namespace ESM
{
    struct Sound;
}

namespace VFS
{
    class Manager;
}

namespace MWSound
{
    struct SoundBufferParams
    {
        std::string mResourceName;
        float mVolume = 0;
        float mMinDist = 0;
        float mMaxDist = 0;
    };

    class SoundBufferPool;
    class SoundBufferSharedRef;
    class Sound_Buffer;

    using SoundBufferIterator = std::list<Sound_Buffer*>::const_iterator;

    class Sound_Buffer
    {
        public:
            void init(SoundBufferParams params)
            {
                mParams = std::move(params);
                mHandle = nullptr;
                mUses = 0;
            }

            const std::string& getResourceName() const noexcept { return mParams.mResourceName; }

            Sound_Handle getHandle() const noexcept { return mHandle; }

            void setHandle(Sound_Handle value) noexcept { mHandle = value; }

            float getVolume() const noexcept { return mParams.mVolume; }

            float getMinDist() const noexcept { return mParams.mMinDist; }

            float getMaxDist() const noexcept { return mParams.mMaxDist; }

            void setPool(SoundBufferPool& pool, SoundBufferIterator value) noexcept
            {
                mPool = &pool;
                mIterator = value;
            }

            inline SoundBufferSharedRef use() noexcept;

        private:
            SoundBufferParams mParams;
            Sound_Handle mHandle = nullptr;
            std::size_t mUses = 0;
            SoundBufferPool* mPool = nullptr;
            SoundBufferIterator mIterator;

            friend class SoundBufferSharedRef;
    };

    class SoundBufferPool
    {
        public:
            SoundBufferPool(const VFS::Manager& vfs, Sound_Output& output);

            ~SoundBufferPool();

            Sound_Buffer* lookup(const std::string& soundId) const;

            Sound_Buffer* load(const std::string& soundId);

            void acquire(SoundBufferIterator it) noexcept
            {
                mUsedBuffers.splice(mUsedBuffers.end(), mUnusedBuffers, it);
            }

            void release(SoundBufferIterator it) noexcept
            {
                mUnusedBuffers.splice(mUnusedBuffers.end(), mUsedBuffers, it);
            }

            void clear();

        private:
            const VFS::Manager* const mVfs;
            Sound_Output* mOutput;
            Misc::ObjectPool<Sound_Buffer> mSoundBuffers;
            std::unordered_map<std::string, Misc::ObjectPtr<Sound_Buffer>> mBufferNameMap;
            std::size_t mBufferCacheMin;
            std::size_t mBufferCacheMax;
            std::size_t mBufferCacheSize = 0;
            std::list<Sound_Buffer*> mUnusedBuffers;
            std::list<Sound_Buffer*> mUsedBuffers;

            inline Sound_Buffer* insertSound(const std::string& soundId, const ESM::Sound& sound);

            inline void unloadUnused();
    };

    class SoundBufferSharedRef
    {
        public:
            explicit SoundBufferSharedRef(Sound_Buffer& ref) noexcept
                : mRef(&ref)
            {
                if (mRef->mUses++ == 0)
                    mRef->mPool->acquire(mRef->mIterator);
            }

            SoundBufferSharedRef(const SoundBufferSharedRef& other) noexcept
                : mRef(other.mRef)
            {
                ++mRef->mUses;
            }

            SoundBufferSharedRef(SoundBufferSharedRef&& other) noexcept
                : mRef(other.mRef)
            {
                other.mRef = nullptr;
            }

            ~SoundBufferSharedRef() noexcept
            {
                if (mRef != nullptr && --mRef->mUses == 0)
                    mRef->mPool->release(mRef->mIterator);
            }

            void reset() noexcept
            {
                if (mRef != nullptr)
                {
                    mRef->mPool->release(mRef->mIterator);
                    mRef = nullptr;
                }
            }

            SoundBufferSharedRef& operator=(const SoundBufferSharedRef& other) noexcept
            {
                SoundBufferSharedRef copy(other);
                std::swap(mRef, copy.mRef);
                return *this;
            }

            SoundBufferSharedRef& operator=(SoundBufferSharedRef&& other) noexcept
            {
                SoundBufferSharedRef moved(std::move(other));
                std::swap(mRef, moved.mRef);
                return *this;
            }

            friend inline bool operator==(SoundBufferSharedRef lhs, Sound_Buffer* rhs) noexcept
            {
                return lhs.mRef == rhs;
            }

            friend inline bool operator!=(SoundBufferSharedRef lhs, Sound_Buffer* rhs) noexcept
            {
                return lhs.mRef != rhs;
            }

        private:
            Sound_Buffer* mRef;
    };

    inline SoundBufferSharedRef Sound_Buffer::use() noexcept { return SoundBufferSharedRef(*this); }
}

#endif /* GAME_SOUND_SOUND_BUFFER_H */
