#ifndef GAME_MWCLASS_CREATURELEVLIST_H
#define GAME_MWCLASS_CREATURELEVLIST_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class CreatureLevList : public MWWorld::Class
    {
        void ensureCustomData (const MWWorld::Ptr& ptr) const;

        public:

            virtual std::string getName (const MWWorld::ConstPtr& ptr) const;
            ///< \return name or ID; can return an empty string.

            virtual bool hasToolTip (const MWWorld::ConstPtr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: true)

            static void registerSelf();

            virtual void getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const;
            ///< Get a list of models to preload that this object may use (directly or indirectly). default implementation: list getModel().

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const;
            ///< Read additional state from \a state into \a ptr.

            virtual void writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const;
            ///< Write additional state from \a ptr into \a state.

            virtual void respawn (const MWWorld::Ptr& ptr) const;
    };
}

#endif
