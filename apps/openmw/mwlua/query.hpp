#ifndef MWLUA_QUERY_H
#define MWLUA_QUERY_H

#include <string>

#include <components/queries/query.hpp>

#include "context.hpp"
#include "object.hpp"

namespace MWLua
{

    struct ObjectQueryTypes
    {
        static constexpr std::string_view ACTIVATORS = "activators";
        static constexpr std::string_view ACTORS = "actors";
        static constexpr std::string_view CONTAINERS = "containers";
        static constexpr std::string_view DOORS = "doors";
        static constexpr std::string_view ITEMS = "items";

        static constexpr std::string_view types[] = {ACTIVATORS, ACTORS, CONTAINERS, DOORS, ITEMS};
    };

    struct QueryFieldGroup
    {
        std::string mName;
        std::vector<const Queries::Field*> mFields;
    };
    const std::vector<QueryFieldGroup>& getBasicQueryFieldGroups();

    // TODO: Implement custom fields. QueryFieldGroup registerCustomFields(...);

    ObjectIdList selectObjectsFromList(const Queries::Query& query, const ObjectIdList& list, const Context&);
    ObjectIdList selectObjectsFromCellStore(const Queries::Query& query, MWWorld::CellStore* store, const Context&);

}

#endif // MWLUA_QUERY_H
