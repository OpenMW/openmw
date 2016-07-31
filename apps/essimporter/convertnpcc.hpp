#ifndef OPENMW_ESSIMPORT_CONVERTNPCC_H
#define OPENMW_ESSIMPORT_CONVERTNPCC_H

#include "importnpcc.hpp"

#include <components/esm/npcstate.hpp>

namespace ESSImport
{

    void convertNPCC (const NPCC& npcc, ESM::NpcState& npcState);

}

#endif
