#include "volumesettings.hpp"

#include <components/settings/settings.hpp>

#include <algorithm>

namespace MWSound
{
    namespace
    {
        float clamp(float value)
        {
            return std::clamp(value, 0.f, 1.f);
        }
    }

    VolumeSettings::VolumeSettings()
        : mMasterVolume(clamp(Settings::Manager::getFloat("master volume", "Sound"))),
          mSFXVolume(clamp(Settings::Manager::getFloat("sfx volume", "Sound"))),
          mMusicVolume(clamp(Settings::Manager::getFloat("music volume", "Sound"))),
          mVoiceVolume(clamp(Settings::Manager::getFloat("voice volume", "Sound"))),
          mFootstepsVolume(clamp(Settings::Manager::getFloat("footsteps volume", "Sound")))
    {
    }

    float VolumeSettings::getVolumeFromType(Type type) const
    {
        float volume = mMasterVolume;

        switch(type)
        {
            case Type::Sfx:
                volume *= mSFXVolume;
                break;
            case Type::Voice:
                volume *= mVoiceVolume;
                break;
            case Type::Foot:
                volume *= mFootstepsVolume;
                break;
            case Type::Music:
                volume *= mMusicVolume;
                break;
            case Type::Movie:
            case Type::Mask:
                break;
        }

        return volume;
    }

    void VolumeSettings::update()
    {
        *this = VolumeSettings();
    }
}
