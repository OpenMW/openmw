#include "scriptcontext.hpp"

#include <algorithm>

#include <components/misc/stringops.hpp>

#include <components/compiler/quickfileparser.hpp>
#include <components/compiler/nullerrorhandler.hpp>
#include <components/compiler/scanner.hpp>

#include "data.hpp"

CSMWorld::ScriptContext::ScriptContext (const Data& data) : mData (data), mIdsUpdated (false) {}

bool CSMWorld::ScriptContext::canDeclareLocals() const
{
    return true;
}

char CSMWorld::ScriptContext::getGlobalType (const std::string& name) const
{
    int index = mData.getGlobals().searchId (name);

    if (index!=-1)
    {
        switch (mData.getGlobals().getRecord (index).get().mValue.getType())
        {
            case ESM::VT_Short: return 's';
            case ESM::VT_Long: return 'l';
            case ESM::VT_Float: return 'f';

            default: return ' ';
        }
    }

    return ' ';
}

std::pair<char, bool> CSMWorld::ScriptContext::getMemberType (const std::string& name,
    const std::string& id) const
{
    std::string id2 = Misc::StringUtils::lowerCase (id);

    int index = mData.getScripts().searchId (id2);
    bool reference = false;

    if (index==-1)
    {
        // ID is not a script ID. Search for a matching referenceable instead.
        index = mData.getReferenceables().searchId (id2);

        if (index!=-1)
        {
            // Referenceable found.
            int columnIndex = mData.getReferenceables().findColumnIndex (Columns::ColumnId_Script);

            id2 = Misc::StringUtils::lowerCase (mData.getReferenceables().
                getData (index, columnIndex).toString().toUtf8().constData());

            if (!id2.empty())
            {
                // Referenceable has a script -> use it.
                index = mData.getScripts().searchId (id2);
                reference = true;
            }
        }
    }

    if (index==-1)
        return std::make_pair (' ', false);

    std::map<std::string, Compiler::Locals>::iterator iter = mLocals.find (id2);

    if (iter==mLocals.end())
    {
        Compiler::Locals locals;

        Compiler::NullErrorHandler errorHandler;
        std::istringstream stream (mData.getScripts().getRecord (index).get().mScriptText);
        Compiler::QuickFileParser parser (errorHandler, *this, locals);
        Compiler::Scanner scanner (errorHandler, stream, getExtensions());
        scanner.scan (parser);

        iter = mLocals.insert (std::make_pair (id2, locals)).first;
    }

    return std::make_pair (iter->second.getType (Misc::StringUtils::lowerCase (name)), reference);
}

bool CSMWorld::ScriptContext::isId (const std::string& name) const
{
    if (!mIdsUpdated)
    {
        mIds = mData.getIds();

        std::for_each (mIds.begin(), mIds.end(), &Misc::StringUtils::lowerCaseInPlace);
        std::sort (mIds.begin(), mIds.end());

        mIdsUpdated = true;
    }

    return std::binary_search (mIds.begin(), mIds.end(), Misc::StringUtils::lowerCase (name));
}

bool CSMWorld::ScriptContext::isJournalId (const std::string& name) const
{
    return mData.getJournals().searchId (name)!=-1;
}

void CSMWorld::ScriptContext::invalidateIds()
{
    mIdsUpdated = false;
}

void CSMWorld::ScriptContext::clear()
{
    mIds.clear();
    mIdsUpdated = false;
    mLocals.clear();
}

bool CSMWorld::ScriptContext::clearLocals (const std::string& script)
{
    std::map<std::string, Compiler::Locals>::iterator iter =
        mLocals.find (Misc::StringUtils::lowerCase (script));

    if (iter!=mLocals.end())
    {
        mLocals.erase (iter);
        mIdsUpdated = false;
        return true;
    }

    return false;
}
