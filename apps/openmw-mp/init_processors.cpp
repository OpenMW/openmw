//
// Created by koncord on 31.03.17.
//

#include "init_processors.hpp"

#include "PlayerProcessor.hpp"
#include "processors/ProcessorPlayerPos.hpp"
#include "processors/ProcessorPlayerCellChange.hpp"
#include "processors/ProcessorPlayerCellState.hpp"
#include "processors/ProcessorPlayerAttribute.hpp"
#include "processors/ProcessorPlayerSkill.hpp"
#include "processors/ProcessorLevel.hpp"
#include "processors/ProcessorPlayerEquipment.hpp"
#include "processors/ProcessorPlayerInventory.hpp"
#include "processors/ProcessorPlayerSpellbook.hpp"
#include "processors/ProcessorPlayerJournal.hpp"
#include "processors/ProcessorPlayerAttack.hpp"
#include "processors/ProcessorPlayerDynamicStats.hpp"
#include "processors/ProcessorPlayerDeath.hpp"
#include "processors/ProcessorPlayerResurrect.hpp"
#include "processors/ProcessorPlayerDrawState.hpp"
#include "processors/ProcessorChatMsg.hpp"
#include "processors/ProcessorPlayerCharGen.hpp"
#include "processors/ProcessorGUIMessageBox.hpp"
#include "processors/ProcessorPlayerCharClass.hpp"


using namespace mwmp;

void init_processors()
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