
#include "class.hpp"

#include <stdexcept>

namespace MWWorld
{
    std::map<std::string, boost::shared_ptr<Class> > Class::sClasses;

    Class::Class() {}

    Class::~Class() {}

    const Class& Class::get (const std::string& key)
    {
        std::map<std::string, boost::shared_ptr<Class> >::const_iterator iter = sClasses.find (key);

        if (iter==sClasses.end())
            throw std::logic_error ("unknown class key: " + key);

        return *iter->second;
    }

    void Class::registerClass (const std::string& key,  boost::shared_ptr<Class> instance)
    {
        sClasses.insert (std::make_pair (key, instance));
    }
}
