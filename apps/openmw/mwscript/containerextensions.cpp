#include "containerextensions.hpp"

#include <stdexcept>

#include <MyGUI_LanguageManager.h>

#include <components/debug/debuglog.hpp>

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>

#include <components/misc/strings/format.hpp>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadskil.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/levelledlist.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace
{
    void addToStore(const MWWorld::Ptr& itemPtr, int count, MWWorld::ContainerStore& store, bool resolve = true)
    {
        if (itemPtr.getClass().getScript(itemPtr).empty())
        {
            store.add(itemPtr, count, true, resolve);
        }
        else
        {
            // Adding just one item per time to make sure there isn't a stack of scripted items
            for (int i = 0; i < count; i++)
                store.add(itemPtr, 1, true, resolve);
        }
    }

    void addRandomToStore(const MWWorld::Ptr& itemPtr, int count, MWWorld::ContainerStore& store, bool topLevel = true)
    {
        if (itemPtr.getType() == ESM::ItemLevList::sRecordId)
        {
            const ESM::ItemLevList* levItemList = itemPtr.get<ESM::ItemLevList>()->mBase;

            if (topLevel && count > 1 && levItemList->mFlags & ESM::ItemLevList::Each)
            {
                for (int i = 0; i < count; i++)
                    addRandomToStore(itemPtr, 1, store, true);
            }
            else
            {
                auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                const ESM::RefId& itemId
                    = MWMechanics::getLevelledItem(itemPtr.get<ESM::ItemLevList>()->mBase, false, prng);
                if (itemId.empty())
                    return;
                MWWorld::ManualRef manualRef(*MWBase::Environment::get().getESMStore(), itemId, 1);
                addRandomToStore(manualRef.getPtr(), count, store, false);
            }
        }
        else
            addToStore(itemPtr, count, store);
    }
}

namespace MWScript
{
    namespace Container
    {
        template <class R>
        class OpAddItem : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId item = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Integer count = runtime[0].mInteger;
                runtime.pop();

                if (!MWBase::Environment::get().getESMStore()->find(item))
                {
                    runtime.getContext().report("Failed to add item '" + item.getRefIdString() + "': unknown ID");
                    return;
                }

                if (count < 0)
                    count = static_cast<uint16_t>(count);

                // no-op
                if (count == 0)
                    return;

                if (item == "gold_005" || item == "gold_010" || item == "gold_025" || item == "gold_100")
                    item = MWWorld::ContainerStore::sGoldId;

                // Check if "item" can be placed in a container
                MWWorld::ManualRef manualRef(*MWBase::Environment::get().getESMStore(), item, 1);
                MWWorld::Ptr itemPtr = manualRef.getPtr();
                bool isLevelledList = itemPtr.getClass().getType() == ESM::ItemLevList::sRecordId;
                if (!isLevelledList)
                    MWWorld::ContainerStore::getType(itemPtr);

                // Explicit calls to non-unique actors affect the base record
                if (!R::implicit && ptr.getClass().isActor()
                    && MWBase::Environment::get().getESMStore()->getRefCount(ptr.getCellRef().getRefId()) > 1)
                {
                    ptr.getClass().modifyBaseInventory(ptr.getCellRef().getRefId(), item, count);
                    return;
                }

                // Calls to unresolved containers affect the base record
                if (ptr.getClass().getType() == ESM::Container::sRecordId
                    && (!ptr.getRefData().getCustomData() || !ptr.getClass().getContainerStore(ptr).isResolved()))
                {
                    ptr.getClass().modifyBaseInventory(ptr.getCellRef().getRefId(), item, count);
                    const ESM::Container* baseRecord
                        = MWBase::Environment::get().getESMStore()->get<ESM::Container>().find(
                            ptr.getCellRef().getRefId());
                    const auto& ptrs = MWBase::Environment::get().getWorld()->getAll(ptr.getCellRef().getRefId());
                    for (const auto& container : ptrs)
                    {
                        // use the new base record
                        container.get<ESM::Container>()->mBase = baseRecord;
                        if (container.getRefData().getCustomData())
                        {
                            auto& store = container.getClass().getContainerStore(container);
                            if (isLevelledList)
                            {
                                if (store.isResolved())
                                {
                                    addRandomToStore(itemPtr, count, store);
                                }
                            }
                            else
                                addToStore(itemPtr, count, store, store.isResolved());
                        }
                    }
                    return;
                }
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
                if (isLevelledList)
                    addRandomToStore(itemPtr, count, store);
                else
                    addToStore(itemPtr, count, store);

