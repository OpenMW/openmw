
#include "potion.hpp"

#include <components/esm/loadalch.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionapply.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace MWClass
{
    void Potion::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Potion::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr,true);
    }

    std::string Potion::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Potion::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Potion::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Potion::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return ref->mBase->mScript;
    }

    int Potion::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return ref->mBase->mData.mValue;
    }

    void Potion::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Potion);

        registerClass (typeid (ESM::Potion).name(), instance);
    }

    std::string Potion::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Potion Up");
    }

    std::string Potion::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Potion Down");
    }

    std::string Potion::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return ref->mBase->mIcon;
    }

    bool Potion::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Potion::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        info.effects = MWGui::Widgets::MWEffectList::effectListFromESM(&ref->mBase->mEffects);

        // hide effects the player doesnt know about
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::NpcStats& npcStats = MWWorld::Class::get(player).getNpcStats (player);
        int alchemySkill = npcStats.getSkill (ESM::Skill::Alchemy).getBase();
        int i=0;
        static const float fWortChanceValue =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWortChanceValue")->getFloat();
        for (MWGui::Widgets::SpellEffectList::iterator it = info.effects.begin(); it != info.effects.end(); ++it)
        {
            it->mKnown = ( (i == 0 && alchemySkill >= fWortChanceValue)
                 || (i == 1 && alchemySkill >= fWortChanceValue*2)
                 || (i == 2 && alchemySkill >= fWortChanceValue*3)
                 || (i == 3 && alchemySkill >= fWortChanceValue*4));

            ++i;
        }

        info.isPotion = true;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mRef.mFaction, "Faction");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> Potion::use (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        MWWorld::Ptr actor = MWBase::Environment::get().getWorld()->getPlayerPtr();

        // remove used potion (assume it is present in inventory)
        ptr.getContainerStore()->remove(ptr, 1, actor);

        boost::shared_ptr<MWWorld::Action> action (
            new MWWorld::ActionApply (actor, ref->mBase->mId));

        action->setSound ("Drink");

        return action;
    }

    MWWorld::Ptr
    Potion::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return MWWorld::Ptr(&cell.mPotions.insert(*ref), &cell);
    }

    bool Potion::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return npcServices & ESM::NPC::Potions;
    }

    float Potion::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();
        return ref->mBase->mData.mWeight;
    }
}
