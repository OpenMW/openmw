#include "commands.hpp"

#include <components/esm/loadland.hpp>

#include "terrainselection.hpp"

CSVRender::DrawTerrainSelectionCommand::DrawTerrainSelectionCommand(TerrainSelection& terrainSelection, QUndoCommand* parent)
    : mTerrainSelection(terrainSelection)
{ }

void CSVRender::DrawTerrainSelectionCommand::redo()
{
    mTerrainSelection.update();
}

void CSVRender::DrawTerrainSelectionCommand::undo()
{
    mTerrainSelection.update();
}
