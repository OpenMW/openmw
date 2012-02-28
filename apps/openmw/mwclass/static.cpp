
#include "static.hpp"

#include <components/esm/loadstat.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwrender/objects.hpp"

namespace MWClass
{
    void Static::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        ESMS::LiveCellRef<ESM::Static, MWWorld::RefData> *ref =
            ptr.get<ESM::Static>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if (!model.empty())
        {
            MWRender::Objects& objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), true);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Static::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Static, MWWorld::RefData> *ref =
            ptr.get<ESM::Static>();

        assert (ref->base != NULL);
        const std::string &model = ref->base->model;

        if(!model.empty()){
            physics.insertObjectPhysics(ptr, "meshes\\" + model);
        }
    }

    std::string Static::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void Static::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Static);

        registerClass (typeid (ESM::Static).name(), instance);
    }
}
