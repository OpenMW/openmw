#ifndef OPENMW_ESSIMPORT_CONVERTPLAYER_H
#define OPENMW_ESSIMPORT_CONVERTPLAYER_H

#include "importplayer.hpp"

#include <components/esm/player.hpp>

namespace ESSImport
{

    void convertPCDT(const PCDT& pcdt, ESM::Player& out, std::vector<std::string>& outDialogueTopics, bool& firstPersonCam);

}

#endif
