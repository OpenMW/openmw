
#include "scriptcontext.hpp"

#include <algorithm>

#include <components/misc/stringops.hpp>

#include "data.hpp"

CSMWorld::ScriptContext::ScriptContext (const Data& data) : mData (data), mIdsUpdated (false) {}

bool CSMWorld::ScriptContext::canDeclareLocals() const
{
    return false;
}

char CSMWorld::ScriptContext::getGlobalType (const std::string& name) const
{
    return ' ';
}

char CSMWorld::ScriptContext::getMemberType (const std::string& name, const std::string& id) const
{
    return ' ';
}

bool CSMWorld::ScriptContext::isId (const std::string& name) const
{
    if (!mIdsUpdated)
    {
        mIds = mData.getIds();

        std::for_each (mIds.begin(), mIds.end(), &Misc::StringUtils::lowerCase);

        mIdsUpdated = true;
    }

    return std::binary_search (mIds.begin(), mIds.end(), Misc::StringUtils::lowerCase (name));
}

void CSMWorld::ScriptContext::invalidateIds()
{
    mIdsUpdated = false;
}