
#include "book.hpp"

#include <components/esm/loadbook.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actionread.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/window_manager.hpp"
#include "../mwgui/tooltips.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWClass
{
    void Book::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, model);
        }
    }

    void Book::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty()) {
            physics.insertObjectPhysics(ptr, model);
        }
    }
    
    std::string Book::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();
        assert(ref->base != NULL);

        const std::string &model = ref->base->model;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Book::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Book::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return boost::shared_ptr<MWWorld::Action> (
            new MWWorld::ActionRead (ptr));
    }

    std::string Book::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->base->script;
    }

    int Book::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->base->data.value;
    }

    void Book::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Book);

        registerClass (typeid (ESM::Book).name(), instance);
    }

    std::string Book::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Book Up");
    }

    std::string Book::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return std::string("Item Book Down");
    }

    std::string Book::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->base->icon;
    }

    bool Book::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return (ref->base->name != "");
    }

    MWGui::ToolTipInfo Book::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

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

        info.enchant = ref->base->enchant;

        info.text = text;

        return info;
    }

    std::string Book::getEnchantment (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return ref->base->enchant;
    }

    boost::shared_ptr<MWWorld::Action> Book::use (const MWWorld::Ptr& ptr) const
    {
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionRead(ptr));
    }

    MWWorld::Ptr
    Book::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Book> *ref =
            ptr.get<ESM::Book>();

        return MWWorld::Ptr(&cell.books.insert(*ref), &cell);
    }
}