                // Spawn a messagebox (only for items added to player's inventory and if player is talking to someone)
                if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                {
                    // The two GMST entries below expand to strings informing the player of what, and how many of it has
                    // been added to their inventory
                    std::string msgBox;
                    std::string_view itemName = itemPtr.getClass().getName(itemPtr);
                    if (count == 1)
                    {
                        msgBox = MyGUI::LanguageManager::getInstance().replaceTags("#{sNotifyMessage60}");
                        msgBox = ::Misc::StringUtils::format(msgBox, itemName);
                    }
                    else
                    {
                        msgBox = MyGUI::LanguageManager::getInstance().replaceTags("#{sNotifyMessage61}");
                        msgBox = ::Misc::StringUtils::format(msgBox, count, itemName);
                    }
                    MWBase::Environment::get().getWindowManager()->messageBox(msgBox, MWGui::ShowInDialogueMode_Only);
                }
            }
        };

        template <class R>
        class OpGetItemCount : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime, false);

                ESM::RefId item = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (ptr.isEmpty() || (ptr.getType() != ESM::Container::sRecordId && !ptr.getClass().isActor()))
                {
                    runtime.push(0);
                    return;
                }

                if (item == "gold_005" || item == "gold_010" || item == "gold_025" || item == "gold_100")
                    item = MWWorld::ContainerStore::sGoldId;

                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);

                runtime.push(store.count(item));
            }
        };

        template <class R>
        class OpRemoveItem : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId item = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Integer count = runtime[0].mInteger;
                runtime.pop();

                if (!MWBase::Environment::get().getESMStore()->find(item))
                {
                    runtime.getContext().report("Failed to remove item '" + item.getRefIdString() + "': unknown ID");
                    return;
                }

                if (count < 0)
                    count = static_cast<uint16_t>(count);

                // no-op
                if (count == 0)
                    return;

                if (item == "gold_005" || item == "gold_010" || item == "gold_025" || item == "gold_100")
                    item = MWWorld::ContainerStore::sGoldId;

                // Explicit calls to non-unique actors affect the base record
                if (!R::implicit && ptr.getClass().isActor()
                    && MWBase::Environment::get().getESMStore()->getRefCount(ptr.getCellRef().getRefId()) > 1)
                {
                    ptr.getClass().modifyBaseInventory(ptr.getCellRef().getRefId(), item, -count);
                    return;
                }
                // Calls to unresolved containers affect the base record instead
                else if (ptr.getClass().getType() == ESM::Container::sRecordId
                    && (!ptr.getRefData().getCustomData() || !ptr.getClass().getContainerStore(ptr).isResolved()))
                {
                    ptr.getClass().modifyBaseInventory(ptr.getCellRef().getRefId(), item, -count);
                    const ESM::Container* baseRecord
                        = MWBase::Environment::get().getESMStore()->get<ESM::Container>().find(
                            ptr.getCellRef().getRefId());
                    const auto& ptrs = MWBase::Environment::get().getWorld()->getAll(ptr.getCellRef().getRefId());
                    for (const auto& container : ptrs)
                    {
                        container.get<ESM::Container>()->mBase = baseRecord;
                        if (container.getRefData().getCustomData())
                        {
                            auto& store = container.getClass().getContainerStore(container);
                            // Note that unlike AddItem, RemoveItem only removes from unresolved containers
                            if (!store.isResolved())
                                store.remove(item, count, false, false);
                        }
                    }
                    return;
                }
                MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);

                std::string_view itemName;
                for (MWWorld::ConstContainerStoreIterator iter(store.cbegin()); iter != store.cend(); ++iter)
                {
                    if (iter->getCellRef().getRefId() == item)
                    {
                        itemName = iter->getClass().getName(*iter);
                        break;
                    }
                }

                int numRemoved = store.remove(item, count);

                // Spawn a messagebox (only for items removed from player's inventory)
                if ((numRemoved > 0) && (ptr == MWMechanics::getPlayer()))
                {
                    // The two GMST entries below expand to strings informing the player of what, and how many of it has
                    // been removed from their inventory
                    std::string msgBox;

                    if (numRemoved > 1)
                    {
                        msgBox = MyGUI::LanguageManager::getInstance().replaceTags("#{sNotifyMessage63}");
                        msgBox = ::Misc::StringUtils::format(msgBox, numRemoved, itemName);
                    }
                    else
                    {
                        msgBox = MyGUI::LanguageManager::getInstance().replaceTags("#{sNotifyMessage62}");
                        msgBox = ::Misc::StringUtils::format(msgBox, itemName);
                    }
                    MWBase::Environment::get().getWindowManager()->messageBox(msgBox, MWGui::ShowInDialogueMode_Only);
                }
            }
        };

        template <class R>
        class OpEquip : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId item = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
                auto found = invStore.end();
                const auto& store = *MWBase::Environment::get().getESMStore();

                // With soul gems we prefer filled ones.
                for (auto it = invStore.begin(); it != invStore.end(); ++it)
                {
                    if (it->getCellRef().getRefId() == item)
                    {
                        found = it;
                        const ESM::RefId& soul = it->getCellRef().getSoul();
                        if (!it->getClass().isSoulGem(*it)
                            || (!soul.empty() && store.get<ESM::Creature>().search(soul)))
                            break;
                    }
                }

                if (found == invStore.end())
                {
                    MWWorld::ManualRef ref(store, item, 1);
                    found = ptr.getClass().getContainerStore(ptr).add(ref.getPtr(), 1, false);
                    Log(Debug::Warning) << "Implicitly adding one " << item << " to the inventory store of "
                                        << ptr.getCellRef().getRefId()
                                        << " to fulfill the requirements of Equip instruction";
                }

                if (ptr == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->useItem(*found, true);
                else
                {
                    std::unique_ptr<MWWorld::Action> action = found->getClass().use(*found, true);
                    action->execute(ptr, true);
                }
            }
        };

        template <class R>
        class OpGetArmorType : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer location = runtime[0].mInteger;
                runtime.pop();

                int slot;
                switch (location)
                {
                    case 0:
                        slot = MWWorld::InventoryStore::Slot_Helmet;
                        break;
                    case 1:
                        slot = MWWorld::InventoryStore::Slot_Cuirass;
                        break;
                    case 2:
                        slot = MWWorld::InventoryStore::Slot_LeftPauldron;
                        break;
                    case 3:
                        slot = MWWorld::InventoryStore::Slot_RightPauldron;
                        break;
                    case 4:
                        slot = MWWorld::InventoryStore::Slot_Greaves;
                        break;
                    case 5:
                        slot = MWWorld::InventoryStore::Slot_Boots;
                        break;
                    case 6:
                        slot = MWWorld::InventoryStore::Slot_LeftGauntlet;
                        break;
                    case 7:
                        slot = MWWorld::InventoryStore::Slot_RightGauntlet;
                        break;
                    case 8:
                        slot = MWWorld::InventoryStore::Slot_CarriedLeft; // shield
                        break;
                    case 9:
                        slot = MWWorld::InventoryStore::Slot_LeftGauntlet;
                        break;
                    case 10:
                        slot = MWWorld::InventoryStore::Slot_RightGauntlet;
                        break;
                    default:
                        throw std::runtime_error("armor index out of range");
                }

                const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
                MWWorld::ConstContainerStoreIterator it = invStore.getSlot(slot);

                if (it == invStore.end() || it->getType() != ESM::Armor::sRecordId)
                {
                    runtime.push(-1);
                    return;
                }

                ESM::RefId skill = it->getClass().getEquipmentSkill(*it);
                if (skill == ESM::Skill::HeavyArmor)
                    runtime.push(2);
                else if (skill == ESM::Skill::MediumArmor)
                    runtime.push(1);
                else if (skill == ESM::Skill::LightArmor)
                    runtime.push(0);
                else
                    runtime.push(-1);
            }
        };

        template <class R>
        class OpHasItemEquipped : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId item = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
                for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                {
                    MWWorld::ConstContainerStoreIterator it = invStore.getSlot(slot);
                    if (it != invStore.end() && it->getCellRef().getRefId() == item)
                    {
                        runtime.push(1);
                        return;
                    }
                }
                runtime.push(0);
            }
        };

        template <class R>
        class OpHasSoulGem : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId name = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                int count = 0;
                const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
                for (MWWorld::ConstContainerStoreIterator it
                     = invStore.cbegin(MWWorld::ContainerStore::Type_Miscellaneous);
                     it != invStore.cend(); ++it)
                {
                    if (it->getCellRef().getSoul() == name)
                        count += it->getCellRef().getCount();
                }
                runtime.push(count);
            }
        };

        template <class R>
        class OpGetWeaponType : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
                MWWorld::ConstContainerStoreIterator it = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                if (it == invStore.end())
                {
                    runtime.push(-1);
                    return;
                }
                else if (it->getType() != ESM::Weapon::sRecordId)
                {
                    if (it->getType() == ESM::Lockpick::sRecordId)
                    {
                        runtime.push(-2);
                    }
                    else if (it->getType() == ESM::Probe::sRecordId)
                    {
                        runtime.push(-3);
                    }
                    else
                    {
                        runtime.push(-1);
                    }
                    return;
                }

                runtime.push(it->get<ESM::Weapon>()->mBase->mData.mType);
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5<OpAddItem<ImplicitRef>>(Compiler::Container::opcodeAddItem);
            interpreter.installSegment5<OpAddItem<ExplicitRef>>(Compiler::Container::opcodeAddItemExplicit);
            interpreter.installSegment5<OpGetItemCount<ImplicitRef>>(Compiler::Container::opcodeGetItemCount);
            interpreter.installSegment5<OpGetItemCount<ExplicitRef>>(Compiler::Container::opcodeGetItemCountExplicit);
            interpreter.installSegment5<OpRemoveItem<ImplicitRef>>(Compiler::Container::opcodeRemoveItem);
            interpreter.installSegment5<OpRemoveItem<ExplicitRef>>(Compiler::Container::opcodeRemoveItemExplicit);
            interpreter.installSegment5<OpEquip<ImplicitRef>>(Compiler::Container::opcodeEquip);
            interpreter.installSegment5<OpEquip<ExplicitRef>>(Compiler::Container::opcodeEquipExplicit);
            interpreter.installSegment5<OpGetArmorType<ImplicitRef>>(Compiler::Container::opcodeGetArmorType);
            interpreter.installSegment5<OpGetArmorType<ExplicitRef>>(Compiler::Container::opcodeGetArmorTypeExplicit);
            interpreter.installSegment5<OpHasItemEquipped<ImplicitRef>>(Compiler::Container::opcodeHasItemEquipped);
            interpreter.installSegment5<OpHasItemEquipped<ExplicitRef>>(
                Compiler::Container::opcodeHasItemEquippedExplicit);
            interpreter.installSegment5<OpHasSoulGem<ImplicitRef>>(Compiler::Container::opcodeHasSoulGem);
            interpreter.installSegment5<OpHasSoulGem<ExplicitRef>>(Compiler::Container::opcodeHasSoulGemExplicit);
            interpreter.installSegment5<OpGetWeaponType<ImplicitRef>>(Compiler::Container::opcodeGetWeaponType);
            interpreter.installSegment5<OpGetWeaponType<ExplicitRef>>(Compiler::Container::opcodeGetWeaponTypeExplicit);
        }
    }
}
