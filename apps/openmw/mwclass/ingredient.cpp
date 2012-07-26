
#include "ingredient.hpp"

#include <components/esm/loadingr.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
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

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Ingredient::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Ingredient::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWBase::Environment::get().getSoundManager()->playSound3D (ptr, getUpSoundId(ptr), 1.0, 1.0, MWSound::Play_NoTrack);

        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionTake (ptr));
    }

    std::string Ingredient::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->script;
    }

    int Ingredient::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return ref->base->data.value;
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

        return ref->base->icon;
    }

    bool Ingredient::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Ingredient::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Ingredient> *ref =
            ptr.get<ESM::Ingredient>();

        MWGui::ToolTipInfo info;
        info.caption = ref->base->name + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->base->icon;

        const ESMS::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string text;

        text += "\n" + store.gameSettings.search("sWeight")->str + ": " + MWGui::ToolTips::toString(ref->base->data.weight);
        text += MWGui::ToolTips::getValueString(ref->base->data.value, store.gameSettings.search("sValue")->str);

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->ref.owner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->base->script, "Script");
        }

        MWGui::Widgets::SpellEffectList list;
        for (int i=0; i<4; ++i)
        {
            if (ref->base->data.effectID[i] < 0)
                continue;
            MWGui::Widgets::SpellEffectParams params;
            params.mEffectID = ref->base->data.effectID[i];
            params.mAttribute = ref->base->data.attributes[i];
            params.mSkill = ref->base->data.skills[i];
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
