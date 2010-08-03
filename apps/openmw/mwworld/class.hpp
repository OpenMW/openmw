#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

namespace MWMechanics
{
    struct CreatureStats;
}

namespace MWWorld
{
    class Ptr;

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

            virtual MWMechanics::CreatureStats& getCreatureStats (const Ptr& ptr) const;
            ///< Return creature stats or throw an exception, if class does not have creature stats
            /// (default implementation: throw an exceoption)

            virtual bool hasItemHealth (const Ptr& ptr) const;
            ///< \return Item health data available? (default implementation: false)

            virtual int getItemMaxHealth (const Ptr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health
            /// (default implementation: throw an exceoption)

            static const Class& get (const std::string& key);
            ///< If there is no class for this \a key, an exception is thrown.

            static const Class& get (const Ptr& ptr);
            ///< If there is no class for this pointer, an exception is thrown.

            static void registerClass (const std::string& key,  boost::shared_ptr<Class> instance);
    };
}

#endif
