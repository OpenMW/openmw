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

            bool canDeclareLocals() const override;
            ///< Is the compiler allowed to declare local variables?

            char getGlobalType (const std::string& name) const override;
            ///< 'l: long, 's': short, 'f': float, ' ': does not exist.

            std::pair<char, bool> getMemberType (const std::string& name,
                const std::string& id) const override;
            ///< Return type of member variable \a name in script \a id or in script of reference of
            /// \a id
            /// \return first: 'l: long, 's': short, 'f': float, ' ': does not exist.
            /// second: true: script of reference

            bool isId (const std::string& name) const override;
            ///< Does \a name match an ID, that can be referenced?

            bool isJournalId (const std::string& name) const override;
            ///< Does \a name match a journal ID?

            void invalidateIds();

            void clear();
            ///< Remove all cached data.

            /// \return Were there any locals that needed clearing?
            bool clearLocals (const std::string& script);
    };
}

#endif
