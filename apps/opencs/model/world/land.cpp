#include "land.hpp"

#include <sstream>

namespace CSMWorld
{
    void Land::load(ESM::ESMReader &esm, bool &isDeleted)
    {
        mLand->load(esm, isDeleted);

        std::ostringstream stream;
        stream << "#" << mX << " " << mY;
        mId = stream.str();
    }
}
