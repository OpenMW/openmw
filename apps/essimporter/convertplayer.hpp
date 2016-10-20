#ifndef OPENMW_ESSIMPORT_CONVERTPLAYER_H
#define OPENMW_ESSIMPORT_CONVERTPLAYER_H

#include "importplayer.hpp"

#include <components/esm/player.hpp>
#include <components/esm/controlsstate.hpp>

namespace ESSImport
{

    void convertPCDT(const PCDT& pcdt, ESM::Player& out, std::vector<std::string>& outDialogueTopics, bool& firstPersonCam, bool& teleportingEnabled, bool& levitationEnabled, ESM::ControlsState& controls);

}

#endif
