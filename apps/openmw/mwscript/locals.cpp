#include "locals.hpp"

#include <components/esm/loadscpt.hpp>
#include <components/esm/variant.hpp>
#include <components/esm/locals.hpp>

#include <components/compiler/locals.hpp>
#include <components/compiler/exception.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"

namespace MWScript
{
    void Locals::configure (const ESM::Script& script)
    {
        const Compiler::Locals& locals =
            MWBase::Environment::get().getScriptManager()->getLocals (script.mId);

        mShorts.clear();
        mShorts.resize (locals.get ('s').size(), 0);
        mLongs.clear();
        mLongs.resize (locals.get ('l').size(), 0);
        mFloats.clear();
        mFloats.resize (locals.get ('f').size(), 0);
    }

    bool Locals::isEmpty() const
    {
        return (mShorts.empty() && mLongs.empty() && mFloats.empty());
    }

    int Locals::getIntVar(const std::string &script, const std::string &var)
    {
        const Compiler::Locals& locals = MWBase::Environment::get().getScriptManager()->getLocals(script);
        int index = locals.getIndex(var);
        char type = locals.getType(var);
        if(index != -1)
        {
            switch(type)
            {
                case 's':
                    return mShorts.at (index);

                case 'l':
                    return mLongs.at (index);

                case 'f':
                    return mFloats.at (index);
                default:
                    return 0;
            }
        }
        return 0;
    }

    bool Locals::setVarByInt(const std::string& script, const std::string& var, int val)
    {
        const Compiler::Locals& locals = MWBase::Environment::get().getScriptManager()->getLocals(script);
        int index = locals.getIndex(var);
        char type = locals.getType(var);
        if(index != -1)
        {
            switch(type)
            {
                case 's':
                    mShorts.at (index) = val; break;

                case 'l':
                    mLongs.at (index) = val; break;

                case 'f':
                    mFloats.at (index) = val; break;
            }
            return true;
        }
        return false;
    }

    void Locals::write (ESM::Locals& locals, const std::string& script) const
    {
        try
        {
            const Compiler::Locals& declarations =
                MWBase::Environment::get().getScriptManager()->getLocals(script);

            for (int i=0; i<3; ++i)
            {
                char type = 0;

                switch (i)
                {
                    case 0: type = 's'; break;
                    case 1: type = 'l'; break;
                    case 2: type = 'f'; break;
                }

                const std::vector<std::string>& names = declarations.get (type);

                for (int i2=0; i2<static_cast<int> (names.size()); ++i2)
                {
                    ESM::Variant value;

                    switch (i)
                    {
                        case 0: value.setType (ESM::VT_Int); value.setInteger (mShorts.at (i2)); break;
                        case 1: value.setType (ESM::VT_Int); value.setInteger (mLongs.at (i2)); break;
                        case 2: value.setType (ESM::VT_Float); value.setFloat (mFloats.at (i2)); break;
                    }

                    locals.mVariables.push_back (std::make_pair (names[i2], value));
                }
            }
        }
        catch (const Compiler::SourceException&)
        {
        }
    }

    void Locals::read (const ESM::Locals& locals, const std::string& script)
    {
        try
        {
            const Compiler::Locals& declarations =
                MWBase::Environment::get().getScriptManager()->getLocals(script);

            for (std::vector<std::pair<std::string, ESM::Variant> >::const_iterator iter
                = locals.mVariables.begin(); iter!=locals.mVariables.end(); ++iter)
            {
                char type =  declarations.getType (iter->first);
                char index = declarations.getIndex (iter->first);

                try
                {
                    switch (type)
                    {
                        case 's': mShorts.at (index) = iter->second.getInteger(); break;
                        case 'l': mLongs.at (index) = iter->second.getInteger(); break;
                        case 'f': mFloats.at (index) = iter->second.getFloat(); break;

                        // silently ignore locals that don't exist anymore
                    }
                }
                catch (...)
                {
                    // ignore type changes
                    /// \todo write to log
                }
            }
        }
        catch (const Compiler::SourceException&)
        {
        }
    }
}
