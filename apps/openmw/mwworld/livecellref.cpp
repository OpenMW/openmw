#include "livecellref.hpp"

#include <sstream>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/objectstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"

#include "class.hpp"
#include "esmstore.hpp"
#include "ptr.hpp"
#include "worldmodel.hpp"

namespace MWWorld
{
    LiveCellRefBase::LiveCellRefBase(unsigned int type, const ESM::CellRef& cref)
        : mClass(&Class::get(type))
        , mRef(cref)
        , mData(cref)
    {
    }

    LiveCellRefBase::LiveCellRefBase(unsigned int type, const ESM4::Reference& cref)
        : mClass(&Class::get(type))
        , mRef(cref)
        , mData(cref)
    {
    }

    LiveCellRefBase::LiveCellRefBase(unsigned int type, const ESM4::ActorCharacter& cref)
        : mClass(&Class::get(type))
        , mRef(cref)
        , mData(cref)
    {
    }

    LiveCellRefBase::~LiveCellRefBase()
    {
        MWBase::Environment::get().getWorldModel()->deregisterLiveCellRef(*this);
    }

    void LiveCellRefBase::loadImp(const ESM::ObjectState& state)
    {
        mRef = CellRef(state.mRef);
        mData = RefData(state, mData.isDeletedByContentFile());

        Ptr ptr(this);

        if (state.mHasLocals)
        {
            const ESM::RefId& scriptId = mClass->getScript(ptr);
            // Make sure we still have a script. It could have been coming from a content file that is no longer active.
            if (!scriptId.empty())
            {
                if (const ESM::Script* script
                    = MWBase::Environment::get().getESMStore()->get<ESM::Script>().search(scriptId))
                {
                    try
                    {
                        mData.setLocals(*script);
                        mData.getLocals().read(state.mLocals, scriptId);
                    }
                    catch (const std::exception& exception)
                    {
                        Log(Debug::Error) << "Error: failed to load state for local script " << scriptId
                                          << " because an exception has been thrown: " << exception.what();
                    }
                }
            }
        }

        mClass->readAdditionalState(ptr, state);

        if (!mRef.getSoul().empty()
            && !MWBase::Environment::get().getESMStore()->get<ESM::Creature>().search(mRef.getSoul()))
        {
            Log(Debug::Warning) << "Soul '" << mRef.getSoul() << "' not found, removing the soul from soul gem";
            mRef.setSoul(ESM::RefId());
        }

        MWBase::Environment::get().getLuaManager()->loadLocalScripts(ptr, state.mLuaScripts);
    }

    void LiveCellRefBase::saveImp(ESM::ObjectState& state) const
    {
        mRef.writeState(state);

        ConstPtr ptr(this);

        mData.write(state, mClass->getScript(ptr));
        MWBase::Environment::get().getLuaManager()->saveLocalScripts(
            Ptr(const_cast<LiveCellRefBase*>(this)), state.mLuaScripts);

        mClass->writeAdditionalState(ptr, state);
    }

    bool LiveCellRefBase::checkStateImp(const ESM::ObjectState& state)
    {
        return true;
    }

    unsigned int LiveCellRefBase::getType() const
    {
        return mClass->getType();
    }

    bool LiveCellRefBase::isDeleted() const
    {
        return mData.isDeletedByContentFile() || mRef.getCount(false) == 0;
    }

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
