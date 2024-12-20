#include "actoridconverter.hpp"

namespace ESM
{
    void ActorIdConverter::apply()
    {
        for (auto& [refNum, actorId] : mToConvert)
        {
            auto it = mMappings.find(actorId);
            if (it == mMappings.end())
                refNum = {};
            else
                refNum = it->second;
        }
    }

    void ActorIdConverter::convert(ESM::RefNum& refNum, int actorId)
    {
        if (actorId == -1)
        {
            refNum = {};
            return;
        }
        auto it = mMappings.find(actorId);
        if (it == mMappings.end())
        {
            mToConvert.emplace_back(refNum, actorId);
            return;
        }
        refNum = it->second;
    }
}
