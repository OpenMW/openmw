#include "actionharvest.hpp"

#include <sstream>

#include <MyGUI_LanguageManager.h>

#include <components/misc/stringops.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "class.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionHarvest::ActionHarvest (const MWWorld::Ptr& container)
        : Action (true, container)
    {
        setSound("Item Ingredient Up");
    }

    void ActionHarvest::executeImp (const MWWorld::Ptr& actor)
    {
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return;

        MWBase::MechanicsManager* mechMgr = MWBase::Environment::get().getMechanicsManager();
        MWWorld::Ptr target = getTarget();
        MWWorld::ContainerStore& store = target.getClass().getContainerStore (target);
        MWWorld::ContainerStore& actorStore = actor.getClass().getContainerStore(actor);
        std::map<std::string, int> takenMap;
        int value = 0;
        MWWorld::Ptr victim;
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        bool isPlayer = (actor == player);
        bool triggerCrime = isPlayer && !mechMgr->isAllowedToUse(actor, target, victim);
        for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
        {
            if (!it->getClass().showsInInventory(*it))
                continue;

            int itemCount = it->getRefData().getCount();
            value += it->getClass().getValue(*it) * itemCount;
            // It is important to fire this event before transferring the item because harvested containers are considered "allowed to use".
            mechMgr->itemTaken(actor, *it, target, itemCount, false);
            actorStore.add(*it, itemCount, actor);
            store.remove(*it, itemCount, getTarget());
            takenMap[it->getClass().getName(*it)]+=itemCount;
        }

        // Spawn a messagebox and trigger potential crime event (only for items added to player's inventory)
        if (isPlayer)
        {
            if (!takenMap.empty() && triggerCrime)
                mechMgr->commitCrime(actor, victim, MWBase::MechanicsManager::OT_Theft, value);

            std::ostringstream stream;
            int lineCount = 0;
            const static int maxLines = 10;
            for (auto & pair : takenMap)
            {
                std::string itemName = pair.first;
                int itemCount = pair.second;
                lineCount++;
                if (lineCount == maxLines)
                    stream << "\n...";
                else if (lineCount > maxLines)
                    break;

                // The two GMST entries below expand to strings informing the player of what, and how many of it has been added to their inventory
                std::string msgBox;
                if (itemCount == 1)
                {
                    msgBox = MyGUI::LanguageManager::getInstance().replaceTags("\n#{sNotifyMessage60}");
                    Misc::StringUtils::replace(msgBox, "%s", itemName.c_str(), 2);
                }
                else
                {
                    msgBox = MyGUI::LanguageManager::getInstance().replaceTags("\n#{sNotifyMessage61}");
                    Misc::StringUtils::replace(msgBox, "%d", std::to_string(itemCount).c_str(), 2);
                    Misc::StringUtils::replace(msgBox, "%s", itemName.c_str(), 2);
                }

                stream << msgBox;
            }
            std::string tooltip = stream.str();
            // remove the first newline (easier this way)
            if (tooltip.size() > 0 && tooltip[0] == '\n')
                tooltip.erase(0, 1);

            if (tooltip.size() > 0)
                MWBase::Environment::get().getWindowManager()->messageBox(tooltip);
        }

        // Update animation object
        MWBase::Environment::get().getWorld()->disable(target);
        MWBase::Environment::get().getWorld()->enable(target);
    }
}
