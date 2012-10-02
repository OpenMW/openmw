
#include "ingredient.hpp"

#include <components/esm/loadingr.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/actioneat.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Ingredient::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->mId;
    }
    
    void Ingredient::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Ingredient::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }

    std::string Ingredient::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Ingredient::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->mName;
    }

    boost::shared_ptr<MWWorld::Action> Ingredient::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTake (ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    std::string Ingredient::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->mScript;
    }

    int Ingredient::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->mData.mValue;
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

        return ref->base->mIcon;
    }

    bool Ingredient::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return (ref->base->mName != "");
    }

    MWGui::ToolTipInfo Ingredient::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->base->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->base->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->base->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->mScript, "Script");
        }

        MWGui::Widgets::SpellEffectList list;
        for (int i=0; i<4; ++i)
        {
            if (ref->base->mData.mEffectID[i] < 0)
                continue;
            MWGui::Widgets::SpellEffectParams params;
            params.mEffectID = ref->base->mData.mEffectID[i];
            params.mAttribute = ref->base->mData.mAttributes[i];
            params.mSkill = ref->base->mData.mSkills[i];
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

        return MWWorld::Ptr(&cell.ingreds.insert(*ref), &cell);
    }
}
