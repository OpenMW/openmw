#include "scenesubview.hpp"

#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>

#include <QHBoxLayout>
#include <QMargins>
#include <QVBoxLayout>
#include <QWidget>

#include <apps/opencs/model/world/cellcoordinates.hpp>
#include <apps/opencs/view/doc/subview.hpp>
#include <apps/opencs/view/render/worldspacewidget.hpp>

#include "../../model/doc/document.hpp"

#include "../../model/world/cellselection.hpp"

#include "../filter/filterbox.hpp"

#include "../render/pagedworldspacewidget.hpp"
#include "../render/unpagedworldspacewidget.hpp"

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetoolrun.hpp"
#include "../widget/scenetooltoggle2.hpp"

#include "creator.hpp"
#include "tablebottombox.hpp"

CSVWorld::SceneSubView::SceneSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    : SubView(id)
    , mScene(nullptr)
    , mLayout(new QHBoxLayout)
    , mDocument(document)
    , mToolbar(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout;

    layout->addWidget(mBottom = new TableBottomBox(NullCreatorFactory(), document, id, this), 0);

    mLayout->setContentsMargins(QMargins(0, 0, 0, 0));

    CSVRender::WorldspaceWidget* worldspaceWidget = nullptr;
    WidgetType whatWidget;

    if (Misc::StringUtils::ciEqual(id.getId(), ESM::Cell::sDefaultWorldspaceId.getValue()))
    {
        whatWidget = widget_Paged;

        CSVRender::PagedWorldspaceWidget* newWidget = new CSVRender::PagedWorldspaceWidget(this, document);

        worldspaceWidget = newWidget;

        makeConnections(newWidget);
    }
    else
    {
        whatWidget = widget_Unpaged;

        CSVRender::UnpagedWorldspaceWidget* newWidget
            = new CSVRender::UnpagedWorldspaceWidget(id.getId(), document, this);

        worldspaceWidget = newWidget;

        makeConnections(newWidget);
    }

    replaceToolbarAndWorldspace(worldspaceWidget, makeToolbar(worldspaceWidget, whatWidget));

    layout->insertLayout(0, mLayout, 1);

    CSVFilter::FilterBox* filterBox = new CSVFilter::FilterBox(document.getData(), this);

    layout->insertWidget(0, filterBox);

    QWidget* widget = new QWidget;

    widget->setLayout(layout);

    setWidget(widget);
}

void CSVWorld::SceneSubView::makeConnections(CSVRender::UnpagedWorldspaceWidget* widget)
{
    connect(widget, &CSVRender::UnpagedWorldspaceWidget::closeRequest, this, qOverload<>(&SceneSubView::closeRequest));

    connect(widget, &CSVRender::UnpagedWorldspaceWidget::dataDropped, this, &SceneSubView::handleDrop);

    connect(widget, &CSVRender::UnpagedWorldspaceWidget::cellChanged, this,
        qOverload<const CSMWorld::UniversalId&>(&SceneSubView::cellSelectionChanged));

    connect(widget, &CSVRender::UnpagedWorldspaceWidget::requestFocus, this, &SceneSubView::requestFocus);
}

void CSVWorld::SceneSubView::makeConnections(CSVRender::PagedWorldspaceWidget* widget)
{
    connect(widget, &CSVRender::PagedWorldspaceWidget::closeRequest, this, qOverload<>(&SceneSubView::closeRequest));

    connect(widget, &CSVRender::PagedWorldspaceWidget::dataDropped, this, &SceneSubView::handleDrop);

    connect(widget, &CSVRender::PagedWorldspaceWidget::cellSelectionChanged, this,
        qOverload<const CSMWorld::CellSelection&>(&SceneSubView::cellSelectionChanged));

    connect(widget, &CSVRender::PagedWorldspaceWidget::requestFocus, this, &SceneSubView::requestFocus);
}

CSVWidget::SceneToolbar* CSVWorld::SceneSubView::makeToolbar(CSVRender::WorldspaceWidget* widget, WidgetType type)
{
    CSVWidget::SceneToolbar* toolbar = new CSVWidget::SceneToolbar(48 + 6, this);

    CSVWidget::SceneToolMode* navigationTool = widget->makeNavigationSelector(toolbar);
    toolbar->addTool(navigationTool);

    CSVWidget::SceneToolMode* lightingTool = widget->makeLightingSelector(toolbar);
    toolbar->addTool(lightingTool);

    CSVWidget::SceneToolToggle2* sceneVisibilityTool = widget->makeSceneVisibilitySelector(toolbar);
    toolbar->addTool(sceneVisibilityTool);

    if (type == widget_Paged)
    {
        CSVWidget::SceneToolToggle2* controlVisibilityTool
            = dynamic_cast<CSVRender::PagedWorldspaceWidget&>(*widget).makeControlVisibilitySelector(toolbar);

        toolbar->addTool(controlVisibilityTool);
    }

    CSVWidget::SceneToolRun* runTool = widget->makeRunTool(toolbar);
    toolbar->addTool(runTool);

    toolbar->addTool(widget->makeEditModeSelector(toolbar), runTool);

    return toolbar;
}

