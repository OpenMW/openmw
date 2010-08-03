
#include "class.hpp"

#include <stdexcept>

#include "ptr.hpp"
#include "nullaction.hpp"

namespace MWWorld
{
    std::map<std::string, boost::shared_ptr<Class> > Class::sClasses;

    Class::Class() {}

    Class::~Class() {}

    MWMechanics::CreatureStats& Class::getCreatureStats (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have creature stats");
    }

    bool Class::hasItemHealth (const Ptr& ptr) const
    {
        return false;
    }

    int Class::getItemMaxHealth (const Ptr& ptr) const
    {
        throw std::runtime_error ("class does not have item health");
    }

    boost::shared_ptr<Action> Class::activate (const Ptr& ptr) const
    {
        return boost::shared_ptr<Action> (new NullAction);
    }

    boost::shared_ptr<Action> Class::use (const Ptr& ptr) const
    {
        return boost::shared_ptr<Action> (new NullAction);
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
}
