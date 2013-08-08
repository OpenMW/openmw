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
        enum Scope
        {
            Scope_Global = 0, // per user
            Scope_Project = 1, // per project
            Scope_Session = 2, // exists only for one editing session; not saved
            Scope_Content = 3 // embedded in the edited content file
        };

        Scope mScope;
    };
}

#endif