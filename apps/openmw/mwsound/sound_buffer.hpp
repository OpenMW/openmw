#ifndef GAME_SOUND_SOUND_BUFFER_H
#define GAME_SOUND_SOUND_BUFFER_H

#include <algorithm>
#include <string>
#include <deque>
#include <unordered_map>

#include "sound_output.hpp"

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
                : mResourceName(std::forward<T>(resname)), mVolume(volume), mMinDist(mindist), mMaxDist(maxdist)
            {}

            const std::string& getResourceName() const noexcept { return mResourceName; }

            Sound_Handle getHandle() const noexcept { return mHandle; }

            float getVolume() const noexcept { return mVolume; }

            float getMinDist() const noexcept { return mMinDist; }

            float getMaxDist() const noexcept { return mMaxDist; }

        private:
            std::string mResourceName;
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
            SoundBufferPool(const VFS::Manager& vfs, Sound_Output& output);

            SoundBufferPool(const SoundBufferPool&) = delete;

            ~SoundBufferPool();

            /// Lookup a soundId for its sound data (resource name, local volume,
            /// minRange, and maxRange)
            Sound_Buffer* lookup(const std::string& soundId) const;

            /// Lookup a soundId for its sound data (resource name, local volume,
            /// minRange, and maxRange), and ensure it's ready for use.
            Sound_Buffer* load(const std::string& soundId);

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
            const VFS::Manager* const mVfs;
            Sound_Output* mOutput;
            std::deque<Sound_Buffer> mSoundBuffers;
            std::unordered_map<std::string, Sound_Buffer*> mBufferNameMap;
            std::size_t mBufferCacheMax;
            std::size_t mBufferCacheMin;
            std::size_t mBufferCacheSize = 0;
            // NOTE: unused buffers are stored in front-newest order.
            std::deque<Sound_Buffer*> mUnusedBuffers;

            inline Sound_Buffer* insertSound(const std::string& soundId, const ESM::Sound& sound);

            inline void unloadUnused();
    };
}

#endif /* GAME_SOUND_SOUND_BUFFER_H */
