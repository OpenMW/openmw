//
// Created by koncord on 31.03.17.
//

#include "ProcessorInitializer.hpp"

#include "PlayerProcessor.hpp"

#include "processors/player/ProcessorChatMessage.hpp"
#include "processors/player/ProcessorGameConsole.hpp"
#include "processors/player/ProcessorGameTime.hpp"
#include "processors/player/ProcessorGUIMessageBox.hpp"
#include "processors/player/ProcessorHandshake.hpp"
#include "processors/player/ProcessorUserDisconnected.hpp"
#include "processors/player/ProcessorUserMyID.hpp"
#include "processors/player/ProcessorPlayerBaseInfo.hpp"
#include "processors/player/ProcessorPlayerCharGen.hpp"
#include "processors/player/ProcessorPlayerAnimFlags.hpp"
#include "processors/player/ProcessorPlayerAttack.hpp"
#include "processors/player/ProcessorPlayerAttribute.hpp"
#include "processors/player/ProcessorPlayerBounty.hpp"
#include "processors/player/ProcessorPlayerCellChange.hpp"
#include "processors/player/ProcessorPlayerCellState.hpp"
#include "processors/player/ProcessorPlayerCharClass.hpp"
#include "processors/player/ProcessorPlayerDeath.hpp"
#include "processors/player/ProcessorPlayerEquipment.hpp"
#include "processors/player/ProcessorPlayerFaction.hpp"
#include "processors/player/ProcessorPlayerInventory.hpp"
#include "processors/player/ProcessorPlayerJournal.hpp"
#include "processors/player/ProcessorPlayerLevel.hpp"
#include "processors/player/ProcessorPlayerPosition.hpp"
#include "processors/player/ProcessorPlayerResurrect.hpp"
#include "processors/player/ProcessorPlayerSkill.hpp"
#include "processors/player/ProcessorPlayerSpellbook.hpp"
#include "processors/player/ProcessorPlayerStatsDynamic.hpp"

#include "WorldProcessor.hpp"
#include "processors/world/ProcessorContainer.hpp"
#include "processors/world/ProcessorDoorState.hpp"
#include "processors/world/ProcessorMusicPlay.hpp"
#include "processors/world/ProcessorObjectAnimPlay.hpp"
#include "processors/world/ProcessorObjectDelete.hpp"
#include "processors/world/ProcessorObjectLock.hpp"
#include "processors/world/ProcessorObjectMove.hpp"
#include "processors/world/ProcessorObjectPlace.hpp"
#include "processors/world/ProcessorObjectRotate.hpp"
#include "processors/world/ProcessorObjectScale.hpp"
#include "processors/world/ProcessorScriptLocalShort.hpp"
#include "processors/world/ProcessorScriptLocalFloat.hpp"
#include "processors/world/ProcessorScriptMemberShort.hpp"
#include "processors/world/ProcessorScriptGlobalShort.hpp"
#include "processors/world/ProcessorVideoPlay.hpp"

#include "processors/actor/ProcessorActorAnimFlags.hpp"
#include "processors/actor/ProcessorActorAnimPlay.hpp"
#include "processors/actor/ProcessorActorAttack.hpp"
#include "processors/actor/ProcessorActorAuthority.hpp"
#include "processors/actor/ProcessorActorCellChange.hpp"
#include "processors/actor/ProcessorActorEquipment.hpp"
#include "processors/actor/ProcessorActorList.hpp"
#include "processors/actor/ProcessorActorPosition.hpp"
#include "processors/actor/ProcessorActorSpeech.hpp"
#include "processors/actor/ProcessorActorStatsDynamic.hpp"
#include "processors/actor/ProcessorActorTest.hpp"

using namespace mwmp;

void ProcessorInitializer()
{
    PlayerProcessor::AddProcessor(new ProcessorChatMessage());
    PlayerProcessor::AddProcessor(new ProcessorGameConsole());
    PlayerProcessor::AddProcessor(new ProcessorGameTime());
    PlayerProcessor::AddProcessor(new ProcessorGUIMessageBox());
    PlayerProcessor::AddProcessor(new ProcessorHandshake());
    PlayerProcessor::AddProcessor(new ProcessorUserDisconnected());
    PlayerProcessor::AddProcessor(new ProcessorUserMyID());
    PlayerProcessor::AddProcessor(new ProcessorPlayerBaseInfo());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCharGen());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAnimFlags());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAttack());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAttribute());
    PlayerProcessor::AddProcessor(new ProcessorPlayerBounty());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCellChange());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCellState());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCharClass());
    PlayerProcessor::AddProcessor(new ProcessorPlayerDeath());
    PlayerProcessor::AddProcessor(new ProcessorPlayerEquipment());
    PlayerProcessor::AddProcessor(new ProcessorPlayerFaction());
    PlayerProcessor::AddProcessor(new ProcessorPlayerInventory());
    PlayerProcessor::AddProcessor(new ProcessorPlayerJournal());
    PlayerProcessor::AddProcessor(new ProcessorPlayerLevel());
    PlayerProcessor::AddProcessor(new ProcessorPlayerPosition());
    PlayerProcessor::AddProcessor(new ProcessorPlayerResurrect());
    PlayerProcessor::AddProcessor(new ProcessorPlayerSkill());
    PlayerProcessor::AddProcessor(new ProcessorPlayerSpellbook());
    PlayerProcessor::AddProcessor(new ProcessorPlayerStatsDynamic());

    WorldProcessor::AddProcessor(new ProcessorContainer());
    WorldProcessor::AddProcessor(new ProcessorDoorState());
    WorldProcessor::AddProcessor(new ProcessorMusicPlay());
    WorldProcessor::AddProcessor(new ProcessorObjectAnimPlay());
    WorldProcessor::AddProcessor(new ProcessorObjectDelete());
    WorldProcessor::AddProcessor(new ProcessorObjectLock());
    WorldProcessor::AddProcessor(new ProcessorObjectMove());
    WorldProcessor::AddProcessor(new ProcessorObjectPlace());
    WorldProcessor::AddProcessor(new ProcessorObjectRotate());
    WorldProcessor::AddProcessor(new ProcessorObjectScale());
    WorldProcessor::AddProcessor(new ProcessorScriptLocalShort());
    WorldProcessor::AddProcessor(new ProcessorScriptLocalFloat());
    WorldProcessor::AddProcessor(new ProcessorScriptMemberShort());
    WorldProcessor::AddProcessor(new ProcessorScriptGlobalShort());
    WorldProcessor::AddProcessor(new ProcessorVideoPlay());

    ActorProcessor::AddProcessor(new ProcessorActorAnimFlags());
    ActorProcessor::AddProcessor(new ProcessorActorAnimPlay());
    ActorProcessor::AddProcessor(new ProcessorActorAttack());
    ActorProcessor::AddProcessor(new ProcessorActorAuthority());
    ActorProcessor::AddProcessor(new ProcessorActorCellChange());
    ActorProcessor::AddProcessor(new ProcessorActorEquipment());
    ActorProcessor::AddProcessor(new ProcessorActorList());
    ActorProcessor::AddProcessor(new ProcessorActorPosition());
    ActorProcessor::AddProcessor(new ProcessorActorSpeech());
    ActorProcessor::AddProcessor(new ProcessorActorStatsDynamic());
    ActorProcessor::AddProcessor(new ProcessorActorTest());
}
