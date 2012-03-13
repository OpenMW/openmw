
#include "class.hpp"

#include <stdexcept>

#include <OgreVector3.h>

#include "ptr.hpp"
#include "nullaction.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    std::map<std::string, boost::shared_ptr<Class> > Class::sClasses;

    Class::Class() {}

    Class::~Class() {}

    std::string Class::getId (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not support ID retrieval");
    }

    void Class::insertObjectRendering (const Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {

    }
    void Class::insertObject(const Ptr& ptr, MWWorld::PhysicsSystem& physics, MWWorld::Environment& environment) const{

    }

    void Class::enable (const Ptr& ptr, MWWorld::Environment& environment) const
    {

    }

    void Class::disable (const Ptr& ptr, MWWorld::Environment& environment) const
    {

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

    int Class::getItemMaxHealth (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have item health");
    }

    boost::shared_ptr<Action> Class::activate (const Ptr& ptr, const Ptr& actor,
        const Environment& environment) const
    {
        return boost::shared_ptr<Action> (new NullAction);
    }

    boost::shared_ptr<Action> Class::use (const Ptr& ptr,
        const Environment& environment) const
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

    void Class::lock (const Ptr& ptr, int lockLevel) const
    {
        throw std::runtime_error ("class does not support locking");
    }

    void Class::unlock (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not support unlocking");
    }

    std::string Class::getScript (const Ptr& ptr) const
    {
        return "";
    }

    void Class::setForceStance (const Ptr& ptr, Stance stance, bool force) const
    {
        throw std::runtime_error ("stance not supported by class");
    }

    void Class::setStance (const Ptr& ptr, Stance stance, bool set) const
    {
        throw std::runtime_error ("stance not supported by class");
    }

    bool Class::getStance (const Ptr& ptr, Stance stance, bool ignoreForce) const
    {
        return false;
    }

    float Class::getSpeed (const Ptr& ptr) const
    {
        return 0;
    }

    MWMechanics::Movement& Class::getMovementSettings (const Ptr& ptr) const
    {
        throw std::runtime_error ("movement settings not supported by class");
    }

    Ogre::Vector3 Class::getMovementVector (const Ptr& ptr) const
    {
        return Ogre::Vector3 (0, 0, 0);
    }

    std::pair<std::vector<int>, bool> Class::getEquipmentSlots (const Ptr& ptr) const
    {
        return std::make_pair (std::vector<int>(), false);
    }

    int Class::getEuqipmentSkill (const Ptr& ptr, const Environment& environment) const
    {
        return -1;
    }

    const Class& Class::get (const std::string& key)
    {
        std::map<std::string, boost::shared_ptr<Class> >::const_iterator iter = sClasses.find (key);

        if (iter==sClasses.end())
            throw std::logic_error ("unknown class key: " + key);

        return *iter->second;
    }

    const Class& Class::get (const Ptr& ptr)
    {
        return get (ptr.getTypeName());
    }

    void Class::registerClass (const std::string& key,  boost::shared_ptr<Class> instance)
    {
        sClasses.insert (std::make_pair (key, instance));
    }

    std::string Class::getUpSoundId (const Ptr& ptr, const MWWorld::Environment& environment) const
    {
        throw std::runtime_error ("class does not have an up sound");
    }

    std::string Class::getDownSoundId (const Ptr& ptr, const MWWorld::Environment& environment) const
    {
        throw std::runtime_error ("class does not have an down sound");
    }
}
