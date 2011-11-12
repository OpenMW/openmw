
#include "static.hpp"

#include <components/esm/loadstat.hpp>

#include "../mwworld/ptr.hpp"

#include "../mwrender/cellimp.hpp"

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
            MWRender::Objects objects = renderingInterface.getObjects();
            objects.insertBegin(ptr, ptr.getRefData().isEnabled(), false);
            objects.insertMesh(ptr, "meshes\\" + model);
        }
    }

    void Static::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Static, MWWorld::RefData> *ref =
            ptr.get<ESM::Static>();


        const std::string &model = ref->base->model;
        assert (ref->base != NULL);
        if(!model.empty()){
            physics.insertObjectPhysics(ptr);
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
