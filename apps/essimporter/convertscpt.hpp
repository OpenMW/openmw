#ifndef OPENMW_ESSIMPORT_CONVERTSCPT_H
#define OPENMW_ESSIMPORT_CONVERTSCPT_H

#include <components/esm/globalscript.hpp>

#include "importscpt.hpp"

namespace ESSImport
{

void convertSCPT(const SCPT& scpt, ESM::GlobalScript& out);

}

#endif
