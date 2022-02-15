#include "types.hpp"

#include "../luabindings.hpp"

namespace MWLua
{

    static const MWWorld::Ptr& doorPtr(const Object& o) { return verifyType(ESM::REC_DOOR, o.ptr()); }

    void addDoorBindings(sol::table door, const Context& context)
    {
        door["isTeleport"] = [](const Object& o) { return doorPtr(o).getCellRef().getTeleport(); };
        door["destPosition"] = [](const Object& o) -> osg::Vec3f
        {
            return doorPtr(o).getCellRef().getDoorDest().asVec3();
        };
        door["destRotation"] = [](const Object& o) -> osg::Vec3f
        {
            return doorPtr(o).getCellRef().getDoorDest().asRotationVec3();
        };
        door["destCell"] = [worldView=context.mWorldView](sol::this_state lua, const Object& o) -> sol::object
        {
            const MWWorld::CellRef& cellRef = doorPtr(o).getCellRef();
            if (!cellRef.getTeleport())
                return sol::nil;
            MWWorld::CellStore* cell = worldView->findCell(cellRef.getDestCell(), cellRef.getDoorDest().asVec3());
            if (cell)
                return o.getCell(lua, cell);
            else
                return sol::nil;
        };
    }

}
