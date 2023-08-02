#include "timestamp.hpp"

#include "esmreader.hpp"

namespace ESM
{
    void TimeStamp::load(ESMReader& esm, NAME name)
    {
        esm.getHNT(name, mHour, mDay);
    }
}
