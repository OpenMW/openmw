#include "potion.hpp"

#include <components/esm/loadalch.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actionapply.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/alchemy.hpp"

namespace MWClass
{

    void Potion::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Potion::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Potion::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref = ptr.get<ESM::Potion>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Potion::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref = ptr.get<ESM::Potion>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    std::shared_ptr<MWWorld::Action> Potion::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Potion::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        return ref->mBase->mScript;
    }

    int Potion::getValue (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref = ptr.get<ESM::Potion>();

        return ref->mBase->mData.mValue;
    }

    void Potion::registerSelf()
    {
        std::shared_ptr<Class> instance (new Potion);

        registerClass (typeid (ESM::Potion).name(), instance);
    }

    std::string Potion::getUpSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Potion Up");
    }

    std::string Potion::getDownSoundId (const MWWorld::ConstPtr& ptr) const
    {
        return std::string("Item Potion Down");
    }

    std::string Potion::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref = ptr.get<ESM::Potion>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Potion::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref = ptr.get<ESM::Potion>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        info.effects = MWGui::Widgets::MWEffectList::effectListFromESM(&ref->mBase->mEffects);

        // hide effects the player doesn't know about
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        for (unsigned int i=0; i<info.effects.size(); ++i)
            info.effects[i].mKnown = MWMechanics::Alchemy::knownEffect(i, player);

        info.isPotion = true;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    std::shared_ptr<MWWorld::Action> Potion::use (const MWWorld::Ptr& ptr, bool force) const
    {
        MWWorld::LiveCellRef<ESM::Potion> *ref =
            ptr.get<ESM::Potion>();

        std::shared_ptr<MWWorld::Action> action (
            new MWWorld::ActionApply (ptr, ref->mBase->mId));

        action->setSound ("Drink");

        return action;
    }

    MWWorld::Ptr Potion::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref = ptr.get<ESM::Potion>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    bool Potion::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Potions) != 0;
    }

    float Potion::getWeight(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion> *ref = ptr.get<ESM::Potion>();
        return ref->mBase->mData.mWeight;
    }
}
