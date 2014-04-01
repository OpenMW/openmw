
#include "scenesubview.hpp"

#include <sstream>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "../../model/doc/document.hpp"

#include "../filter/filterbox.hpp"

#include "../render/pagedworldspacewidget.hpp"
#include "../render/unpagedworldspacewidget.hpp"

#include "tablebottombox.hpp"
#include "creator.hpp"
#include "scenetoolbar.hpp"
#include "scenetoolmode.hpp"

CSVWorld::SceneSubView::SceneSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    layout->addWidget (mBottom =
        new TableBottomBox (NullCreatorFactory(), document.getData(), document.getUndoStack(), id,
        this), 0);

    QHBoxLayout *layout2 = new QHBoxLayout;

    layout2->setContentsMargins (QMargins (0, 0, 0, 0));

    SceneToolbar *toolbar = new SceneToolbar (48, this);

    if (id.getId()=="sys::default")
    {
        CSVRender::PagedWorldspaceWidget *widget = new CSVRender::PagedWorldspaceWidget (this);
        mScene = widget;
        connect (widget,
            SIGNAL (cellIndexChanged (const std::pair<int, int>&, const std::pair<int, int>&)),
            this,
            SLOT (cellIndexChanged (const std::pair<int, int>&, const std::pair<int, int>&)));
    }
    else
        mScene = new CSVRender::UnpagedWorldspaceWidget (id.getId(), document, this);

    SceneToolMode *navigationTool = mScene->makeNavigationSelector (toolbar);
    toolbar->addTool (navigationTool);

    SceneToolMode *lightingTool = mScene->makeLightingSelector (toolbar);
    toolbar->addTool (lightingTool);

    layout2->addWidget (toolbar, 0);

    layout2->addWidget (mScene, 1);

    layout->insertLayout (0, layout2, 1);

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

void CSVWorld::SceneSubView::cellIndexChanged (const std::pair<int, int>& min,
    const std::pair<int, int>& max)
{
    std::ostringstream stream;
    stream << "Scene: " << getUniversalId().getId() << " (" << min.first << ", " << min.second;

    if (min!=max)
        stream << " to " << max.first << ", " << max.second;

    stream << ")";

    setWindowTitle (QString::fromUtf8 (stream.str().c_str()));
}