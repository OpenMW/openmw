#include "potion.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadnpc.hpp>

#include "../mwworld/actionapply.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/alchemy.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "classmodel.hpp"
#include "nameorid.hpp"

namespace MWClass
{
    Potion::Potion()
        : MWWorld::RegisteredClass<Potion>(ESM::Potion::sRecordId)
    {
    }

    void Potion::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    std::string_view Potion::getModel(const MWWorld::ConstPtr& ptr) const
    {
        return getClassModel<ESM::Potion>(ptr);
    }

    std::string_view Potion::getName(const MWWorld::ConstPtr& ptr) const
    {
        return getNameOrId<ESM::Potion>(ptr);
    }

    std::unique_ptr<MWWorld::Action> Potion::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    ESM::RefId Potion::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion>* ref = ptr.get<ESM::Potion>();

        return ref->mBase->mScript;
    }

    int Potion::getValue(const MWWorld::ConstPtr& ptr) const
    {
        return MWMechanics::getPotionValue(*ptr.get<ESM::Potion>()->mBase);
    }

    const ESM::RefId& Potion::getUpSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const auto sound = ESM::RefId::stringRefId("Item Potion Up");
        return sound;
    }

    const ESM::RefId& Potion::getDownSoundId(const MWWorld::ConstPtr& ptr) const
    {
        static const auto sound = ESM::RefId::stringRefId("Item Potion Down");
        return sound;
    }

    const std::string& Potion::getInventoryIcon(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion>* ref = ptr.get<ESM::Potion>();

        return ref->mBase->mIcon;
    }

    MWGui::ToolTipInfo Potion::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Potion>* ref = ptr.get<ESM::Potion>();

        MWGui::ToolTipInfo info;
        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name)) + MWGui::ToolTips::getCountString(count);
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        info.effects = MWGui::Widgets::MWEffectList::effectListFromESM(&ref->mBase->mEffects);

        // hide effects the player doesn't know about
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        for (size_t i = 0; i < info.effects.size(); ++i)
            info.effects[i].mKnown = MWMechanics::Alchemy::knownEffect(i, player);

        info.isPotion = true;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            info.extra += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            info.extra += MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");
        }

        info.text = std::move(text);

        return info;
    }

    std::unique_ptr<MWWorld::Action> Potion::use(const MWWorld::Ptr& ptr, bool force) const
    {
        MWWorld::LiveCellRef<ESM::Potion>* ref = ptr.get<ESM::Potion>();

        auto action = std::make_unique<MWWorld::ActionApply>(ptr, ref->mBase->mId);

        action->setSound(ESM::RefId::stringRefId("Drink"));

        return action;
    }

    MWWorld::Ptr Potion::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::Potion>* ref = ptr.get<ESM::Potion>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    bool Potion::canSell(const MWWorld::ConstPtr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Potions) != 0;
    }

    float Potion::getWeight(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Potion>* ref = ptr.get<ESM::Potion>();
        return ref->mBase->mData.mWeight;
    }
}
