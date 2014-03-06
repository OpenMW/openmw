
#include "scenesubview.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "../../model/doc/document.hpp"

#include "../filter/filterbox.hpp"

#include "../render/worldspacewidget.hpp"

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

    mScene = new CSVRender::WorldspaceWidget (this);

    SceneToolMode *tool = mScene->makeNavigationSelector (toolbar);
    toolbar->addTool (tool);

    layout2->addWidget (toolbar, 0);

    layout2->addWidget (mScene, 1);

    layout->insertLayout (0, layout2, 1);

    CSVFilter::FilterBox *filterBox = new CSVFilter::FilterBox (document.getData(), this);

    layout->insertWidget (0, filterBox);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);

    mScene->selectDefaultNavigationMode();
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
