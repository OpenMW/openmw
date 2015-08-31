#include "land.hpp"

#include <sstream>

namespace CSMWorld
{
    void Land::load(ESM::ESMReader &esm)
    {
        ESM::Land::load(esm);

        std::ostringstream stream;
        stream << "#" << mX << " " << mY;

        mId = stream.str();
    }
}
