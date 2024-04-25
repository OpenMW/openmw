#ifndef GAME_SOUND_SOUND_BUFFER_H
#define GAME_SOUND_SOUND_BUFFER_H

#include <algorithm>
#include <deque>
#include <string>
#include <unordered_map>

#include "sound_output.hpp"
#include <components/esm/refid.hpp>

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
    class SoundBufferPool;

    class Sound_Buffer
    {
    public:
        template <class T>
        Sound_Buffer(T&& resname, float volume, float mindist, float maxdist)
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
        SoundBufferPool(Sound_Output& output);

        SoundBufferPool(const SoundBufferPool&) = delete;

        ~SoundBufferPool();

        /// Lookup a soundId for its sound data (resource name, local volume,
        /// minRange, and maxRange)
        Sound_Buffer* lookup(const ESM::RefId& soundId) const;

        /// Lookup a sound by file name for its sound data (resource name, local volume,
        /// minRange, and maxRange)
        Sound_Buffer* lookup(std::string_view fileName) const;

        /// Lookup a soundId for its sound data (resource name, local volume,
        /// minRange, and maxRange), and ensure it's ready for use.
        Sound_Buffer* load(const ESM::RefId& soundId);

        // Lookup for a sound by file name, and ensure it's ready for use.
        Sound_Buffer* load(std::string_view fileName);

        void use(Sound_Buffer& sfx)
        {
            if (sfx.mUses++ == 0)
            {
                const auto it = std::find(mUnusedBuffers.begin(), mUnusedBuffers.end(), &sfx);
                if (it != mUnusedBuffers.end())
                    mUnusedBuffers.erase(it);
            }
        }

        void release(Sound_Buffer& sfx)
        {
            if (--sfx.mUses == 0)
                mUnusedBuffers.push_front(&sfx);
        }

        void clear();

    private:
        Sound_Buffer* loadSfx(Sound_Buffer* sfx);

        Sound_Output* mOutput;
        std::deque<Sound_Buffer> mSoundBuffers;
        std::unordered_map<ESM::RefId, Sound_Buffer*> mBufferNameMap;
        std::unordered_map<std::string, Sound_Buffer*> mBufferFileNameMap;
        std::size_t mBufferCacheMax;
        std::size_t mBufferCacheMin;
        std::size_t mBufferCacheSize = 0;
        // NOTE: unused buffers are stored in front-newest order.
        std::deque<Sound_Buffer*> mUnusedBuffers;

        inline Sound_Buffer* insertSound(const ESM::RefId& soundId, const ESM::Sound& sound);
        inline Sound_Buffer* insertSound(std::string_view fileName);

        inline void unloadUnused();
    };
}

#endif /* GAME_SOUND_SOUND_BUFFER_H */
