#include "containerextensions.hpp"

#include <stdexcept>

#include <MyGUI_LanguageManager.h>

#include <components/debug/debuglog.hpp>

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>

#include <components/misc/stringops.hpp>

#include <components/esm/loadskil.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "ref.hpp"

namespace MWScript
{
    namespace Container
    {
        template<class R>
        class OpAddItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for AddItem must be non-negative");

                    // no-op
                    if (count == 0)
                        return;

                    if(::Misc::StringUtils::ciEqual(item, "gold_005")
                            || ::Misc::StringUtils::ciEqual(item, "gold_010")
                            || ::Misc::StringUtils::ciEqual(item, "gold_025")
                            || ::Misc::StringUtils::ciEqual(item, "gold_100"))
                        item = "gold_001";

                    MWWorld::ContainerStore& store = ptr.getClass().getContainerStore (ptr);
                    // Create a Ptr for the first added item to recover the item name later
                    MWWorld::Ptr itemPtr = *store.add (item, 1, ptr);
                    if (itemPtr.getClass().getScript(itemPtr).empty())
                    {
                        store.add (item, count-1, ptr);
                    }
                    else
                    {
                        // Adding just one item per time to make sure there isn't a stack of scripted items
                        for (int i = 1; i < count; i++)
                            store.add (item, 1, ptr);
                    }

                    // Spawn a messagebox (only for items added to player's inventory and if player is talking to someone)
                    if (ptr == MWBase::Environment::get().getWorld ()->getPlayerPtr() )
                    {
                        // The two GMST entries below expand to strings informing the player of what, and how many of it has been added to their inventory
                        std::string msgBox;
                        std::string itemName = itemPtr.getClass().getName(itemPtr);
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

        template<class R>
        class OpGetItemCount : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    if(::Misc::StringUtils::ciEqual(item, "gold_005")
                            || ::Misc::StringUtils::ciEqual(item, "gold_010")
                            || ::Misc::StringUtils::ciEqual(item, "gold_025")
                            || ::Misc::StringUtils::ciEqual(item, "gold_100"))
                        item = "gold_001";

                    MWWorld::ContainerStore& store = ptr.getClass().getContainerStore (ptr);

                    runtime.push (store.count(item));
                }
        };

        template<class R>
        class OpRemoveItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for RemoveItem must be non-negative");

                    // no-op
                    if (count == 0)
                        return;

                    if(::Misc::StringUtils::ciEqual(item, "gold_005")
                            || ::Misc::StringUtils::ciEqual(item, "gold_010")
                            || ::Misc::StringUtils::ciEqual(item, "gold_025")
                            || ::Misc::StringUtils::ciEqual(item, "gold_100"))
                        item = "gold_001";

                    MWWorld::ContainerStore& store = ptr.getClass().getContainerStore (ptr);

                    std::string itemName;
                    for (MWWorld::ConstContainerStoreIterator iter(store.cbegin()); iter != store.cend(); ++iter)
                    {
                        if (::Misc::StringUtils::ciEqual(iter->getCellRef().getRefId(), item))
                        {
                            itemName = iter->getClass().getName(*iter);
                            break;
                        }
                    }

                    int numRemoved = store.remove(item, count, ptr);

                    // Spawn a messagebox (only for items removed from player's inventory)
                    if ((numRemoved > 0)
                        && (ptr == MWMechanics::getPlayer()))
                    {
                        // The two GMST entries below expand to strings informing the player of what, and how many of it has been removed from their inventory
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

                virtual void execute(Interpreter::Runtime &runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore (ptr);
                    MWWorld::ContainerStoreIterator it = invStore.begin();
                    for (; it != invStore.end(); ++it)
                    {
                        if (::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), item))
                            break;
                    }
                    if (it == invStore.end())
                    {
                        it = ptr.getClass().getContainerStore (ptr).add (item, 1, ptr);
                        Log(Debug::Warning) << "Implicitly adding one " << item << 
                            " to the inventory store of " << ptr.getCellRef().getRefId() <<
                            " to fulfill the requirements of Equip instruction";
                    }

                    if (ptr == MWMechanics::getPlayer())
                        MWBase::Environment::get().getWindowManager()->useItem(*it, true);
                    else
                    {
                        std::shared_ptr<MWWorld::Action> action = it->getClass().use(*it, true);
                        action->execute(ptr, true);
                    }
                }
        };

        template <class R>
        class OpGetArmorType : public Interpreter::Opcode0
        {
            public:

                virtual void execute(Interpreter::Runtime &runtime)
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
                            throw std::runtime_error ("armor index out of range");
                    }

                    const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore (ptr);
                    MWWorld::ConstContainerStoreIterator it = invStore.getSlot (slot);
                    
                    if (it == invStore.end() || it->getTypeName () != typeid(ESM::Armor).name())
                    {
                        runtime.push(-1);
                        return;
                    }

                    int skill = it->getClass().getEquipmentSkill (*it) ;
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

                virtual void execute(Interpreter::Runtime &runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore (ptr);
                    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                    {
                        MWWorld::ConstContainerStoreIterator it = invStore.getSlot (slot);
                        if (it != invStore.end() && ::Misc::StringUtils::ciEqual(it->getCellRef().getRefId(), item))
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

                virtual void execute(Interpreter::Runtime &runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    const std::string &name = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    int count = 0;
                    const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore (ptr);
                    for (MWWorld::ConstContainerStoreIterator it = invStore.cbegin(MWWorld::ContainerStore::Type_Miscellaneous);
                         it != invStore.cend(); ++it)
                    {
                        if (::Misc::StringUtils::ciEqual(it->getCellRef().getSoul(), name))
                            count += it->getRefData().getCount();
                    }
                    runtime.push(count);
                }
        };

        template <class R>
        class OpGetWeaponType : public Interpreter::Opcode0
        {
            public:

                virtual void execute(Interpreter::Runtime &runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    const MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore (ptr);
                    MWWorld::ConstContainerStoreIterator it = invStore.getSlot (MWWorld::InventoryStore::Slot_CarriedRight);
                    if (it == invStore.end())
                    {
                        runtime.push(-1);
                        return;
                    }
                    else if (it->getTypeName() != typeid(ESM::Weapon).name())
                    {
                        if (it->getTypeName() == typeid(ESM::Lockpick).name())
                        {
                            runtime.push(-2);
                        }
                        else if (it->getTypeName() == typeid(ESM::Probe).name())
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


        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
             interpreter.installSegment5 (Compiler::Container::opcodeAddItem, new OpAddItem<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeAddItemExplicit, new OpAddItem<ExplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeGetItemCount, new OpGetItemCount<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeGetItemCountExplicit, new OpGetItemCount<ExplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeRemoveItem, new OpRemoveItem<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeRemoveItemExplicit, new OpRemoveItem<ExplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeEquip, new OpEquip<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeEquipExplicit, new OpEquip<ExplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeGetArmorType, new OpGetArmorType<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeGetArmorTypeExplicit, new OpGetArmorType<ExplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeHasItemEquipped, new OpHasItemEquipped<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeHasItemEquippedExplicit, new OpHasItemEquipped<ExplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeHasSoulGem, new OpHasSoulGem<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeHasSoulGemExplicit, new OpHasSoulGem<ExplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeGetWeaponType, new OpGetWeaponType<ImplicitRef>);
             interpreter.installSegment5 (Compiler::Container::opcodeGetWeaponTypeExplicit, new OpGetWeaponType<ExplicitRef>);
        }
    }
}
