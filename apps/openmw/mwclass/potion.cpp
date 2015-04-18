
#include "potion.hpp"

#include <components/esm/loadalch.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionapply.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace MWClass
{
    std::string Potion::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Potion>()->mBase->mId;
    }

    void Potion::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Potion::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model, true);
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
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        info.effects = MWGui::Widgets::MWEffectList::effectListFromESM(&ref->mBase->mEffects);

        // hide effects the player doesnt know about
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::NpcStats& npcStats = player.getClass().getNpcStats (player);
        int alchemySkill = npcStats.getSkill (ESM::Skill::Alchemy).getBase();
        int i=0;
        static const float fWortChanceValue =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWortChanceValue")->getFloat();
        for (MWGui::Widgets::SpellEffectList::iterator it = info.effects.begin(); it != info.effects.end(); ++it)
        {
            it->mKnown = (i <= 1 && alchemySkill >= fWortChanceValue)
                 || (i <= 3 && alchemySkill >= fWortChanceValue*2);
            ++i;
        }

        info.isPotion = true;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> Potion::use (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        boost::shared_ptr<MWWorld::Action> action (
            new MWWorld::ActionApply (ptr, ref->mBase->mId));

        action->setSound ("Drink");

        return action;
    }

    MWWorld::Ptr
    Potion::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return MWWorld::Ptr(&cell.get<ESM::Potion>().insert(*ref), &cell);
    }

    bool Potion::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Potions) != 0;
    }

    float Potion::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();
        return ref->mBase->mData.mWeight;
    }
}
