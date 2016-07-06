#include "loudness.hpp"

#include <stdint.h>
#include <limits>

#include "soundmanagerimp.hpp"

namespace MWSound
{

void Sound_Loudness::analyzeLoudness(const std::vector< char >& data, int sampleRate, ChannelConfig chans, SampleType type, float valuesPerSecond)
{
    int samplesPerSegment = static_cast<int>(sampleRate / valuesPerSecond);
    int numSamples = bytesToFrames(data.size(), chans, type);
    int advance = framesToBytes(1, chans, type);

    mSamplesPerSec = valuesPerSecond;
    mSamples.clear();
    mSamples.reserve(numSamples/samplesPerSegment);

    int segment=0;
    int sample=0;
    while (segment < numSamples/samplesPerSegment)
    {
        float sum=0;
        int samplesAdded = 0;
        while (sample < numSamples && sample < (segment+1)*samplesPerSegment)
        {
            // get sample on a scale from -1 to 1
            float value = 0;
            if (type == SampleType_UInt8)
                value = ((char)(data[sample*advance]^0x80))/128.f;
            else if (type == SampleType_Int16)
            {
                value = *reinterpret_cast<const int16_t*>(&data[sample*advance]);
                value /= float(std::numeric_limits<int16_t>::max());
            }
            else if (type == SampleType_Float32)
            {
                value = *reinterpret_cast<const float*>(&data[sample*advance]);
                value = std::max(-1.f, std::min(1.f, value)); // Float samples *should* be scaled to [-1,1] already.
            }

            sum += value*value;
            ++samplesAdded;
            ++sample;
        }

        float rms = 0; // root mean square
        if (samplesAdded > 0)
            rms = std::sqrt(sum / samplesAdded);
        mSamples.push_back(rms);
        ++segment;
    }

    mReady = true;
}


float Sound_Loudness::getLoudnessAtTime(float sec) const
{
    if(mSamplesPerSec <= 0.0f || mSamples.empty() || sec < 0.0f)
        return 0.0f;

    size_t index = static_cast<size_t>(sec * mSamplesPerSec);
    index = std::max<size_t>(0, std::min(index, mSamples.size()-1));
    return mSamples[index];
}

}
