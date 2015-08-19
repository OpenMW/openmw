#include "ingredient.hpp"

#include <components/esm/loadingr.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/actioneat.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Ingredient::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->mBase->mId;
    }

    void Ingredient::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Ingredient::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Ingredient::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Ingredient::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Ingredient::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Ingredient::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->mBase->mScript;
    }

    int Ingredient::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->mBase->mData.mValue;
    }


    boost::shared_ptr<MWWorld::Action> Ingredient::use (const MWWorld::Ptr& ptr) const
    {
        boost::shared_ptr<MWWorld::Action> action (new MWWorld::ActionEat (ptr));

        action->setSound ("Swallow");

        return action;
    }

    void Ingredient::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Ingredient);

        registerClass (typeid (ESM::Ingredient).name(), instance);
    }

    std::string Ingredient::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Ingredient Up");
    }

    std::string Ingredient::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Ingredient Down");
    }

    std::string Ingredient::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->mBase->mIcon;
    }

    bool Ingredient::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Ingredient::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::NpcStats& npcStats = player.getClass().getNpcStats (player);
        int alchemySkill = npcStats.getSkill (ESM::Skill::Alchemy).getBase();

        static const float fWortChanceValue =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWortChanceValue")->getFloat();

        MWGui::Widgets::SpellEffectList list;
        for (int i=0; i<4; ++i)
        {
            if (ref->mBase->mData.mEffectID[i] < 0)
                continue;
            MWGui::Widgets::SpellEffectParams params;
            params.mEffectID = ref->mBase->mData.mEffectID[i];
            params.mAttribute = ref->mBase->mData.mAttributes[i];
            params.mSkill = ref->mBase->mData.mSkills[i];

            params.mKnown = ( (i == 0 && alchemySkill >= fWortChanceValue)
                 || (i == 1 && alchemySkill >= fWortChanceValue*2)
                 || (i == 2 && alchemySkill >= fWortChanceValue*3)
                 || (i == 3 && alchemySkill >= fWortChanceValue*4));

            list.push_back(params);
        }
        info.effects = list;

        info.text = text;

        return info;
    }

    MWWorld::Ptr
    Ingredient::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return MWWorld::Ptr(&cell.get<ESM::Ingredient>().insert(*ref), &cell);
    }

    bool Ingredient::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Ingredients) != 0;
    }


    float Ingredient::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();
        return ref->mBase->mData.mWeight;
    }
}
