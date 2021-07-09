#include "luabindings.hpp"

#include <components/esm/loadcell.hpp>

#include "../mwworld/cellstore.hpp"

namespace MWLua
{

    template <class CellT, class ObjectT>
    static void initCellBindings(const std::string& prefix, const Context& context)
    {
        sol::usertype<CellT> cellT = context.mLua->sol().new_usertype<CellT>(prefix + "Cell");

        cellT[sol::meta_function::equal_to] = [](const CellT& a, const CellT& b) { return a.mStore == b.mStore; };
        cellT[sol::meta_function::to_string] = [](const CellT& c)
        {
            const ESM::Cell* cell = c.mStore->getCell();
            std::stringstream res;
            if (cell->isExterior())
                res << "exterior(" << cell->getGridX() << ", " << cell->getGridY() << ")";
            else
                res << "interior(" << cell->mName << ")";
            return res.str();
        };

        cellT["name"] = sol::readonly_property([](const CellT& c) { return c.mStore->getCell()->mName; });
        cellT["region"] = sol::readonly_property([](const CellT& c) { return c.mStore->getCell()->mRegion; });
        cellT["gridX"] = sol::readonly_property([](const CellT& c) { return c.mStore->getCell()->getGridX(); });
        cellT["gridY"] = sol::readonly_property([](const CellT& c) { return c.mStore->getCell()->getGridY(); });
        cellT["isExterior"] = sol::readonly_property([](const CellT& c) { return c.mStore->isExterior(); });
        cellT["hasWater"] = sol::readonly_property([](const CellT& c) { return c.mStore->getCell()->hasWater(); });

        cellT["isInSameSpace"] = [](const CellT& c, const ObjectT& obj)
        {
            const MWWorld::Ptr& ptr = obj.ptr();
            if (!ptr.isInCell())
                return false;
            MWWorld::CellStore* cell = ptr.getCell();
            return cell == c.mStore || (cell->isExterior() && c.mStore->isExterior());
        };
        
        if constexpr (std::is_same_v<CellT, GCell>)
        {  // only for global scripts
            cellT["selectObjects"] = [context](const CellT& cell, const Queries::Query& query)
            {
                return GObjectList{selectObjectsFromCellStore(query, cell.mStore, context)};
            };
        }
    }

    void initCellBindingsForLocalScripts(const Context& context)
    {
        initCellBindings<LCell, LObject>("L", context);
    }

    void initCellBindingsForGlobalScripts(const Context& context)
    {
        initCellBindings<GCell, GObject>("G", context);
    }

}
