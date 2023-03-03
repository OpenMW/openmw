#ifndef CSM_WORLD_SCRIPTCONTEXT_H
#define CSM_WORLD_SCRIPTCONTEXT_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <components/compiler/context.hpp>
#include <components/compiler/locals.hpp>
#include <components/misc/algorithm.hpp>

namespace CSMWorld
{
    class Data;

    class ScriptContext : public Compiler::Context
    {
        const Data& mData;
        mutable std::vector<ESM::RefId> mIds;
        mutable bool mIdsUpdated;
        mutable std::map<ESM::RefId, Compiler::Locals, std::less<>> mLocals;

    public:
        ScriptContext(const Data& data);

        bool canDeclareLocals() const override;
        ///< Is the compiler allowed to declare local variables?

        char getGlobalType(const std::string& name) const override;
        ///< 'l: long, 's': short, 'f': float, ' ': does not exist.

        std::pair<char, bool> getMemberType(const std::string& name, const ESM::RefId& id) const override;
        ///< Return type of member variable \a name in script \a id or in script of reference of
        /// \a id
        /// \return first: 'l: long, 's': short, 'f': float, ' ': does not exist.
        /// second: true: script of reference

        bool isId(const ESM::RefId& name) const override;
        ///< Does \a name match an ID, that can be referenced?

        void invalidateIds();

        void clear();
        ///< Remove all cached data.

        /// \return Were there any locals that needed clearing?
        bool clearLocals(const std::string& script);
    };
}

#endif
