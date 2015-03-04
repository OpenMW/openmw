
#include "class.hpp"

#include <stdexcept>

#include <OgreVector3.h>

#include <components/esm/defs.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

#include "ptr.hpp"
#include "refdata.hpp"
#include "nullaction.hpp"
#include "failedaction.hpp"
#include "actiontake.hpp"
#include "containerstore.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/magiceffects.hpp"

namespace MWWorld
{
    std::map<std::string, boost::shared_ptr<Class> > Class::sClasses;

    Class::Class() {}

    Class::~Class() {}

    std::string Class::getId (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not support ID retrieval");
    }

    void Class::insertObjectRendering (const Ptr& ptr, const std::string& mesh, MWRender::RenderingInterface& renderingInterface) const
    {

    }

    void Class::insertObject(const Ptr& ptr, const std::string& mesh, MWWorld::PhysicsSystem& physics) const
    {

    }

    bool Class::apply (const MWWorld::Ptr& ptr, const std::string& id,  const MWWorld::Ptr& actor) const
    {
        return false;
    }

    void Class::skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType, float extraFactor) const
    {
        throw std::runtime_error ("class does not represent an actor");
    }

    bool Class::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return false;
    }

    int Class::getServices(const Ptr &actor) const
    {
        throw std::runtime_error ("class does not have services");
    }

    MWMechanics::CreatureStats& Class::getCreatureStats (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have creature stats");
    }

    MWMechanics::NpcStats& Class::getNpcStats (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have NPC stats");
    }

    bool Class::hasItemHealth (const Ptr& ptr) const
    {
        return false;
    }

    int Class::getItemHealth(const Ptr &ptr) const
    {
        if (ptr.getCellRef().getCharge() == -1)
            return getItemMaxHealth(ptr);
        else
            return ptr.getCellRef().getCharge();
    }

    int Class::getItemMaxHealth (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have item health");
    }

    void Class::hit(const Ptr& ptr, int type) const
    {
        throw std::runtime_error("class cannot hit");
    }

    void Class::block(const Ptr &ptr) const
    {
        throw std::runtime_error("class cannot block");
    }

    void Class::onHit(const Ptr& ptr, float damage, bool ishealth, const Ptr& object, const Ptr& attacker, bool successful) const
    {
        throw std::runtime_error("class cannot be hit");
    }

    void Class::setActorHealth(const Ptr& ptr, float health, const Ptr& attacker) const
    {
        throw std::runtime_error("class does not have actor health");
    }

    boost::shared_ptr<Action> Class::activate (const Ptr& ptr, const Ptr& actor) const
    {
        return boost::shared_ptr<Action> (new NullAction);
    }

    boost::shared_ptr<Action> Class::use (const Ptr& ptr) const
    {
        return boost::shared_ptr<Action> (new NullAction);
    }

    ContainerStore& Class::getContainerStore (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have a container store");
    }

    InventoryStore& Class::getInventoryStore (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have an inventory store");
    }

    bool Class::hasInventoryStore(const Ptr &ptr) const
    {
        return false;
    }

    void Class::lock (const Ptr& ptr, int lockLevel) const
    {
        throw std::runtime_error ("class does not support locking");
    }

    void Class::unlock (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not support unlocking");
    }

    void Class::setRemainingUsageTime (const Ptr& ptr, float duration) const
    {
        throw std::runtime_error ("class does not support time-based uses");
    }

    float Class::getRemainingUsageTime (const Ptr& ptr) const
    {
        return -1;
    }

    std::string Class::getScript (const Ptr& ptr) const
    {
        return "";
    }

    float Class::getSpeed (const Ptr& ptr) const
    {
        return 0;
    }

    float Class::getJump (const Ptr& ptr) const
    {
        return 0;
    }

    int Class::getEnchantmentPoints (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error ("class does not support enchanting");
    }

    MWMechanics::Movement& Class::getMovementSettings (const Ptr& ptr) const
    {
        throw std::runtime_error ("movement settings not supported by class");
    }

    Ogre::Vector3 Class::getMovementVector (const Ptr& ptr) const
    {
        return Ogre::Vector3 (0, 0, 0);
    }

    Ogre::Vector3 Class::getRotationVector (const Ptr& ptr) const
    {
        return Ogre::Vector3 (0, 0, 0);
    }

    std::pair<std::vector<int>, bool> Class::getEquipmentSlots (const Ptr& ptr) const
    {
        return std::make_pair (std::vector<int>(), false);
    }

    int Class::getEquipmentSkill (const Ptr& ptr) const
    {
        return -1;
    }

    int Class::getValue (const Ptr& ptr) const
    {
        throw std::logic_error ("value not supported by this class");
    }

    float Class::getCapacity (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error ("capacity not supported by this class");
    }

    float Class::getWeight(const Ptr &ptr) const
    {
        throw std::runtime_error ("weight not supported by this class");
    }

    float Class::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error ("encumbrance not supported by class");
    }

    bool Class::isEssential (const MWWorld::Ptr& ptr) const
    {
        return false;
    }

    float Class::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error("Class does not support armor rating");
    }

    const Class& Class::get (const std::string& key)
    {
        if (key.empty())
            throw std::logic_error ("Class::get(): attempting to get an empty key");

        std::map<std::string, boost::shared_ptr<Class> >::const_iterator iter = sClasses.find (key);

        if (iter==sClasses.end())
            throw std::logic_error ("Class::get(): unknown class key: " + key);

        return *iter->second;
    }

    bool Class::isPersistent(const Ptr &ptr) const
    {
        throw std::runtime_error ("class does not support persistence");
    }

    void Class::registerClass(const std::string& key,  boost::shared_ptr<Class> instance)
    {
        instance->mTypeName = key;
        sClasses.insert(std::make_pair(key, instance));
    }

    std::string Class::getUpSoundId (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have an up sound");
    }

    std::string Class::getDownSoundId (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have an down sound");
    }

    std::string Class::getSoundIdFromSndGen(const Ptr &ptr, const std::string &type) const
    {
        throw std::runtime_error("class does not support soundgen look up");
    }

    std::string Class::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have any inventory icon");
    }

    MWGui::ToolTipInfo Class::getToolTipInfo (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have a tool tip");
    }

    bool Class::hasToolTip (const Ptr& ptr) const
    {
        return false;
    }

    std::string Class::getEnchantment (const Ptr& ptr) const
    {
        return "";
    }

    void Class::adjustScale(const MWWorld::Ptr& ptr,float& scale) const
    {
    }

    std::string Class::getModel(const MWWorld::Ptr &ptr) const
    {
        return "";
    }

    std::string Class::applyEnchantment(const MWWorld::Ptr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        throw std::runtime_error ("class can't be enchanted");
    }

    std::pair<int, std::string> Class::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
        return std::make_pair (1, "");
    }

    void Class::adjustPosition(const MWWorld::Ptr& ptr, bool force) const
    {
    }

    boost::shared_ptr<Action> Class::defaultItemActivate(const Ptr &ptr, const Ptr &actor) const
    {
        if(!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return boost::shared_ptr<Action>(new NullAction());

        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfItem");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        boost::shared_ptr<MWWorld::Action> action(new ActionTake(ptr));
        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr
    Class::copyToCellImpl(const Ptr &ptr, CellStore &cell) const
    {
        throw std::runtime_error("unable to move class to cell");
    }

    MWWorld::Ptr
    Class::copyToCell(const Ptr &ptr, CellStore &cell) const
    {
        Ptr newPtr = copyToCellImpl(ptr, cell);
        newPtr.getCellRef().unsetRefNum(); // This RefNum is only valid within the original cell of the reference
        return newPtr;
    }

    MWWorld::Ptr
    Class::copyToCell(const Ptr &ptr, CellStore &cell, const ESM::Position &pos) const
    {
        Ptr newPtr = copyToCell(ptr, cell);
        newPtr.getRefData().setPosition(pos);

        return newPtr;
    }

    bool Class::isBipedal(const Ptr &ptr) const
    {
        return false;
    }

    bool Class::canFly(const Ptr &ptr) const
    {
        return false;
    }

    bool Class::canSwim(const Ptr &ptr) const
    {
        return false;
    }

    bool Class::canWalk(const Ptr &ptr) const
    {
        return false;
    }

    bool Class::isPureWaterCreature(const MWWorld::Ptr& ptr) const
    {
        return canSwim(ptr) && !canWalk(ptr);
    }

    bool Class::isMobile(const MWWorld::Ptr& ptr) const
    {
        return canSwim(ptr) || canWalk(ptr) || canFly(ptr);
    }

    int Class::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        throw std::runtime_error("class does not support skills");
    }

    int Class::getBloodTexture (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error("class does not support gore");
    }

    void Class::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const {}

    void Class::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state) const {}

    int Class::getBaseGold(const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error("class does not support base gold");
    }

    bool Class::isClass(const MWWorld::Ptr& ptr, const std::string &className) const
    {
        return false;
    }

    int Class::getDoorState (const MWWorld::Ptr &ptr) const
    {
        throw std::runtime_error("this is not a door");
    }

    void Class::setDoorState (const MWWorld::Ptr &ptr, int state) const
    {
        throw std::runtime_error("this is not a door");
    }

    float Class::getNormalizedEncumbrance(const Ptr &ptr) const
    {
        float capacity = getCapacity(ptr);
        if (capacity == 0)
            return 1.f;

        return getEncumbrance(ptr) / capacity;
    }

    std::string Class::getSound(const MWWorld::Ptr&) const
    {
      return std::string();
    }

    int Class::getBaseFightRating(const Ptr &ptr) const
    {
        throw std::runtime_error("class does not support fight rating");
    }

    std::string Class::getPrimaryFaction (const MWWorld::Ptr& ptr) const
    {
        return std::string();
    }
    int Class::getPrimaryFactionRank (const MWWorld::Ptr& ptr) const
    {
        return -1;
    }

    int Class::getEffectiveArmorRating(const Ptr &ptr, const Ptr &actor) const
    {
        throw std::runtime_error("class does not support armor ratings");
    }
}
