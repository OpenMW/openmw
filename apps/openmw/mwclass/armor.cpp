
#include "armor.hpp"

#include <components/esm/loadarmo.hpp>
#include <components/esm/loadskil.hpp>
#include <components/esm/loadgmst.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Armor::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Armor::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }
    
    std::string Armor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Armor::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Armor::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWBase::Environment::get().getSoundManager()->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    bool Armor::hasItemHealth (const MWWorld::Ptr& ptr) const
    {
        return true;
    }

    int Armor::getItemMaxHealth (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->data.health;
    }

    std::string Armor::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->script;
    }

    std::pair<std::vector<int>, bool> Armor::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        std::vector<int> slots;

        const int size = 11;

        static const int sMapping[size][2] =
        {
            { ESM::Armor::Helmet, MWWorld::InventoryStore::Slot_Helmet },
            { ESM::Armor::Cuirass, MWWorld::InventoryStore::Slot_Cuirass },
            { ESM::Armor::LPauldron, MWWorld::InventoryStore::Slot_LeftPauldron },
            { ESM::Armor::RPauldron, MWWorld::InventoryStore::Slot_RightPauldron },
            { ESM::Armor::Greaves, MWWorld::InventoryStore::Slot_Greaves },
            { ESM::Armor::Boots, MWWorld::InventoryStore::Slot_Boots },
            { ESM::Armor::LGauntlet, MWWorld::InventoryStore::Slot_LeftGauntlet },
            { ESM::Armor::RGauntlet, MWWorld::InventoryStore::Slot_RightGauntlet },
            { ESM::Armor::Shield, MWWorld::InventoryStore::Slot_CarriedLeft },
            { ESM::Armor::LBracer, MWWorld::InventoryStore::Slot_LeftGauntlet },
            { ESM::Armor::RBracer, MWWorld::InventoryStore::Slot_RightGauntlet }
        };

        for (int i=0; i<size; ++i)
            if (sMapping[i][0]==ref->base->data.type)
            {
                slots.push_back (int (sMapping[i][1]));
                break;
            }

        return std::make_pair (slots, false);
    }

    int Armor::getEquipmentSkill (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        std::string typeGmst;

        switch (ref->base->data.type)
        {
            case ESM::Armor::Helmet: typeGmst = "iHelmWeight"; break;
            case ESM::Armor::Cuirass: typeGmst = "iCuirassWeight"; break;
            case ESM::Armor::LPauldron:
            case ESM::Armor::RPauldron: typeGmst = "iPauldronWeight"; break;
            case ESM::Armor::Greaves: typeGmst = "iGreavesWeight"; break;
            case ESM::Armor::Boots: typeGmst = "iBootsWeight"; break;
            case ESM::Armor::LGauntlet:
            case ESM::Armor::RGauntlet: typeGmst = "iGauntletWeight"; break;
            case ESM::Armor::Shield: typeGmst = "iShieldWeight"; break;
            case ESM::Armor::LBracer:
            case ESM::Armor::RBracer: typeGmst = "iGauntletWeight"; break;
        }

        if (typeGmst.empty())
            return -1;

        float iWeight = MWBase::Environment::get().getWorld()->getStore().gameSettings.find (typeGmst)->i;

        if (iWeight * MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fLightMaxMod")->f>=
            ref->base->data.weight)
            return ESM::Skill::LightArmor;

        if (iWeight * MWBase::Environment::get().getWorld()->getStore().gameSettings.find ("fMedMaxMod")->f>=
            ref->base->data.weight)
            return ESM::Skill::MediumArmor;

        return ESM::Skill::HeavyArmor;
    }

    int Armor::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->data.value;
    }

    void Armor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Armor);

        registerClass (typeid (ESM::Armor).name(), instance);
    }

    std::string Armor::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        int es = getEquipmentSkill(ptr);
        if (es == ESM::Skill::LightArmor)
            return std::string("Item Armor Light Up");
        else if (es == ESM::Skill::MediumArmor)
            return std::string("Item Armor Medium Up");
        else
            return std::string("Item Armor Heavy Up");
    }

    std::string Armor::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        int es = getEquipmentSkill(ptr);
        if (es == ESM::Skill::LightArmor)
            return std::string("Item Armor Light Down");
        else if (es == ESM::Skill::MediumArmor)
            return std::string("Item Armor Medium Down");
        else
            return std::string("Item Armor Heavy Down");
    }

    std::string Armor::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->icon;
    }

    bool Armor::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Armor::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->base->icon;

        std::string text;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        // get armor type string (light/medium/heavy)
        int armorType = getEquipmentSkill(ptr);
        std::string typeText;
        if (armorType == ESM::Skill::LightArmor)
            typeText = store.gameSettings.search("sLight")->str;
        else if (armorType == ESM::Skill::MediumArmor)
            typeText = store.gameSettings.search("sMedium")->str;
        else
            typeText = store.gameSettings.search("sHeavy")->str;

        text += "\n" + store.gameSettings.search("sArmorRating")->str + ": " + MWGui::ToolTips::toString(ref->base->data.armor);

        /// \todo store the current armor health somewhere
        text += "\n" + store.gameSettings.search("sCondition")->str + ": " + MWGui::ToolTips::toString(ref->base->data.health);

        text += "\n" + store.gameSettings.search("sWeight")->str + ": " + MWGui::ToolTips::toString(ref->base->data.weight) + " (" + typeText + ")";
        text += MWGui::ToolTips::getValueString(ref->base->data.value, store.gameSettings.search("sValue")->str);

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.owner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        }

        info.enchant = ref->base->enchant;

        info.text = text;

        return info;
    }

    std::string Armor::getEnchantment (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return ref->base->enchant;
    }

    boost::shared_ptr<MWWorld::Action> Armor::use (const MWWorld::Ptr& ptr) const
    {
        MWBase::Environment::get().getSoundManager()->playSound (getUpSoundId(ptr), 1.0, 1.0);

        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionEquip(ptr));
    }

    MWWorld::Ptr
    Armor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Armor> *ref =
            ptr.get<ESM::Armor>();

        return MWWorld::Ptr(&cell.armors.insert(*ref), &cell);
    }
}
