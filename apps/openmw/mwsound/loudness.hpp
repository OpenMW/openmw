#ifndef GAME_SOUND_LOUDNESS_H
#define GAME_SOUND_LOUDNESS_H

#include <vector>
#include <deque>

#include "sound_decoder.hpp"

namespace MWSound
{

class Sound_Loudness {
    float mSamplesPerSec;
    int mSampleRate;
    ChannelConfig mChannelConfig;
    SampleType mSampleType;

    // Loudness sample info
    std::vector<float> mSamples;

    std::deque<char> mQueue;

public:
    /**
     * @param samplesPerSecond How many loudness values per second of audio to compute.
     * @param sampleRate the sample rate of the sound buffer
     * @param chans channel layout of the buffer
     * @param type sample type of the buffer
    */
    Sound_Loudness(float samplesPerSecond, int sampleRate, ChannelConfig chans, SampleType type)
        : mSamplesPerSec(samplesPerSecond)
        , mSampleRate(sampleRate)
        , mChannelConfig(chans)
        , mSampleType(type)
    { }

    /**
     * Analyzes the energy (closely related to loudness) of a sound buffer.
     * The buffer will be divided into segments according to \a valuesPerSecond,
     * and for each segment a loudness value in the range of [0,1] will be computed.
     * The computed values are then added to the mSamples vector. This method should be called continuously
     * with chunks of audio until the whole audio file is processed.
     * If the size of \a data does not exactly fit a number of loudness samples, the remainder
     * will be kept in the mQueue and used in the next call to analyzeLoudness.
     * @param data the sound buffer to analyze, containing raw samples
     */
    void analyzeLoudness(const std::vector<char>& data);

    /**
     * Get loudness at a particular time. Before calling this, the stream has to be analyzed up to that point in time (see analyzeLoudness()).
     */
    float getLoudnessAtTime(float sec) const;
};

}

#endif /* GAME_SOUND_LOUDNESS_H */
