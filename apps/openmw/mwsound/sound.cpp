#include "sound.hpp"

namespace MWSound
{

    float Sound::getCurrentLoudness()
    {
        if (mLoudnessVector.empty())
            return 0.f;
        int index = static_cast<int>(getTimeOffset() * mLoudnessFPS);

        index = std::max(0, std::min(index, int(mLoudnessVector.size()-1)));

        return mLoudnessVector[index];
    }

    void Sound::setLoudnessVector(const std::vector<float> &loudnessVector, float loudnessFPS)
    {
        mLoudnessVector = loudnessVector;
        mLoudnessFPS = loudnessFPS;
    }

}
