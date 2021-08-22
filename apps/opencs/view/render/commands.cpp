#include "commands.hpp"

#include <QPointer>

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
    if (mWorldspaceWidget)
    {
        if (CSVRender::TerrainShapeMode* terrainMode = dynamic_cast<CSVRender::TerrainShapeMode *> (mWorldspaceWidget->getEditMode()) )
        {
            terrainMode->getTerrainSelection()->update();
            return;
        }
        else
        {
            Log(Debug::Verbose) << "Can't update terrain selection in current EditMode";
            return;
        }
    }
    else
        Log(Debug::Verbose) << "Can't update terrain selection, no WorldspaceWidget found!";
}

void CSVRender::DrawTerrainSelectionCommand::undo()
{
    if (mWorldspaceWidget)
    {
        if (CSVRender::TerrainShapeMode* terrainMode = dynamic_cast<CSVRender::TerrainShapeMode *> (mWorldspaceWidget->getEditMode()) )
        {
            terrainMode->getTerrainSelection()->update();
            return;
        }
        else
        {
            Log(Debug::Verbose) << "Can't undo terrain selection in current EditMode";
            return;
        }
    }
    else
        Log(Debug::Verbose) << "Can't undo terrain selection, no WorldspaceWidget found!";
}
