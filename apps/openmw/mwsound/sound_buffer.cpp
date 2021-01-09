#include "sound_buffer.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/settings.hpp>
#include <components/vfs/manager.hpp>

#include <algorithm>
#include <cmath>

namespace MWSound
{
    namespace
    {
        struct AudioParams
        {
            float mAudioDefaultMinDistance;
            float mAudioDefaultMaxDistance;
            float mAudioMinDistanceMult;
            float mAudioMaxDistanceMult;
        };

        AudioParams makeAudioParams(const MWBase::World& world)
        {
            const auto& settings = world.getStore().get<ESM::GameSetting>();
            AudioParams params;
            params.mAudioDefaultMinDistance = settings.find("fAudioDefaultMinDistance")->mValue.getFloat();
            params.mAudioDefaultMaxDistance = settings.find("fAudioDefaultMaxDistance")->mValue.getFloat();
            params.mAudioMinDistanceMult = settings.find("fAudioMinDistanceMult")->mValue.getFloat();
            params.mAudioMaxDistanceMult = settings.find("fAudioMaxDistanceMult")->mValue.getFloat();
            return params;
        }
    }

    SoundBufferPool::SoundBufferPool(const VFS::Manager& vfs, Sound_Output& output) :
        mVfs(&vfs),
        mOutput(&output),
        mBufferCacheMax(std::max(Settings::Manager::getInt("buffer cache max", "Sound"), 1) * 1024 * 1024),
        mBufferCacheMin(std::min(static_cast<std::size_t>(std::max(Settings::Manager::getInt("buffer cache min", "Sound"), 1)) * 1024 * 1024, mBufferCacheMax))
    {
    }

    SoundBufferPool::~SoundBufferPool()
    {
        clear();
    }

    Sound_Buffer* SoundBufferPool::lookup(const std::string& soundId) const
    {
        const auto it = mBufferNameMap.find(soundId);
        if (it != mBufferNameMap.end())
        {
            Sound_Buffer* sfx = it->second;
            if (sfx->getHandle() != nullptr)
                return sfx;
        }
        return nullptr;
    }

    Sound_Buffer* SoundBufferPool::load(const std::string& soundId)
    {
        if (mBufferNameMap.empty())
        {
            for (const ESM::Sound& sound : MWBase::Environment::get().getWorld()->getStore().get<ESM::Sound>())
                insertSound(Misc::StringUtils::lowerCase(sound.mId), sound);
        }

        Sound_Buffer* sfx;
        const auto it = mBufferNameMap.find(soundId);
        if (it != mBufferNameMap.end())
            sfx = it->second;
        else
        {
            const ESM::Sound *sound = MWBase::Environment::get().getWorld()->getStore().get<ESM::Sound>().search(soundId);
            if (sound == nullptr)
                return {};
            sfx = insertSound(soundId, *sound);
        }

        if (sfx->getHandle() == nullptr)
        {
            auto [handle, size] = mOutput->loadSound(sfx->getResourceName());
            if (handle == nullptr)
                return {};

            sfx->mHandle = handle;

            mBufferCacheSize += size;
            if (mBufferCacheSize > mBufferCacheMax)
            {
                unloadUnused();
                if (!mUnusedBuffers.empty() && mBufferCacheSize > mBufferCacheMax)
                    Log(Debug::Warning) << "No unused sound buffers to free, using " << mBufferCacheSize << " bytes!";
            }
            mUnusedBuffers.push_front(sfx);
        }

        return sfx;
    }

    void SoundBufferPool::clear()
    {
        for (auto &sfx : mSoundBuffers)
        {
            if(sfx.mHandle)
                mOutput->unloadSound(sfx.mHandle);
            sfx.mHandle = nullptr;
        }
        mUnusedBuffers.clear();
    }

    Sound_Buffer* SoundBufferPool::insertSound(const std::string& soundId, const ESM::Sound& sound)
    {
        static const AudioParams audioParams = makeAudioParams(*MWBase::Environment::get().getWorld());

        float volume = static_cast<float>(std::pow(10.0, (sound.mData.mVolume / 255.0 * 3348.0 - 3348.0) / 2000.0));
        float min = sound.mData.mMinRange;
        float max = sound.mData.mMaxRange;
        if (min == 0 && max == 0)
        {
            min = audioParams.mAudioDefaultMinDistance;
            max = audioParams.mAudioDefaultMaxDistance;
        }

        min *= audioParams.mAudioMinDistanceMult;
        max *= audioParams.mAudioMaxDistanceMult;
        min = std::max(min, 1.0f);
        max = std::max(min, max);

        Sound_Buffer& sfx = mSoundBuffers.emplace_back("Sound/" + sound.mSound, volume, min, max);
        mVfs->normalizeFilename(sfx.mResourceName);

        mBufferNameMap.emplace(soundId, &sfx);
        return &sfx;
    }

    void SoundBufferPool::unloadUnused()
    {
        while (!mUnusedBuffers.empty() && mBufferCacheSize > mBufferCacheMin)
        {
            Sound_Buffer* const unused = mUnusedBuffers.back();

            mBufferCacheSize -= mOutput->unloadSound(unused->getHandle());
            unused->mHandle = nullptr;

            mUnusedBuffers.pop_back();
        }
    }
}
