#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SOUND_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SOUND_H

#include "components/settings/sanitizerimpl.hpp"
#include "components/settings/settingvalue.hpp"

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct SoundCategory
    {
        SettingValue<std::string> mDevice{ "Sound", "device" };
        SettingValue<float> mMasterVolume{ "Sound", "master volume", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mFootstepsVolume{ "Sound", "footsteps volume", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mMusicVolume{ "Sound", "music volume", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mSfxVolume{ "Sound", "sfx volume", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mVoiceVolume{ "Sound", "voice volume", makeClampSanitizerFloat(0, 1) };
        SettingValue<int> mBufferCacheMin{ "Sound", "buffer cache min", makeMaxSanitizerInt(1) };
        SettingValue<int> mBufferCacheMax{ "Sound", "buffer cache max", makeMaxSanitizerInt(1) };
        SettingValue<int> mHrtfEnable{ "Sound", "hrtf enable", makeEnumSanitizerInt({ -1, 0, 1 }) };
        SettingValue<std::string> mHrtf{ "Sound", "hrtf" };
    };
}

#endif
