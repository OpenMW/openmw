#include "commands.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/loadland.hpp>

#include "editmode.hpp"
#include "terrainselection.hpp"
#include "terrainshapemode.hpp"
#include "terraintexturemode.hpp"
#include "worldspacewidget.hpp"

CSVRender::DrawTerrainSelectionCommand::DrawTerrainSelectionCommand(WorldspaceWidget* worldspaceWidget, QUndoCommand* parent)
    : mWorldspaceWidget(worldspaceWidget)
{ }

void CSVRender::DrawTerrainSelectionCommand::redo()
{
    if (CSVRender::WorldspaceWidget* worldspaceWidget = dynamic_cast<CSVRender::WorldspaceWidget *> (mWorldspaceWidget))
    {
        if (CSVRender::TerrainShapeMode* terrainMode = dynamic_cast<CSVRender::TerrainShapeMode *> (worldspaceWidget->getEditMode()) )
            {
                terrainMode->getTerrainSelection()->update();
                return;
            }
    }
    Log(Debug::Warning) << "Error in redoing terrain selection";
}

void CSVRender::DrawTerrainSelectionCommand::undo()
{
    if (CSVRender::WorldspaceWidget* worldspaceWidget = dynamic_cast<CSVRender::WorldspaceWidget *> (mWorldspaceWidget))
    {
        if (CSVRender::TerrainShapeMode* terrainMode = dynamic_cast<CSVRender::TerrainShapeMode *> (worldspaceWidget->getEditMode()) )
            {
                terrainMode->getTerrainSelection()->update();
                return;
            }
    }
    Log(Debug::Warning) << "Error in undoing terrain selection";
}
