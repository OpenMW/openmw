#ifndef CSM_FILTER_FILTER_H
#define CSM_FILTER_FILTER_H

#include <vector>
#include <string>

#include <components/esm/filter.hpp>

namespace CSMFilter
{
    /// \brief Wrapper for Filter record
    struct Filter : public ESM::Filter
    {
    };
}

#endif