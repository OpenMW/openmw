//
// Created by koncord on 31.03.17.
//

#include "ProcessorInitializer.hpp"

#include "PlayerProcessor.hpp"

#include "player/ProcessorChatMessage.hpp"
#include "player/ProcessorGUIMessageBox.hpp"
#include "player/ProcessorHandshake.hpp"
#include "player/ProcessorUserDisconnected.hpp"
#include "player/ProcessorGameSettings.hpp"
#include "player/ProcessorGameTime.hpp"
#include "player/ProcessorGameWeather.hpp"
#include "player/ProcessorPlayerBaseInfo.hpp"
#include "player/ProcessorPlayerCharGen.hpp"
#include "player/ProcessorPlayerAnimFlags.hpp"
#include "player/ProcessorPlayerAnimPlay.hpp"
#include "player/ProcessorPlayerAttack.hpp"
#include "player/ProcessorPlayerAttribute.hpp"
#include "player/ProcessorPlayerBook.hpp"
#include "player/ProcessorPlayerBounty.hpp"
#include "player/ProcessorPlayerCellChange.hpp"
#include "player/ProcessorPlayerCellState.hpp"
#include "player/ProcessorPlayerCharClass.hpp"
#include "player/ProcessorPlayerDeath.hpp"
#include "player/ProcessorPlayerDisposition.hpp"
#include "player/ProcessorPlayerEquipment.hpp"
#include "player/ProcessorPlayerFaction.hpp"
#include "player/ProcessorPlayerInventory.hpp"
#include "player/ProcessorPlayerJail.hpp"
#include "player/ProcessorPlayerJournal.hpp"
#include "player/ProcessorPlayerKillCount.hpp"
#include "player/ProcessorPlayerLevel.hpp"
#include "player/ProcessorPlayerMap.hpp"
#include "player/ProcessorPlayerPosition.hpp"
#include "player/ProcessorPlayerRegionAuthority.hpp"
#include "player/ProcessorPlayerRest.hpp"
#include "player/ProcessorPlayerResurrect.hpp"
#include "player/ProcessorPlayerShapeshift.hpp"
#include "player/ProcessorPlayerSkill.hpp"
#include "player/ProcessorPlayerSpeech.hpp"
#include "player/ProcessorPlayerSpellbook.hpp"
#include "player/ProcessorPlayerStatsDynamic.hpp"
#include "player/ProcessorPlayerTopic.hpp"

#include "WorldProcessor.hpp"
#include "world/ProcessorConsoleCommand.hpp"
#include "world/ProcessorContainer.hpp"
#include "world/ProcessorDoorState.hpp"
#include "world/ProcessorMusicPlay.hpp"
#include "world/ProcessorObjectAnimPlay.hpp"
#include "world/ProcessorObjectDelete.hpp"
#include "world/ProcessorObjectLock.hpp"
#include "world/ProcessorObjectMove.hpp"
#include "world/ProcessorObjectPlace.hpp"
#include "world/ProcessorObjectRotate.hpp"
#include "world/ProcessorObjectScale.hpp"
#include "world/ProcessorObjectSpawn.hpp"
#include "world/ProcessorObjectState.hpp"
#include "world/ProcessorObjectTrap.hpp"
#include "world/ProcessorScriptLocalShort.hpp"
#include "world/ProcessorScriptLocalFloat.hpp"
#include "world/ProcessorScriptMemberShort.hpp"
#include "world/ProcessorScriptGlobalShort.hpp"
#include "world/ProcessorVideoPlay.hpp"

#include "actor/ProcessorActorAI.hpp"
#include "actor/ProcessorActorAnimFlags.hpp"
#include "actor/ProcessorActorAnimPlay.hpp"
#include "actor/ProcessorActorAttack.hpp"
#include "actor/ProcessorActorAuthority.hpp"
#include "actor/ProcessorActorCellChange.hpp"
#include "actor/ProcessorActorDeath.hpp"
#include "actor/ProcessorActorEquipment.hpp"
#include "actor/ProcessorActorList.hpp"
#include "actor/ProcessorActorPosition.hpp"
#include "actor/ProcessorActorSpeech.hpp"
#include "actor/ProcessorActorStatsDynamic.hpp"
#include "actor/ProcessorActorTest.hpp"

