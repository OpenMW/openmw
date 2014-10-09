#include "land.hpp"

#include <sstream>

namespace CSMWorld
{

    Land::Land()
    {
        mLand.reset(new ESM::Land());
    }

    void Land::load(ESM::ESMReader &esm)
    {
        mLand->load(esm);

        std::ostringstream stream;
        stream << "#" << mLand->mX << " " << mLand->mY;

        mId = stream.str();
    }

    void Land::blank()
    {
        /// \todo
    }

}
