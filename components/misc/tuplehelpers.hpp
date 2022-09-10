#ifndef OPENMW_COMPONENTS_MISC_TUPLEHELPER_H
#define OPENMW_COMPONENTS_MISC_TUPLEHELPER_H

#include <tuple>

namespace Misc
{
    template <typename TupleType, typename Callable>
    void tupleForEach(TupleType& tuple, Callable&& f)
    {
        std::apply([&f](auto&... x) { (f(x), ...); }, tuple);
    }
}

#endif
