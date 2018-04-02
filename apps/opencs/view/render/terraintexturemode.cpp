#include "terraintexturemode.hpp"
#include "editmode.hpp"

#include <QIcon>

#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetoolbar.hpp"
#include "pagedworldspacewidget.hpp"

CSVRender::TerrainTextureMode::TerrainTextureMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-texture"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent), mSubMode (0)
{
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{

    if (!mSubMode)
    {
        mSubMode = new CSVWidget::SceneToolMode (toolbar, "TERRAINTEXTURESUBMODE");
        mSubMode->addButton (":scenetoolbar/brush-point", "point-brush",
            "Single point brush (no size)");
        mSubMode->addButton (":scenetoolbar/brush-square", "square-brush",
            "Square brush (size is length of an edge)");
        mSubMode->addButton (":scenetoolbar/brush-circle", "circle-brush",
            "Circle brush (size is diameter)");
        mSubMode->addButton (":scenetoolbar/brush-custom", "custom-brush",
            "Custom selection brush");

        mSubMode->setButton (mSubModeId);

        connect (mSubMode, SIGNAL (modeChanged (const std::string&)),
            this, SLOT (subModeChanged (const std::string&)));
    }

    toolbar->addTool (mSubMode);
    EditMode::activate(toolbar);
}


void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{

    if (mSubMode)
    {
        toolbar->removeTool (mSubMode);
        delete mSubMode;
        mSubMode = 0;
    }

    EditMode::deactivate(toolbar);
}


void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
}


void CSVRender::TerrainTextureMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
}

int CSVRender::TerrainTextureMode::getSubModeFromId (const std::string& id) const
{
    return id=="point-brush" ? 0 :
    id=="square-brush" ? 1 :
    id=="circle-brush" ? 2 :
    (id=="custom-brush" ? 3 : 4);
}

void CSVRender::TerrainTextureMode::subModeChanged (const std::string& id)
{
    mSubModeId = id;
    getWorldspaceWidget().abortDrag();
    getWorldspaceWidget().setSubMode (getSubModeFromId (id), Mask_Reference);
}
