
#include "scenesubview.hpp"

#include <sstream>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "../../model/doc/document.hpp"

#include "../../model/world/cellselection.hpp"

#include "../filter/filterbox.hpp"

#include "../render/pagedworldspacewidget.hpp"
#include "../render/unpagedworldspacewidget.hpp"

#include "tablebottombox.hpp"
#include "creator.hpp"
#include "scenetoolmode.hpp"

CSVWorld::SceneSubView::SceneSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id), mLayout(new QHBoxLayout), mDocument(document)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    layout->addWidget (mBottom =
        new TableBottomBox (NullCreatorFactory(), document.getData(), document.getUndoStack(), id,
        this), 0);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    mToolbar = new SceneToolbar (48+6, this);

    if (id.getId()=="sys::default")
    {
        CSVRender::PagedWorldspaceWidget *widget = new CSVRender::PagedWorldspaceWidget (this, document);

        mScene = widget;

        connect (widget, SIGNAL (cellSelectionChanged (const CSMWorld::CellSelection&)),
                 this, SLOT (cellSelectionChanged (const CSMWorld::CellSelection&)));

        connect (widget, SIGNAL(interiorCellsDropped (const std::vector<CSMWorld::UniversalId>&)),
                 this, SLOT(changeToUnpaged (const std::vector<CSMWorld::UniversalId>&)));
    }
    else
    {
        CSVRender::UnpagedWorldspaceWidget *widget = new CSVRender::UnpagedWorldspaceWidget (id.getId(), document, this);

        mScene = widget;

        connect (widget, SIGNAL(exteriorCellsDropped(const std::vector<CSMWorld::UniversalId>&)),
                 this, SLOT(changeToUnpaged(const std::vector<CSMWorld::UniversalId>&)));
    }

    SceneToolMode *navigationTool = mScene->makeNavigationSelector (mToolbar);
    mToolbar->addTool (navigationTool);

    SceneToolMode *lightingTool = mScene->makeLightingSelector (mToolbar);
    mToolbar->addTool (lightingTool);

    mLayout->addWidget (mToolbar, 0);

    mLayout->addWidget (mScene, 1);

    layout->insertLayout (0, mLayout, 1);

    CSVFilter::FilterBox *filterBox = new CSVFilter::FilterBox (document.getData(), this);

    layout->insertWidget (0, filterBox);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);

    mScene->selectDefaultNavigationMode();

    connect (mScene, SIGNAL (closeRequest()), this, SLOT (closeRequest()));
}

void CSVWorld::SceneSubView::setEditLock (bool locked)
{


}

void CSVWorld::SceneSubView::updateEditorSetting(const QString &settingName, const QString &settingValue)
{


}

void CSVWorld::SceneSubView::setStatusBar (bool show)
{
    mBottom->setStatusBar (show);
}

void CSVWorld::SceneSubView::useHint (const std::string& hint)
{
    mScene->useViewHint (hint);
}

void CSVWorld::SceneSubView::closeRequest()
{
    deleteLater();
}

void CSVWorld::SceneSubView::cellSelectionChanged (const CSMWorld::CellSelection& selection)
{
    int size = selection.getSize();

    std::ostringstream stream;
    stream << "Scene: " << getUniversalId().getId();

    if (size==0)
        stream << " (empty)";
    else if (size==1)
    {
        stream << " (" << *selection.begin() << ")";
    }
    else
    {
        stream << " (" << selection.getCentre() << " and " << size-1 << " more ";

        if (size>1)
            stream << "cells around it)";
        else
            stream << "cell around it)";
    }

    setWindowTitle (QString::fromUtf8 (stream.str().c_str()));
}

void CSVWorld::SceneSubView::changeToPaged (const std::vector< CSMWorld::UniversalId >& data)
{
    mLayout->removeWidget(mToolbar);
    mLayout->removeWidget(mScene);

    delete mScene;
    delete mToolbar;

    setUniversalId(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Cell, "sys::default"));
    mToolbar = new SceneToolbar (48+6, this);

    CSVRender::PagedWorldspaceWidget* widget = new CSVRender::PagedWorldspaceWidget (this, mDocument);

    mScene = widget;

    SceneToolMode* navigationTool = mScene->makeNavigationSelector (mToolbar);
    mToolbar->addTool (navigationTool);

    SceneToolMode* lightingTool = mScene->makeLightingSelector (mToolbar);
    mToolbar->addTool (lightingTool);

    connect (widget, SIGNAL (cellSelectionChanged (const CSMWorld::CellSelection&)),
             this, SLOT (cellSelectionChanged (const CSMWorld::CellSelection&)));

    connect (widget, SIGNAL (interiorCellsDropped (const std::vector<CSMWorld::UniversalId>&)),
             this, SLOT (changeToUnpaged (const std::vector<CSMWorld::UniversalId>&)));

    mLayout->addWidget (mToolbar, 0);
    mLayout->addWidget (mScene, 1);

    mScene->selectDefaultNavigationMode();

    connect (mScene, SIGNAL (closeRequest()), this, SLOT (closeRequest()));

    widget->handleDrop (data);
}

void CSVWorld::SceneSubView::changeToUnpaged (const std::vector< CSMWorld::UniversalId >& data)
{
    mLayout->removeWidget(mToolbar);
    mLayout->removeWidget(mScene);

    delete mScene;
    delete mToolbar;

    mToolbar = new SceneToolbar (48+6, this);
    CSVRender::UnpagedWorldspaceWidget* widget = new CSVRender::UnpagedWorldspaceWidget (data.begin()->getId(), mDocument, this);
    setUniversalId(*(data.begin()));

    mScene = widget;

    SceneToolMode* navigationTool = mScene->makeNavigationSelector (mToolbar);
    mToolbar->addTool (navigationTool);

    SceneToolMode* lightingTool = mScene->makeLightingSelector (mToolbar);
    mToolbar->addTool (lightingTool);

    connect (widget, SIGNAL (exteriorCellsDropped (const std::vector<CSMWorld::UniversalId>&)),
             this, SLOT (changeToPaged (const std::vector<CSMWorld::UniversalId>&)));

    mLayout->addWidget (mToolbar, 0);
    mLayout->addWidget (mScene, 1);

    mScene->selectDefaultNavigationMode();

    connect (mScene, SIGNAL (closeRequest()), this, SLOT (closeRequest()));
}
