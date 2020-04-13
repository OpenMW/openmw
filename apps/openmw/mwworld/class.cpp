#include "class.hpp"

#include <stdexcept>

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

namespace MWWorld
{
    std::map<std::string, std::shared_ptr<Class> > Class::sClasses;

    Class::Class() {}

    Class::~Class() {}

    void Class::insertObjectRendering (const Ptr& ptr, const std::string& mesh, MWRender::RenderingInterface& renderingInterface) const
    {

    }

    void Class::insertObject(const Ptr& ptr, const std::string& mesh, MWPhysics::PhysicsSystem& physics) const
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

    bool Class::canSell (const MWWorld::ConstPtr& item, int npcServices) const
    {
        return false;
    }

    int Class::getServices(const ConstPtr &actor) const
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

    bool Class::hasItemHealth (const ConstPtr& ptr) const
    {
        return false;
    }

    int Class::getItemHealth(const ConstPtr &ptr) const
    {
        if (ptr.getCellRef().getCharge() == -1)
            return getItemMaxHealth(ptr);
        else
            return ptr.getCellRef().getCharge();
    }

    float Class::getItemNormalizedHealth (const ConstPtr& ptr) const
    {
        if (getItemMaxHealth(ptr) == 0)
        {
            return 0.f;
        }
        else
        {
            return getItemHealth(ptr) / static_cast<float>(getItemMaxHealth(ptr));
        }
    }

    int Class::getItemMaxHealth (const ConstPtr& ptr) const
    {
        throw std::runtime_error ("class does not have item health");
    }

    void Class::hit(const Ptr& ptr, float attackStrength, int type) const
    {
        throw std::runtime_error("class cannot hit");
    }

    void Class::block(const Ptr &ptr) const
    {
        throw std::runtime_error("class cannot block");
    }

    void Class::onHit(const Ptr& ptr, float damage, bool ishealth, const Ptr& object, const Ptr& attacker, const osg::Vec3f& hitPosition, bool successful) const
    {
        throw std::runtime_error("class cannot be hit");
    }

    std::shared_ptr<Action> Class::activate (const Ptr& ptr, const Ptr& actor) const
    {
        return std::shared_ptr<Action> (new NullAction);
    }

    std::shared_ptr<Action> Class::use (const Ptr& ptr, bool force) const
    {
        return std::shared_ptr<Action> (new NullAction);
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

    bool Class::canLock(const ConstPtr &ptr) const
    {
        return false;
    }

    void Class::setRemainingUsageTime (const Ptr& ptr, float duration) const
    {
        throw std::runtime_error ("class does not support time-based uses");
    }

    float Class::getRemainingUsageTime (const ConstPtr& ptr) const
    {
        return -1;
    }

    std::string Class::getScript (const ConstPtr& ptr) const
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

    int Class::getEnchantmentPoints (const MWWorld::ConstPtr& ptr) const
    {
        throw std::runtime_error ("class does not support enchanting");
    }

    MWMechanics::Movement& Class::getMovementSettings (const Ptr& ptr) const
    {
        throw std::runtime_error ("movement settings not supported by class");
    }

    osg::Vec3f Class::getRotationVector (const Ptr& ptr) const
    {
        return osg::Vec3f (0, 0, 0);
    }

    std::pair<std::vector<int>, bool> Class::getEquipmentSlots (const ConstPtr& ptr) const
    {
        return std::make_pair (std::vector<int>(), false);
    }

    int Class::getEquipmentSkill (const ConstPtr& ptr) const
    {
        return -1;
    }

    int Class::getValue (const ConstPtr& ptr) const
    {
        throw std::logic_error ("value not supported by this class");
    }

    float Class::getCapacity (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error ("capacity not supported by this class");
    }

    float Class::getWeight(const ConstPtr &ptr) const
    {
        throw std::runtime_error ("weight not supported by this class");
    }

    float Class::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        throw std::runtime_error ("encumbrance not supported by class");
    }

