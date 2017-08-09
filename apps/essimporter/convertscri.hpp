#ifndef OPENMW_ESSIMPORT_CONVERTSCRI_H
#define OPENMW_ESSIMPORT_CONVERTSCRI_H

#include "importscri.hpp"

#include <components/esm/locals.hpp>

namespace ESSImport
{

    /// Convert script variable assignments
    void convertSCRI (const SCRI& scri, ESM::Locals& locals);

}

#endif
