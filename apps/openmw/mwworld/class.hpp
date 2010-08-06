#ifndef GAME_MWWORLD_CLASS_H
#define GAME_MWWORLD_CLASS_H

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "action.hpp"
#include "containerstore.hpp"
#include "refdata.hpp"

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

            virtual std::string getName (const Ptr& ptr) const = 0;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual MWMechanics::CreatureStats& getCreatureStats (const Ptr& ptr) const;
            ///< Return creature stats or throw an exception, if class does not have creature stats
            /// (default implementation: throw an exceoption)

            virtual bool hasItemHealth (const Ptr& ptr) const;
            ///< \return Item health data available? (default implementation: false)

            virtual int getItemMaxHealth (const Ptr& ptr) const;
            ///< Return item max health or throw an exception, if class does not have item health
            /// (default implementation: throw an exceoption)

            virtual boost::shared_ptr<Action> activate (const Ptr& ptr, const Ptr& actor,
                const Environment& environment) const;
            ///< Generate action for activation (default implementation: return a null action).

            virtual boost::shared_ptr<Action> use (const Ptr& ptr, const Environment& environment)
                const;
            ///< Generate action for using via inventory menu (default implementation: return a
            /// null action).

            virtual ContainerStore<RefData>& getContainerStore (const Ptr& ptr) const;
            ///< Return container store or throw an exception, if class does not have a
            /// container store (default implementation: throw an exceoption)

            virtual void insertIntoContainer (const Ptr& ptr, ContainerStore<RefData>& containerStore)
                const;
            ///< Insert into a container or throw an exception, if class does not support inserting into
            /// a container.

            virtual std::string getScript (const Ptr& ptr) const;
            ///< Return name of the script attached to ptr (default implementation: return an empty
            /// string).

            static const Class& get (const std::string& key);
            ///< If there is no class for this \a key, an exception is thrown.

            static const Class& get (const Ptr& ptr);
            ///< If there is no class for this pointer, an exception is thrown.

            static void registerClass (const std::string& key,  boost::shared_ptr<Class> instance);
    };
}

#endif
