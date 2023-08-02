#ifndef OPENMW_COMPONENTS_ESM3_TIMESTAMP_H
#define OPENMW_COMPONENTS_ESM3_TIMESTAMP_H

#include <cstdint>

#include <components/esm/esmcommon.hpp>

namespace ESM
{
    class ESMReader;

    struct TimeStamp
    {
        float mHour;
        int32_t mDay;

        void load(ESMReader& esm, NAME name = "TIME");
    };
}

#endif
