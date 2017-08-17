#include "terraintexturemode.hpp"

#include "pagedworldspacewidget.hpp"

CSVRender::TerrainTextureMode::TerrainTextureMode(WorldspaceWidget* worldspaceWidget, QWidget* parent)
    :EditMode {worldspaceWidget, QIcon {":placeholder"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent}
{
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    // TODO

    EditMode::activate(toolbar);
}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    // TODO

    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
    // TODO
}

void CSVRender::TerrainTextureMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
    // TODO
}

CSVRender::PagedWorldspaceWidget& CSVRender::TerrainTextureMode::getPagedWorldspaceWidget()
{
    return dynamic_cast<PagedWorldspaceWidget&>(getWorldspaceWidget());
}
