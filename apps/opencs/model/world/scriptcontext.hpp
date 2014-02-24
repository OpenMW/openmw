#ifndef CSM_WORLD_SCRIPTCONTEXT_H
#define CSM_WORLD_SCRIPTCONTEXT_H

#include <string>
#include <vector>
#include <map>

#include <components/compiler/context.hpp>
#include <components/compiler/locals.hpp>

namespace CSMWorld
{
    class Data;

    class ScriptContext : public Compiler::Context
    {
            const Data& mData;
            mutable std::vector<std::string> mIds;
            mutable bool mIdsUpdated;
            mutable std::map<std::string, Compiler::Locals> mLocals;

        public:

            ScriptContext (const Data& data);

            virtual bool canDeclareLocals() const;
            ///< Is the compiler allowed to declare local variables?

            virtual char getGlobalType (const std::string& name) const;
            ///< 'l: long, 's': short, 'f': float, ' ': does not exist.

            virtual std::pair<char, bool> getMemberType (const std::string& name,
                const std::string& id) const;
            ///< Return type of member variable \a name in script \a id or in script of reference of
            /// \a id
            /// \return first: 'l: long, 's': short, 'f': float, ' ': does not exist.
            /// second: true: script of reference

            virtual bool isId (const std::string& name) const;
            ///< Does \a name match an ID, that can be referenced?

            virtual bool isJournalId (const std::string& name) const;
            ///< Does \a name match a journal ID?

            void invalidateIds();

            void clear();
            ///< Remove all cached data.
    };
}

#endif
