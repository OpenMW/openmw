#ifndef OPENMW_COMPONENTS_SPT_STREAM_HPP
#define OPENMW_COMPONENTS_SPT_STREAM_HPP

#include <components/files/istreamptr.hpp>

namespace Spt
{
    class SPTStream
    {
    public:
        explicit SPTStream(Files::IStreamPtr&& stream);
    };
}

#endif