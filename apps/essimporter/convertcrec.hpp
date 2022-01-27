#ifndef OPENMW_ESSIMPORT_CONVERTCREC_H
#define OPENMW_ESSIMPORT_CONVERTCREC_H

#include "importcrec.hpp"

#include <components/esm3/creaturestate.hpp>

namespace ESSImport
{

    void convertCREC(const CREC& crec, ESM::CreatureState& state);

}

#endif
