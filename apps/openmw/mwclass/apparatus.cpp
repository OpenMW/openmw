#include "apparatus.hpp"

#include <components/esm/loadappa.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionalchemy.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string Apparatus::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Apparatus>()->mBase->mId;
    }

    void Apparatus::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Apparatus::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        // TODO: add option somewhere to enable collision for placeable objects
    }

    std::string Apparatus::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Apparatus::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Apparatus::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string Apparatus::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->mBase->mScript;
    }

    int Apparatus::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->mBase->mData.mValue;
    }

    void Apparatus::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Apparatus);

        registerClass (typeid (ESM::Apparatus).name(), instance);
    }

    std::string Apparatus::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Apparatus Up");
    }

    std::string Apparatus::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Apparatus Down");
    }

    std::string Apparatus::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return ref->mBase->mIcon;
    }

    bool Apparatus::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Apparatus::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;
        text += "\n#{sQuality}: " + MWGui::ToolTips::toString(ref->mBase->mData.mQuality);
        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.mValue, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }
        info.text = text;

        return info;
    }


    boost::shared_ptr<MWWorld::Action> Apparatus::use (const MWWorld::Ptr& ptr) const
    {
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionAlchemy());
    }

    MWWorld::Ptr
    Apparatus::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();

        return MWWorld::Ptr(&cell.get<ESM::Apparatus>().insert(*ref), &cell);
    }

    bool Apparatus::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return (npcServices & ESM::NPC::Apparatus) != 0;
    }

    float Apparatus::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Apparatus> *ref =
            ptr.get<ESM::Apparatus>();
        return ref->mBase->mData.mWeight;
    }
}
