#ifndef GAME_SOUND_SOUND_BUFFER_H
#define GAME_SOUND_SOUND_BUFFER_H

#include <string>

#include "soundmanagerimp.hpp"

#include "../mwworld/ptr.hpp"

namespace MWSound
{
    class Sound_Buffer
    {
    public:
        std::string mResourceName;

        float mVolume;
        float mMinDist, mMaxDist;

        Sound_Buffer(std::string resname, float volume, float mindist, float maxdist)
          : mResourceName(resname), mVolume(volume), mMinDist(mindist), mMaxDist(maxdist)
        { }
    };
}

#endif /* GAME_SOUND_SOUND_BUFFER_H */