    bool Class::isEssential (const MWWorld::ConstPtr& ptr) const
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

        std::map<std::string, std::shared_ptr<Class> >::const_iterator iter = sClasses.find (key);

        if (iter==sClasses.end())
            throw std::logic_error ("Class::get(): unknown class key: " + key);

        return *iter->second;
    }

    bool Class::isPersistent(const ConstPtr &ptr) const
    {
        throw std::runtime_error ("class does not support persistence");
    }

    void Class::registerClass(const std::string& key,  std::shared_ptr<Class> instance)
    {
        instance->mTypeName = key;
        sClasses.insert(std::make_pair(key, instance));
    }

    std::string Class::getUpSoundId (const ConstPtr& ptr) const
    {
        throw std::runtime_error ("class does not have an up sound");
    }

    std::string Class::getDownSoundId (const ConstPtr& ptr) const
    {
        throw std::runtime_error ("class does not have an down sound");
    }

    std::string Class::getSoundIdFromSndGen(const Ptr &ptr, const std::string &type) const
    {
        throw std::runtime_error("class does not support soundgen look up");
    }

    std::string Class::getInventoryIcon (const MWWorld::ConstPtr& ptr) const
    {
        throw std::runtime_error ("class does not have any inventory icon");
    }

    MWGui::ToolTipInfo Class::getToolTipInfo (const ConstPtr& ptr, int count) const
    {
        throw std::runtime_error ("class does not have a tool tip");
    }

    bool Class::showsInInventory (const ConstPtr& ptr) const
    {
        // NOTE: Don't show WerewolfRobe objects in the inventory, or allow them to be taken.
        // Vanilla likely uses a hack like this since there's no other way to prevent it from
        // being shown or taken.
        return (ptr.getCellRef().getRefId() != "werewolfrobe");
    }

    bool Class::hasToolTip (const ConstPtr& ptr) const
    {
        return true;
    }

    std::string Class::getEnchantment (const ConstPtr& ptr) const
    {
        return "";
    }

    void Class::adjustScale(const MWWorld::ConstPtr& ptr, osg::Vec3f& scale, bool rendering) const
    {
    }

    std::string Class::getModel(const MWWorld::ConstPtr &ptr) const
    {
        return "";
    }

    bool Class::useAnim() const
    {
        return false;
    }

    void Class::getModelsToPreload(const Ptr &ptr, std::vector<std::string> &models) const
    {
        std::string model = getModel(ptr);
        if (!model.empty())
            models.push_back(model);
    }

    std::string Class::applyEnchantment(const MWWorld::ConstPtr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        throw std::runtime_error ("class can't be enchanted");
    }

    std::pair<int, std::string> Class::canBeEquipped(const MWWorld::ConstPtr &ptr, const MWWorld::Ptr &npc) const
    {
        return std::make_pair (1, "");
    }

    void Class::adjustPosition(const MWWorld::Ptr& ptr, bool force) const
    {
    }

    std::shared_ptr<Action> Class::defaultItemActivate(const Ptr &ptr, const Ptr &actor) const
    {
        if(!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return std::shared_ptr<Action>(new NullAction());

        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfItem");

            std::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        std::shared_ptr<MWWorld::Action> action(new ActionTake(ptr));
        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr
    Class::copyToCellImpl(const ConstPtr &ptr, CellStore &cell) const
    {
        throw std::runtime_error("unable to copy class to cell");
    }

    MWWorld::Ptr
    Class::copyToCell(const ConstPtr &ptr, CellStore &cell, int count) const
    {
        Ptr newPtr = copyToCellImpl(ptr, cell);
        newPtr.getCellRef().unsetRefNum(); // This RefNum is only valid within the original cell of the reference
        newPtr.getRefData().setCount(count);
        return newPtr;
    }

    MWWorld::Ptr
    Class::copyToCell(const ConstPtr &ptr, CellStore &cell, const ESM::Position &pos, int count) const
    {
        Ptr newPtr = copyToCell(ptr, cell, count);
        newPtr.getRefData().setPosition(pos);

        return newPtr;
    }

    bool Class::isBipedal(const ConstPtr &ptr) const
    {
        return false;
    }

    bool Class::canFly(const ConstPtr &ptr) const
    {
        return false;
    }

    bool Class::canSwim(const ConstPtr &ptr) const
    {
        return false;
    }

    bool Class::canWalk(const ConstPtr &ptr) const
    {
        return false;
    }

    bool Class::isPureWaterCreature(const ConstPtr& ptr) const
    {
        return canSwim(ptr)
                && !isBipedal(ptr)
                && !canFly(ptr)
                && !canWalk(ptr);
    }

    bool Class::isPureFlyingCreature(const ConstPtr& ptr) const
    {
        return canFly(ptr)
                && !isBipedal(ptr)
                && !canSwim(ptr)
                && !canWalk(ptr);
    }

    bool Class::isPureLandCreature(const Ptr& ptr) const
    {
        return canWalk(ptr)
                && !isBipedal(ptr)
                && !canFly(ptr)
                && !canSwim(ptr);
    }

    bool Class::isMobile(const MWWorld::Ptr& ptr) const
    {
        return canSwim(ptr) || canWalk(ptr) || canFly(ptr);
    }

    int Class::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        throw std::runtime_error("class does not support skills");
    }

    int Class::getBloodTexture (const MWWorld::ConstPtr& ptr) const
    {
        throw std::runtime_error("class does not support gore");
    }

    void Class::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const {}

    void Class::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const {}

    int Class::getBaseGold(const MWWorld::ConstPtr& ptr) const
    {
        throw std::runtime_error("class does not support base gold");
    }

    bool Class::isClass(const MWWorld::ConstPtr& ptr, const std::string &className) const
    {
        return false;
    }

    MWWorld::DoorState Class::getDoorState (const MWWorld::ConstPtr &ptr) const
    {
        throw std::runtime_error("this is not a door");
    }

    void Class::setDoorState (const MWWorld::Ptr &ptr, MWWorld::DoorState state) const
    {
        throw std::runtime_error("this is not a door");
    }

    float Class::getNormalizedEncumbrance(const Ptr &ptr) const
    {
        float capacity = getCapacity(ptr);
        float encumbrance = getEncumbrance(ptr);

        if (encumbrance == 0)
            return 0.f;

        if (capacity == 0)
            return 1.f;

        return encumbrance / capacity;
    }

    std::string Class::getSound(const MWWorld::ConstPtr&) const
    {
      return std::string();
    }

    int Class::getBaseFightRating(const ConstPtr &ptr) const
    {
        throw std::runtime_error("class does not support fight rating");
    }

    std::string Class::getPrimaryFaction (const MWWorld::ConstPtr& ptr) const
    {
        return std::string();
    }
    int Class::getPrimaryFactionRank (const MWWorld::ConstPtr& ptr) const
    {
        return -1;
    }

    float Class::getEffectiveArmorRating(const ConstPtr &armor, const Ptr &actor) const
    {
        throw std::runtime_error("class does not support armor ratings");
    }

    osg::Vec4f Class::getEnchantmentColor(const MWWorld::ConstPtr& item) const
    {
        osg::Vec4f result(1,1,1,1);
        std::string enchantmentName = item.getClass().getEnchantment(item);
        if (enchantmentName.empty())
            return result;

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(enchantmentName);
        if (!enchantment)
            return result;

        assert (enchantment->mEffects.mList.size());

        const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().search(
                enchantment->mEffects.mList.front().mEffectID);
        if (!magicEffect)
            return result;

        result.x() = magicEffect->mData.mRed / 255.f;
        result.y() = magicEffect->mData.mGreen / 255.f;
        result.z() = magicEffect->mData.mBlue / 255.f;
        return result;
    }
}
