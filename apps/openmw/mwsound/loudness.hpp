#include "sound_decoder.hpp"

namespace MWSound
{

/**
 * Analyzes the energy (closely related to loudness) of a sound buffer.
 * The buffer will be divided into segments according to \a valuesPerSecond,
 * and for each segment a loudness value in the range of [0,1] will be computed.
 * @param data the sound buffer to analyze, containing raw samples
 * @param sampleRate the sample rate of the sound buffer
 * @param chans channel layout of the buffer
 * @param type sample type of the buffer
 * @param out Will contain the output loudness values.
 * @param valuesPerSecond How many loudness values per second of audio to compute.
 */
void analyzeLoudness (const std::vector<char>& data, int sampleRate, ChannelConfig chans, SampleType type,
                      std::vector<float>& out, float valuesPerSecond);

}
