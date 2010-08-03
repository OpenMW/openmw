#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

namespace MWWorld
{
    /// \brief Base class for referenceable esm records
    class Class
    {
            static std::map<std::string, boost::shared_ptr<Class> > sClasses;

            // not implemented
            Class (const Class&);
            Class& operator= (const Class&);

        protected:

            Class();

        public:

            virtual ~Class();



            static const Class& get (const std::string& key);
            ///< If there is no class for this \a key, an exception is thrown.

            static void registerClass (const std::string& key,  boost::shared_ptr<Class> instance);
    };
}

#endif
