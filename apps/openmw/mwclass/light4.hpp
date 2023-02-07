#ifndef OPENW_MWCLASS_LIGHT4
#define OPENW_MWCLASS_LIGHT4
#include "../mwworld/registeredclass.hpp"

namespace MWClass
{
    class ESM4Light : public MWWorld::RegisteredClass<ESM4Light>
    {
        friend MWWorld::RegisteredClass<ESM4Light>;

        ESM4Light();

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override;

    public:
        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override;
        ///< Add reference into a cell for rendering

        void insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;
        void insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation,
            MWPhysics::PhysicsSystem& physics) const override;

        std::string_view getName(const MWWorld::ConstPtr& ptr) const override;
        ///< \return name or ID; can return an empty string.

        bool hasToolTip(const MWWorld::ConstPtr& ptr) const override;
        ///< @return true if this object has a tooltip when focused (default implementation: true)

        std::string getModel(const MWWorld::ConstPtr& ptr) const override;
    };
}
#endif
