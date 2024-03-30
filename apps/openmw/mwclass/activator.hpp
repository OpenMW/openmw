#ifndef GAME_MWCLASS_ACTIVATOR_H
#define GAME_MWCLASS_ACTIVATOR_H

#include "../mwworld/registeredclass.hpp"

namespace MWClass
{
    class Activator final : public MWWorld::RegisteredClass<Activator>
    {
        friend MWWorld::RegisteredClass<Activator>;

        Activator();

        MWWorld::Ptr copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const override;

        static int getSndGenTypeFromName(std::string_view name);

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

        MWGui::ToolTipInfo getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const override;
        ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

        ESM::RefId getScript(const MWWorld::ConstPtr& ptr) const override;
        ///< Return name of the script attached to ptr

        std::unique_ptr<MWWorld::Action> activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const override;
        ///< Generate action for activation

        std::string_view getModel(const MWWorld::ConstPtr& ptr) const override;

        bool useAnim() const override;
        ///< Whether or not to use animated variant of model (default false)

        bool isActivator() const override;

        ESM::RefId getSoundIdFromSndGen(const MWWorld::Ptr& ptr, std::string_view name) const override;
    };
}

#endif
