#ifndef OPENMW_ESM_LOCALS_H
#define OPENMW_ESM_LOCALS_H

#include <vector>
#include <string>

#include "variant.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    /// \brief Storage structure for local variables (only used in saved games)
    ///
    /// \note This is not a top-level record.

    struct Locals
    {
        std::vector<std::pair<std::string, Variant> > mVariables;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif
