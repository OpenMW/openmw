#include "loudness.hpp"

#include "soundmanagerimp.hpp"

namespace MWSound
{

    void analyzeLoudness(const std::vector<char> &data, int sampleRate, ChannelConfig chans,
                         SampleType type, std::vector<float> &out, float valuesPerSecond)
    {
        int samplesPerSegment = sampleRate / valuesPerSecond;
        int numSamples = bytesToFrames(data.size(), chans, type);
        int advance = framesToBytes(1, chans, type);

        out.reserve(numSamples/samplesPerSegment);

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
                    value = data[sample*advance]/128.f;
                else if (type == SampleType_Int16)
                {
                    value = *reinterpret_cast<const Ogre::int16*>(&data[sample*advance]);
                    value /= float(std::numeric_limits<Ogre::uint16>().max());
                }
                else if (type == SampleType_Float32)
                {
                    value = *reinterpret_cast<const float*>(&data[sample*advance]);
                    value /= std::numeric_limits<float>().max();
                }

                sum += value*value;
                ++samplesAdded;
                ++sample;
            }

            float rms = 0; // root mean square
            if (samplesAdded > 0)
                rms = std::sqrt(sum / samplesAdded);
            out.push_back(rms);
            ++segment;
        }
    }

}
