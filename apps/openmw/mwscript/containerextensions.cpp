
#include "containerextensions.hpp"

#include <stdexcept>

#include <boost/format.hpp>

#include <MyGUI_LanguageManager.h>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"

#include "interpretercontext.hpp"
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

                    MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), item);

                    ref.getPtr().getRefData().setCount (count);

                    MWWorld::Class::get (ptr).getContainerStore (ptr).add (ref.getPtr());
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

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);

                    Interpreter::Type_Integer sum = 0;

                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                        if (Misc::StringUtils::ciEqual(iter->getCellRef().mRefID, item))
                            sum += iter->getRefData().getCount();

                    runtime.push (sum);
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

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);
                    
                    std::string itemName = "";
                    Interpreter::Type_Integer originalCount = count;

                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end() && count;
                        ++iter)
                    {
                        if (Misc::StringUtils::ciEqual(iter->getCellRef().mRefID, item))
                        {
                            itemName = MWWorld::Class::get(*iter).getName(*iter);
                            
                            if (iter->getRefData().getCount()<=count)
                            {
                                count -= iter->getRefData().getCount();
                                iter->getRefData().setCount (0);
                            }
                            else
                            {
                                iter->getRefData().setCount (iter->getRefData().getCount()-count);
                                count = 0;
                            }
                        }
                    }
                  
                    /* The two GMST entries below expand to strings informing the player of what, and how many of it has been removed from their inventory */
                    std::string msgBox;
                    if(originalCount - count > 1)
                    {
                        msgBox = MyGUI::LanguageManager::getInstance().replaceTags("#{sNotifyMessage63}");
                        std::stringstream temp;
                        temp << boost::format(msgBox) % (originalCount - count) % itemName;
                        msgBox = temp.str();
                    }
                    else
                    {
                        msgBox = MyGUI::LanguageManager::getInstance().replaceTags("#{sNotifyMessage62}");
                        std::stringstream temp;
                        temp << boost::format(msgBox) % itemName;
                        msgBox = temp.str();
                    }
                    
                    if(originalCount - count > 0)
                        MWBase::Environment::get().getWindowManager()->messageBox(msgBox, std::vector<std::string>());

                    // To be fully compatible with original Morrowind, we would need to check if
                    // count is >= 0 here and throw an exception. But let's be tollerant instead.
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

                    MWWorld::InventoryStore& invStore = MWWorld::Class::get(ptr).getInventoryStore (ptr);
                    MWWorld::ContainerStoreIterator it = invStore.begin();
                    for (; it != invStore.end(); ++it)
                    {
                        if (Misc::StringUtils::ciEqual(it->getCellRef().mRefID, item))
                            break;
                    }
                    if (it == invStore.end())
                        throw std::runtime_error("Item to equip not found");

                    MWWorld::ActionEquip action (*it);
                    action.execute(ptr);
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

                    MWWorld::InventoryStore& invStore = MWWorld::Class::get(ptr).getInventoryStore (ptr);

                    MWWorld::ContainerStoreIterator it = invStore.getSlot (slot);
                    if (it == invStore.end() || it->getTypeName () != typeid(ESM::Armor).name())
                    {
                        runtime.push(-1);
                        return;
                    }

                    int skill = MWWorld::Class::get(*it).getEquipmentSkill (*it) ;
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

                    MWWorld::InventoryStore& invStore = MWWorld::Class::get(ptr).getInventoryStore (ptr);
                    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                    {
                        MWWorld::ContainerStoreIterator it = invStore.getSlot (slot);
                        if (it != invStore.end() && Misc::StringUtils::ciEqual(it->getCellRef().mRefID, item))
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

                    MWWorld::InventoryStore& invStore = MWWorld::Class::get(ptr).getInventoryStore (ptr);
                    for (MWWorld::ContainerStoreIterator it = invStore.begin(MWWorld::ContainerStore::Type_Miscellaneous);
                         it != invStore.end(); ++it)
                    {

                        if (Misc::StringUtils::ciEqual(it->getCellRef().mSoul, name))
                        {
                            runtime.push(1);
                            return;
                        }
                    }
                    runtime.push(0);
                }
        };

        template <class R>
        class OpGetWeaponType : public Interpreter::Opcode0
        {
            public:

                virtual void execute(Interpreter::Runtime &runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::InventoryStore& invStore = MWWorld::Class::get(ptr).getInventoryStore (ptr);
                    MWWorld::ContainerStoreIterator it = invStore.getSlot (MWWorld::InventoryStore::Slot_CarriedRight);
                    if (it == invStore.end() || it->getTypeName () != typeid(ESM::Weapon).name())
                    {
                        runtime.push(-1);
                        return;
                    }

                    runtime.push(it->get<ESM::Weapon>()->mBase->mData.mType);
                }
        };

        const int opcodeAddItem = 0x2000076;
        const int opcodeAddItemExplicit = 0x2000077;
        const int opcodeGetItemCount = 0x2000078;
        const int opcodeGetItemCountExplicit = 0x2000079;
        const int opcodeRemoveItem = 0x200007a;
        const int opcodeRemoveItemExplicit = 0x200007b;
        const int opcodeEquip = 0x20001b3;
        const int opcodeEquipExplicit = 0x20001b4;
        const int opcodeGetArmorType = 0x20001d1;
        const int opcodeGetArmorTypeExplicit = 0x20001d2;
        const int opcodeHasItemEquipped = 0x20001d5;
        const int opcodeHasItemEquippedExplicit = 0x20001d6;
        const int opcodeHasSoulGem = 0x20001de;
        const int opcodeHasSoulGemExplicit = 0x20001df;
        const int opcodeGetWeaponType = 0x20001e0;
        const int opcodeGetWeaponTypeExplicit = 0x20001e1;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("additem", "cl", opcodeAddItem, opcodeAddItemExplicit);
            extensions.registerFunction ("getitemcount", 'l', "c", opcodeGetItemCount,
                opcodeGetItemCountExplicit);
            extensions.registerInstruction ("removeitem", "cl", opcodeRemoveItem,
                opcodeRemoveItemExplicit);
            extensions.registerInstruction ("equip", "c", opcodeEquip, opcodeEquipExplicit);
            extensions.registerFunction ("getarmortype", 'l', "l", opcodeGetArmorType, opcodeGetArmorTypeExplicit);
            extensions.registerFunction ("hasitemequipped", 'l', "c", opcodeHasItemEquipped, opcodeHasItemEquippedExplicit);
            extensions.registerFunction ("hassoulgem", 'l', "c", opcodeHasSoulGem, opcodeHasSoulGemExplicit);
            extensions.registerFunction ("getweapontype", 'l', "", opcodeGetWeaponType, opcodeGetWeaponTypeExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
             interpreter.installSegment5 (opcodeAddItem, new OpAddItem<ImplicitRef>);
             interpreter.installSegment5 (opcodeAddItemExplicit, new OpAddItem<ExplicitRef>);
             interpreter.installSegment5 (opcodeGetItemCount, new OpGetItemCount<ImplicitRef>);
             interpreter.installSegment5 (opcodeGetItemCountExplicit, new OpGetItemCount<ExplicitRef>);
             interpreter.installSegment5 (opcodeRemoveItem, new OpRemoveItem<ImplicitRef>);
             interpreter.installSegment5 (opcodeRemoveItemExplicit, new OpRemoveItem<ExplicitRef>);
             interpreter.installSegment5 (opcodeEquip, new OpEquip<ImplicitRef>);
             interpreter.installSegment5 (opcodeEquipExplicit, new OpEquip<ExplicitRef>);
             interpreter.installSegment5 (opcodeGetArmorType, new OpGetArmorType<ImplicitRef>);
             interpreter.installSegment5 (opcodeGetArmorTypeExplicit, new OpGetArmorType<ExplicitRef>);
             interpreter.installSegment5 (opcodeHasItemEquipped, new OpHasItemEquipped<ImplicitRef>);
             interpreter.installSegment5 (opcodeHasItemEquippedExplicit, new OpHasItemEquipped<ExplicitRef>);
             interpreter.installSegment5 (opcodeHasSoulGem, new OpHasSoulGem<ImplicitRef>);
             interpreter.installSegment5 (opcodeHasSoulGemExplicit, new OpHasSoulGem<ExplicitRef>);
             interpreter.installSegment5 (opcodeGetWeaponType, new OpGetWeaponType<ImplicitRef>);
             interpreter.installSegment5 (opcodeGetWeaponTypeExplicit, new OpGetWeaponType<ExplicitRef>);
        }
    }
}