void CSVWorld::SceneSubView::setEditLock(bool locked)
{
    mScene->setEditLock(locked);
}

void CSVWorld::SceneSubView::setStatusBar(bool show)
{
    mBottom->setStatusBar(show);
}

void CSVWorld::SceneSubView::useHint(const std::string& hint)
{
    mScene->useViewHint(hint);
}

std::string CSVWorld::SceneSubView::getTitle() const
{
    return mTitle;
}

void CSVWorld::SceneSubView::cellSelectionChanged(const CSMWorld::UniversalId& id)
{
    setUniversalId(id);

    mTitle = "Scene: " + getUniversalId().getId();
    setWindowTitle(QString::fromUtf8(mTitle.c_str()));
    emit updateTitle();
}

void CSVWorld::SceneSubView::cellSelectionChanged(const CSMWorld::CellSelection& selection)
{
    setUniversalId(
        CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Scene, ESM::Cell::sDefaultWorldspaceId.getValue()));
    int size = selection.getSize();

    std::ostringstream stream;
    stream << "Scene: " << getUniversalId().getId();

    if (size == 0)
        stream << " (empty)";
    else if (size == 1)
    {
        stream << " (" << *selection.begin() << ")";
    }
    else
    {
        stream << " (" << selection.getCentre() << " and " << size - 1 << " more ";

        if (size > 1)
            stream << "cells around it)";
        else
            stream << "cell around it)";
    }

    mTitle = stream.str();
    setWindowTitle(QString::fromUtf8(mTitle.c_str()));
    emit updateTitle();
}

void CSVWorld::SceneSubView::handleDrop(const std::vector<CSMWorld::UniversalId>& universalIdData)
{
    CSVRender::PagedWorldspaceWidget* pagedNewWidget = nullptr;
    CSVRender::UnpagedWorldspaceWidget* unPagedNewWidget = nullptr;
    CSVWidget::SceneToolbar* toolbar = nullptr;

    CSVRender::WorldspaceWidget::DropType type = CSVRender::WorldspaceWidget::getDropType(universalIdData);

    switch (mScene->getDropRequirements(type))
    {
        case CSVRender::WorldspaceWidget::canHandle:
            mScene->handleDrop(universalIdData, type);
            break;

        case CSVRender::WorldspaceWidget::needPaged:
            pagedNewWidget = new CSVRender::PagedWorldspaceWidget(this, mDocument);
            toolbar = makeToolbar(pagedNewWidget, widget_Paged);
            makeConnections(pagedNewWidget);
            replaceToolbarAndWorldspace(pagedNewWidget, toolbar);
            mScene->handleDrop(universalIdData, type);
            break;

        case CSVRender::WorldspaceWidget::needUnpaged:
            unPagedNewWidget
                = new CSVRender::UnpagedWorldspaceWidget(universalIdData.begin()->getId(), mDocument, this);
            toolbar = makeToolbar(unPagedNewWidget, widget_Unpaged);
            makeConnections(unPagedNewWidget);
            replaceToolbarAndWorldspace(unPagedNewWidget, toolbar);
            cellSelectionChanged(*(universalIdData.begin()));
            break;

        case CSVRender::WorldspaceWidget::ignored:
            return;
    }
}

void CSVWorld::SceneSubView::replaceToolbarAndWorldspace(
    CSVRender::WorldspaceWidget* widget, CSVWidget::SceneToolbar* toolbar)
{
    assert(mLayout);

    if (mScene)
    {
        mLayout->removeWidget(mScene);
        mScene->deleteLater();
    }

    if (mToolbar)
    {
        mLayout->removeWidget(mToolbar);
        mToolbar->deleteLater();
    }

    mScene = widget;
    mToolbar = toolbar;

    connect(mScene, &CSVRender::WorldspaceWidget::focusToolbarRequest, mToolbar,
        qOverload<>(&CSVWidget::SceneToolbar::setFocus));
    connect(mToolbar, &CSVWidget::SceneToolbar::focusSceneRequest, mScene,
        qOverload<>(&CSVRender::WorldspaceWidget::setFocus));

    mLayout->addWidget(mToolbar, 0);
    mLayout->addWidget(mScene, 1);

    mScene->selectDefaultNavigationMode();
    setFocusProxy(mScene);
}
