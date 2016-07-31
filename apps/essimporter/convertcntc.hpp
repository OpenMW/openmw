#ifndef OPENMW_ESSIMPORT_CONVERTCNTC_H
#define OPENMW_ESSIMPORT_CONVERTCNTC_H

#include "importcntc.hpp"

#include <components/esm/containerstate.hpp>

namespace ESSImport
{

    void convertCNTC(const CNTC& cntc, ESM::ContainerState& state);

}

#endif
