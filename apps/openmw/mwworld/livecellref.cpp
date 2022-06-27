#include "livecellref.hpp"

#include <sstream>

#include <components/debug/debuglog.hpp>
#include <components/esm3/objectstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/luamanager.hpp"

#include "ptr.hpp"
#include "class.hpp"
#include "esmstore.hpp"

MWWorld::LiveCellRefBase::LiveCellRefBase(unsigned int type, const ESM::CellRef &cref)
  : mClass(&Class::get(type)), mRef(cref), mData(cref)
{
}

void MWWorld::LiveCellRefBase::loadImp (const ESM::ObjectState& state)
{
    mRef = state.mRef;
    mData = RefData (state, mData.isDeletedByContentFile());

    Ptr ptr (this);

    if (state.mHasLocals)
    {
        std::string scriptId = mClass->getScript (ptr);
        // Make sure we still have a script. It could have been coming from a content file that is no longer active.
        if (!scriptId.empty())
        {
            if (const ESM::Script* script = MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().search (scriptId))
            {
                try
                {
                    mData.setLocals (*script);
                    mData.getLocals().read (state.mLocals, scriptId);
                }
                catch (const std::exception& exception)
                {
                    Log(Debug::Error)
                        << "Error: failed to load state for local script " << scriptId
                        << " because an exception has been thrown: " << exception.what();
                }
            }
        }
    }

    mClass->readAdditionalState (ptr, state);

    if (!mRef.getSoul().empty() && !MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().search(mRef.getSoul()))
    {
        Log(Debug::Warning) << "Soul '" << mRef.getSoul() << "' not found, removing the soul from soul gem";
        mRef.setSoul(std::string());
    }

    MWBase::Environment::get().getLuaManager()->loadLocalScripts(ptr, state.mLuaScripts);
}

void MWWorld::LiveCellRefBase::saveImp (ESM::ObjectState& state) const
{
    mRef.writeState(state);

    ConstPtr ptr (this);

    mData.write (state, mClass->getScript (ptr));
    MWBase::Environment::get().getLuaManager()->saveLocalScripts(Ptr(const_cast<LiveCellRefBase*>(this)), state.mLuaScripts);

    mClass->writeAdditionalState (ptr, state);
}

bool MWWorld::LiveCellRefBase::checkStateImp (const ESM::ObjectState& state)
{
    return true;
}

unsigned int MWWorld::LiveCellRefBase::getType() const
{
    return mClass->getType();
}

namespace MWWorld
{
    std::string makeDynamicCastErrorMessage(const LiveCellRefBase* value, std::string_view recordType)
    {
        std::stringstream message;

        message << "Bad LiveCellRef cast to " << recordType << " from ";

        if (value != nullptr)
            message << value->getTypeDescription();
        else
            message << "an empty object";

        return message.str();
    }
}
