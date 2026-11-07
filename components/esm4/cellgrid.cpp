#include "cellgrid.hpp"

#include "reader.hpp"

void ESM4::readPathingCell(Reader& reader, ESM::FormId& worldSpaceId, CellGrid& cellGrid)
{
    reader.getFormId(worldSpaceId);
    if (worldSpaceId.mIndex != 0)
    {
        Grid grid;
        reader.get(grid.y); // NOTE: reverse order
        reader.get(grid.x);
        cellGrid = grid;
    }
    else
    {
        ESM::FormId cellId;
        reader.getFormId(cellId);
        cellGrid = cellId;
    }
}
