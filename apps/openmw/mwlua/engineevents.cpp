#include "engineevents.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwnet/messageentry.hpp"
#include "../mwnet/networkmanager.hpp"

#include "globalscripts.hpp"
#include "localscripts.hpp"
#include "object.hpp"

namespace MWLua
{

    class EngineEvents::Visitor
    {
    public:
        explicit Visitor(GlobalScripts& globalScripts)
            : mGlobalScripts(globalScripts)
        {
        }

        void operator()(const OnActive& event) const
        {
            MWWorld::Ptr ptr = getPtr(event.mObject);
            if (ptr.isEmpty())
                return;
            if (ptr.getCellRef().getRefId() == "player")
            {
                const auto& netMan = MWBase::Environment::get().getNetworkManager();
                if (!netMan->isServer())
                {
                }
                else
                {
                    mGlobalScripts.playerAdded(GObject(ptr));
                }
            }
            else
            {
                mGlobalScripts.objectActive(GObject(ptr));
                const MWWorld::Class& objClass = ptr.getClass();
                if (objClass.isActor())
                    mGlobalScripts.actorActive(GObject(ptr));
                if (objClass.isItem(ptr))
                    mGlobalScripts.itemActive(GObject(ptr));
            }
            if (auto* scripts = getLocalScripts(ptr))
                scripts->setActive(true);
        }

        void operator()(const OnInactive& event) const
        {
            if (auto* scripts = getLocalScripts(event.mObject))
                scripts->setActive(false);
        }

        void operator()(const OnTeleported& event) const
        {
            if (auto* scripts = getLocalScripts(event.mObject))
                scripts->onTeleported();
        }

        void operator()(const OnActivate& event) const
        {
            MWWorld::Ptr obj = getPtr(event.mObject);
            MWWorld::Ptr actor = getPtr(event.mActor);
            if (actor.isEmpty() || obj.isEmpty())
                return;
            const auto activationMessage = std::make_shared<MWNet::UseOrActivationMessageEntry>(obj, actor, true);
            MWBase::Environment::get().getNetworkManager()->queueMessage(activationMessage);
            if (auto* scripts = getLocalScripts(obj))
                scripts->onActivated(LObject(actor));
        }

        void operator()(const OnUseItem& event) const
        {
            MWWorld::Ptr obj = getPtr(event.mObject);
            MWWorld::Ptr actor = getPtr(event.mActor);
            if (actor.isEmpty() || obj.isEmpty())
                return;
            const auto activationMessage
                = std::make_shared<MWNet::UseOrActivationMessageEntry>(obj, actor, false, event.mForce);
            MWBase::Environment::get().getNetworkManager()->queueMessage(std::move(activationMessage));
        }

        void operator()(const OnConsume& event) const
        {
            MWWorld::Ptr actor = getPtr(event.mActor);
            MWWorld::Ptr consumable = getPtr(event.mConsumable);
            if (actor.isEmpty() || consumable.isEmpty())
                return;
            if (auto* scripts = getLocalScripts(actor))
                scripts->onConsume(LObject(consumable));
        }

        void operator()(const OnNewExterior& event) const { mGlobalScripts.onNewExterior(GCell{ &event.mCell }); }

        void operator()(const OnAnimationTextKey& event) const
        {
            MWWorld::Ptr actor = getPtr(event.mActor);
            if (actor.isEmpty())
                return;
            if (auto* scripts = getLocalScripts(actor))
                scripts->onAnimationTextKey(event.mGroupname, event.mKey);
        }

        void operator()(const OnSkillUse& event) const
        {
            MWWorld::Ptr actor = getPtr(event.mActor);
            if (actor.isEmpty())
                return;
            if (auto* scripts = getLocalScripts(actor))
                scripts->onSkillUse(event.mSkill, event.useType, event.scale);
        }

        void operator()(const OnSkillLevelUp& event) const
        {
            MWWorld::Ptr actor = getPtr(event.mActor);
            if (actor.isEmpty())
                return;
            if (auto* scripts = getLocalScripts(actor))
                scripts->onSkillLevelUp(event.mSkill, event.mSource);
        }

    private:
        MWWorld::Ptr getPtr(ESM::RefNum id) const
        {
            MWWorld::Ptr res = mWorldModel->getPtr(id);
            if (res.isEmpty() && Settings::lua().mLuaDebug)
                Log(Debug::Verbose) << "Can not find object" << id.toString() << " when calling engine hanglers";
            return res;
        }

        LocalScripts* getLocalScripts(const MWWorld::Ptr& ptr) const
        {
            if (ptr.isEmpty())
                return nullptr;
            else
                return ptr.getRefData().getLuaScripts();
        }

        LocalScripts* getLocalScripts(ESM::RefNum id) const { return getLocalScripts(getPtr(id)); }

        GlobalScripts& mGlobalScripts;
        MWWorld::WorldModel* mWorldModel = MWBase::Environment::get().getWorldModel();
    };

    void EngineEvents::callEngineHandlers()
    {
        Visitor vis(mGlobalScripts);
        for (const Event& event : mQueue)
            std::visit(vis, event);
        mQueue.clear();
    }

}
