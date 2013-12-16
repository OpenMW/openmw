
#include "light.hpp"

#include <components/esm/loadligh.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/customdata.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace
{
    struct CustomData : public MWWorld::CustomData
    {
        float mTime;
        ///< Time remaining

        CustomData(MWWorld::Ptr ptr)
        {
            MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();
            mTime = ref->mBase->mData.mTime;
        }
        ///< Constructs this CustomData from the base values for Ptr.

        virtual MWWorld::CustomData *clone() const
        {
            return new CustomData (*this);
        }
    };
}

namespace MWClass
{
    void Light::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Light::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;

        if(!model.empty())
            physics.addObject(ptr,ref->mBase->mData.mFlags & ESM::Light::Carry);

        if (!ref->mBase->mSound.empty())
            MWBase::Environment::get().getSoundManager()->playSound3D(ptr, ref->mBase->mSound, 1.0, 1.0,
                                                                      MWBase::SoundManager::Play_TypeSfx,
                                                                      MWBase::SoundManager::Play_Loop);
    }

    std::string Light::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Light::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        if (ref->mBase->mModel.empty())
            return "";

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Light::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::NullAction());

        MWWorld::LiveCellRef<ESM::Light> *ref = ptr.get<ESM::Light>();
        if(!(ref->mBase->mData.mFlags&ESM::Light::Carry))
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::FailedAction());

        return defaultItemActivate(ptr, actor);
    }

    std::string Light::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Light::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        std::vector<int> slots_;

        if (ref->mBase->mData.mFlags & ESM::Light::Carry)
            slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedLeft));

        return std::make_pair (slots_, false);
    }

    int Light::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return ref->mBase->mData.mValue;
    }

    void Light::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Light);

        registerClass (typeid (ESM::Light).name(), instance);
    }

    std::string Light::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Misc Up");
    }

    std::string Light::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Misc Down");
    }


    std::string Light::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return ref->mBase->mIcon;
    }

    bool Light::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Light::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> Light::use (const MWWorld::Ptr& ptr) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    void Light::setRemainingUsageTime (const MWWorld::Ptr& ptr, float duration) const
    {
        ensureCustomData(ptr);

        float &timeRemaining = dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mTime;
        timeRemaining = duration;
    }

    float Light::getRemainingUsageTime (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mTime;
    }

    MWWorld::Ptr
    Light::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();

        return MWWorld::Ptr(&cell.mLights.insert(*ref), &cell);
    }

    void Light::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
            ptr.getRefData().setCustomData(new CustomData(ptr));
    }

    bool Light::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return npcServices & ESM::NPC::Lights;
    }

    float Light::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Light> *ref =
            ptr.get<ESM::Light>();
        return ref->mBase->mData.mWeight;
    }

    std::pair<int, std::string> Light::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(npc).getInventoryStore(npc);
        MWWorld::ContainerStoreIterator weapon = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

        if(weapon == invStore.end())
            return std::make_pair(1,"");

        /// \todo the 2h check is repeated many times; put it in a function
        if(weapon->getTypeName() == typeid(ESM::Weapon).name() &&
                (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::LongBladeTwoHand ||
        weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoClose ||
        weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoWide ||
        weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::SpearTwoWide ||
        weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::AxeTwoHand ||
        weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow ||
        weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow))
        {
            return std::make_pair(3,"");
        }
        return std::make_pair(1,"");
    }
}
