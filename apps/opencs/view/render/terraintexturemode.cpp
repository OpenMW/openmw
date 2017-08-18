#include "terraintexturemode.hpp"

#include "pagedworldspacewidget.hpp"

CSVRender::TerrainTextureMode::TerrainTextureMode(WorldspaceWidget* worldspaceWidget, QWidget* parent)
    :EditMode {worldspaceWidget, QIcon {":placeholder"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent}
{
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    getPagedWorldspaceWidget().activateTerrainSelection(TerrainSelectionType::Texture);

    EditMode::activate(toolbar);
}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    getPagedWorldspaceWidget().deactivateTerrainSelection(TerrainSelectionType::Texture);

    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
    getPagedWorldspaceWidget().selectTerrain(TerrainSelectionType::Texture, hit);
}

void CSVRender::TerrainTextureMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
    getPagedWorldspaceWidget().toggleSelectTerrain(TerrainSelectionType::Texture, hit);
}

CSVRender::PagedWorldspaceWidget& CSVRender::TerrainTextureMode::getPagedWorldspaceWidget()
{
    return dynamic_cast<PagedWorldspaceWidget&>(getWorldspaceWidget());
}
