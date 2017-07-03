//
// Created by koncord on 14.01.16.
//

#ifndef OPENMW_LOCALPLAYER_HPP
#define OPENMW_LOCALPLAYER_HPP

#include <components/openmw-mp/Base/BasePlayer.hpp>
#include "../mwworld/ptr.hpp"
#include <RakNetTypes.h>

namespace mwmp
{
    class Networking;
    class LocalPlayer : public BasePlayer
    {
    public:

        LocalPlayer();
        virtual ~LocalPlayer();

        void update();

        void charGen(int stageFirst, int stageEnd);
        bool charGenThread(); // return true if CGStage::current == CGStage::end
        bool hasFinishedCharGen();

        void updateStatsDynamic(bool forceUpdate = false);
        void updateAttributes(bool forceUpdate = false);
        void updateSkills(bool forceUpdate = false);
        void updateLevel(bool forceUpdate = false);
        void updateBounty(bool forceUpdate = false);
        void updatePosition(bool forceUpdate = false);
        void updateCell(bool forceUpdate = false);
        void updateChar();
        void updateEquipment(bool forceUpdate = false);
        void updateInventory(bool forceUpdate = false);
        void updateAttack();
        void updateDeadState(bool forceUpdate = false);
        void updateAnimFlags(bool forceUpdate = false);

        void addItems();
        void addSpells();
        void addJournalItems();
        void addTopics();

        void removeItems();
        void removeSpells();

        void setDynamicStats();
        void setAttributes();
        void setSkills();
        void setLevel();
        void setBounty();
        void setPosition();
        void setCell();
        void setClass();
        void setEquipment();
        void setInventory();
        void setSpellbook();
        void setFactions();
        void setKills();
        void setBooks();
        void setMapExplored();

        void sendClass();
        void sendInventory();
        void sendSpellbook();
        void sendCellStates();
        void sendSpellAddition(std::string id);
        void sendSpellAddition(const ESM::Spell &spell);
        void sendSpellRemoval(std::string id);
        void sendSpellRemoval(const ESM::Spell &spell);
        void sendJournalEntry(const std::string& quest, int index, const MWWorld::Ptr& actor);
        void sendJournalIndex(const std::string& quest, int index);
        void sendFaction(const std::string& factionId, int rank, bool isExpelled);
        void sendTopic(const std::string& topic);
        void sendKill(const std::string& refId, int number);
        void sendBook(const std::string& bookId);

        void clearCellStates();
        void clearCurrentContainer();

        void storeCellState(ESM::Cell cell, int stateType);
        void storeCurrentContainer(const MWWorld::Ptr& container, bool loot);

    private:
        Networking *getNetworking();
        MWWorld::Ptr getPlayerPtr();

    };
}

#endif //OPENMW_LOCALPLAYER_HPP
