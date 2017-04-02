//
// Created by koncord on 31.03.17.
//

#include "ProcessorInitializer.hpp"

#include "PlayerProcessor.hpp"
#include "processors/player/ProcessorPlayerPos.hpp"
#include "processors/player/ProcessorPlayerCellChange.hpp"
#include "processors/player/ProcessorPlayerCellState.hpp"
#include "processors/player/ProcessorPlayerAttribute.hpp"
#include "processors/player/ProcessorPlayerSkill.hpp"
#include "processors/player/ProcessorLevel.hpp"
#include "processors/player/ProcessorPlayerEquipment.hpp"
#include "processors/player/ProcessorPlayerInventory.hpp"
#include "processors/player/ProcessorPlayerSpellbook.hpp"
#include "processors/player/ProcessorPlayerJournal.hpp"
#include "processors/player/ProcessorPlayerAttack.hpp"
#include "processors/player/ProcessorPlayerDynamicStats.hpp"
#include "processors/player/ProcessorPlayerDeath.hpp"
#include "processors/player/ProcessorPlayerResurrect.hpp"
#include "processors/player/ProcessorPlayerDrawState.hpp"
#include "processors/player/ProcessorChatMsg.hpp"
#include "processors/player/ProcessorPlayerCharGen.hpp"
#include "processors/player/ProcessorGUIMessageBox.hpp"
#include "processors/player/ProcessorPlayerCharClass.hpp"


using namespace mwmp;

void ProcessorInitializer()
{
    PlayerProcessor::AddProcessor(new ProcessorPlayerPos());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCellChange());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCellState());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAttribute());
    PlayerProcessor::AddProcessor(new ProcessorPlayerSkill());
    PlayerProcessor::AddProcessor(new ProcessorLevel());
    PlayerProcessor::AddProcessor(new ProcessorPlayerEquipment());
    PlayerProcessor::AddProcessor(new ProcessorPlayerInventory());
    PlayerProcessor::AddProcessor(new ProcessorPlayerSpellbook());
    PlayerProcessor::AddProcessor(new ProcessorPlayerJournal());
    PlayerProcessor::AddProcessor(new ProcessorPlayerAttack());
    PlayerProcessor::AddProcessor(new ProcessorPlayerDynamicStats());
    PlayerProcessor::AddProcessor(new ProcessorPlayerDeath());
    PlayerProcessor::AddProcessor(new ProcessorPlayerResurrect());
    PlayerProcessor::AddProcessor(new ProcessorPlayerDrawState());
    PlayerProcessor::AddProcessor(new ProcessorChatMsg());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCharGen());
    PlayerProcessor::AddProcessor(new ProcessorGUIMessageBox());
    PlayerProcessor::AddProcessor(new ProcessorPlayerCharClass());
}