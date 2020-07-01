#ifndef GAME_SOUND_VOLUMESETTINGS_H
#define GAME_SOUND_VOLUMESETTINGS_H

#include "type.hpp"

namespace MWSound
{
    class VolumeSettings
    {
        public:
            VolumeSettings();

            float getVolumeFromType(Type type) const;

            void update();

        private:
            float mMasterVolume;
            float mSFXVolume;
            float mMusicVolume;
            float mVoiceVolume;
            float mFootstepsVolume;
    };
}

#endif
