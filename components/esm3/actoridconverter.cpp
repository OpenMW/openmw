#include "actoridconverter.hpp"

namespace ESM
{
    void ActorIdConverter::apply()
    {
        for (auto& converter : mConverters)
            converter();
    }

    ESM::RefNum ActorIdConverter::convert(int actorId) const
    {
        auto it = mMappings.find(actorId);
        if (it == mMappings.end())
            return {};
        return it->second;
    }

    bool ActorIdConverter::convert(ESM::RefNum& refNum, int actorId) const
    {
        if (actorId == -1)
        {
            refNum = {};
            return true;
        }
        auto it = mMappings.find(actorId);
        if (it == mMappings.end())
            return false;
        refNum = it->second;
        return true;
    }
}
