#ifndef GAME_SOUND_LOUDNESS_H
#define GAME_SOUND_LOUDNESS_H

#include <vector>

#include "sound_decoder.hpp"

namespace MWSound
{

class Sound_Loudness {
    // Loudness sample info
    float mSamplesPerSec;
    std::vector<float> mSamples;
    volatile bool mReady;

public:
    Sound_Loudness() : mSamplesPerSec(0.0f), mReady(false) { }

    /**
     * Analyzes the energy (closely related to loudness) of a sound buffer.
     * The buffer will be divided into segments according to \a valuesPerSecond,
     * and for each segment a loudness value in the range of [0,1] will be computed.
     * @param data the sound buffer to analyze, containing raw samples
     * @param sampleRate the sample rate of the sound buffer
     * @param chans channel layout of the buffer
     * @param type sample type of the buffer
     * @param valuesPerSecond How many loudness values per second of audio to compute.
     */
    void analyzeLoudness(const std::vector<char>& data, int sampleRate,
                         ChannelConfig chans, SampleType type,
                         float valuesPerSecond);

    bool isReady() { return mReady; }
    float getLoudnessAtTime(float sec) const;
};

}

#endif /* GAME_SOUND_LOUDNESS_H */
