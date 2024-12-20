#ifndef OPENMW_COMPONENTS_ESM3_ACTORIDCONVERTER_H
#define OPENMW_COMPONENTS_ESM3_ACTORIDCONVERTER_H

#include <functional>
#include <map>
#include <vector>

#include "refnum.hpp"

namespace ESM
{
    class ActorIdConverter
    {
        std::vector<std::function<void()>> mConverters;

    public:
        std::map<int, ESM::RefNum> mMappings;

        void apply();

        ESM::RefNum convert(int actorId) const;

        bool convert(ESM::RefNum& refNum, int actorId) const;

        void addConverter(std::function<void()>&& converter) { mConverters.emplace_back(converter); }
    };
}

#endif
