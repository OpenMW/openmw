#ifndef GAME_SOUND_SOUNDBUFFER_H
#define GAME_SOUND_SOUNDBUFFER_H

#include <algorithm>
#include <deque>
#include <unordered_map>

#include <components/esm/refid.hpp>
#include <components/vfs/pathutil.hpp>

#include "soundoutput.hpp"

namespace ESM
{
    struct Sound;
}

namespace ESM4
{
    struct Sound;
    struct SoundReference;
}

namespace VFS
{
    class Manager;
}

namespace MWSound
{
    class SoundBufferPool;

    class SoundBuffer
    {
    public:
        template <class T>
        SoundBuffer(T&& resname, float volume, float mindist, float maxdist)
            : mResourceName(std::forward<T>(resname))
            , mVolume(volume)
            , mMinDist(mindist)
            , mMaxDist(maxdist)
        {
        }

        const VFS::Path::Normalized& getResourceName() const noexcept { return mResourceName; }

        Sound_Handle getHandle() const noexcept { return mHandle; }

        float getVolume() const noexcept { return mVolume; }

        float getMinDist() const noexcept { return mMinDist; }

        float getMaxDist() const noexcept { return mMaxDist; }

    private:
        VFS::Path::Normalized mResourceName;
        float mVolume;
        float mMinDist;
        float mMaxDist;
        Sound_Handle mHandle = nullptr;
        std::size_t mUses = 0;

        friend class SoundBufferPool;
    };

    class SoundBufferPool
    {
    public:
        SoundBufferPool(SoundOutput& output);

        SoundBufferPool(const SoundBufferPool&) = delete;

        ~SoundBufferPool();

        /// Lookup a soundId for its sound data (resource name, local volume,
        /// minRange, and maxRange)
        SoundBuffer* lookup(const ESM::RefId& soundId) const;

        /// Lookup a sound by file name for its sound data (resource name, local volume,
        /// minRange, and maxRange)
        SoundBuffer* lookup(VFS::Path::NormalizedView fileName) const;

        /// Lookup a soundId for its sound data (resource name, local volume,
        /// minRange, and maxRange), and ensure it's ready for use.
        SoundBuffer* load(const ESM::RefId& soundId);

        // Lookup for a sound by file name, and ensure it's ready for use.
        SoundBuffer* load(VFS::Path::NormalizedView fileName);

        void use(SoundBuffer& sfx)
        {
            if (sfx.mUses++ == 0)
            {
                const auto it = std::find(mUnusedBuffers.begin(), mUnusedBuffers.end(), &sfx);
                if (it != mUnusedBuffers.end())
                    mUnusedBuffers.erase(it);
            }
        }

        void release(SoundBuffer& sfx)
        {
            if (--sfx.mUses == 0)
                mUnusedBuffers.push_front(&sfx);
        }

        void clear();

    private:
        SoundBuffer* loadSfx(SoundBuffer* sfx);

        SoundOutput* mOutput;
        std::deque<SoundBuffer> mSoundBuffers;
        std::unordered_map<ESM::RefId, SoundBuffer*> mBufferNameMap;
        std::unordered_map<VFS::Path::Normalized, SoundBuffer*, VFS::Path::Hash, std::equal_to<>> mBufferFileNameMap;
        std::size_t mBufferCacheMax;
        std::size_t mBufferCacheMin;
        std::size_t mBufferCacheSize = 0;
        // NOTE: unused buffers are stored in front-newest order.
        std::deque<SoundBuffer*> mUnusedBuffers;

        SoundBuffer* insertSound(const ESM::RefId& soundId, const ESM::Sound& sound);
        SoundBuffer* insertSound(const ESM::RefId& soundId, const ESM4::Sound& sound);
        SoundBuffer* insertSound(const ESM::RefId& soundId, const ESM4::SoundReference& sound);
        SoundBuffer* insertSound(VFS::Path::NormalizedView fileName);

        inline void unloadUnused();
    };
}

#endif /* GAME_SOUND_SOUNDBUFFER_H */