using namespace mwmp;

void ProcessorInitializer()
{
    PlayerProcessor::AddProcessor(new ProcessorChatMessage());
    PlayerProcessor::AddProcessor(new ProcessorGUIMessageBox());
    PlayerProcessor::AddProcessor(new ProcessorHandshake());
    PlayerProcessor::AddProcessor(new ProcessorUserDisconnected());
    PlayerProcessor::AddProcessor(new ProcessorGameSettings());
    PlayerProcessor::AddProcessor(new ProcessorGameTime());
    PlayerProcessor::AddProcessor(new ProcessorGameWeather());
    PlayerProcessor::AddProcessor(new ProcessorPlayerBaseInfo());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCharGen());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAnimFlags());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAnimPlay());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAttack());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAttribute());
    PlayerProcessor::AddProcessor(new ProcessorPlayerBook());
    PlayerProcessor::AddProcessor(new ProcessorPlayerBounty());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCellChange());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCellState());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCharClass());
    PlayerProcessor::AddProcessor(new ProcessorPlayerDeath());
    PlayerProcessor::AddProcessor(new ProcessorPlayerDisposition());
    PlayerProcessor::AddProcessor(new ProcessorPlayerEquipment());
    PlayerProcessor::AddProcessor(new ProcessorPlayerFaction());
    PlayerProcessor::AddProcessor(new ProcessorPlayerInventory());
    PlayerProcessor::AddProcessor(new ProcessorPlayerJail());
    PlayerProcessor::AddProcessor(new ProcessorPlayerJournal());
    PlayerProcessor::AddProcessor(new ProcessorPlayerKillCount());
    PlayerProcessor::AddProcessor(new ProcessorPlayerLevel());
    PlayerProcessor::AddProcessor(new ProcessorPlayerMap());
    PlayerProcessor::AddProcessor(new ProcessorPlayerPosition());
    PlayerProcessor::AddProcessor(new ProcessorPlayerRegionAuthority());
    PlayerProcessor::AddProcessor(new ProcessorPlayerRest());
    PlayerProcessor::AddProcessor(new ProcessorPlayerResurrect());
    PlayerProcessor::AddProcessor(new ProcessorPlayerShapeshift());
    PlayerProcessor::AddProcessor(new ProcessorPlayerSkill());
    PlayerProcessor::AddProcessor(new ProcessorPlayerSpeech());
    PlayerProcessor::AddProcessor(new ProcessorPlayerSpellbook());
    PlayerProcessor::AddProcessor(new ProcessorPlayerStatsDynamic());
    PlayerProcessor::AddProcessor(new ProcessorPlayerTopic());

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
    WorldProcessor::AddProcessor(new ProcessorObjectSpawn());
    WorldProcessor::AddProcessor(new ProcessorObjectState());
    WorldProcessor::AddProcessor(new ProcessorObjectTrap());
    WorldProcessor::AddProcessor(new ProcessorScriptLocalShort());
    WorldProcessor::AddProcessor(new ProcessorScriptLocalFloat());
    WorldProcessor::AddProcessor(new ProcessorScriptMemberShort());
    WorldProcessor::AddProcessor(new ProcessorScriptGlobalShort());
    WorldProcessor::AddProcessor(new ProcessorVideoPlay());

    ActorProcessor::AddProcessor(new ProcessorActorAI());
    ActorProcessor::AddProcessor(new ProcessorActorAnimFlags());
    ActorProcessor::AddProcessor(new ProcessorActorAnimPlay());
    ActorProcessor::AddProcessor(new ProcessorActorAttack());
    ActorProcessor::AddProcessor(new ProcessorActorAuthority());
    ActorProcessor::AddProcessor(new ProcessorActorCellChange());
    ActorProcessor::AddProcessor(new ProcessorActorDeath());
    ActorProcessor::AddProcessor(new ProcessorActorEquipment());
    ActorProcessor::AddProcessor(new ProcessorActorList());
    ActorProcessor::AddProcessor(new ProcessorActorPosition());
    ActorProcessor::AddProcessor(new ProcessorActorSpeech());
    ActorProcessor::AddProcessor(new ProcessorActorStatsDynamic());
    ActorProcessor::AddProcessor(new ProcessorActorTest());
}
