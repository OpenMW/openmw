
#include "scenesubview.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "../../model/doc/document.hpp"

#include "../filter/filterbox.hpp"

#include "../render/scenewidget.hpp"

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

    // navigation mode
    SceneToolMode *tool = new SceneToolMode (toolbar);
    tool->addButton (":door.png", "1st"); /// \todo replace icons
    tool->addButton (":GMST.png", "free");
    tool->addButton (":Info.png", "orbit");
    toolbar->addTool (tool);
    connect (tool, SIGNAL (modeChanged (const std::string&)),
        this, SLOT (selectNavigationMode (const std::string&)));

    layout2->addWidget (toolbar, 0);

    mScene = new CSVRender::SceneWidget(this);

    layout2->addWidget (mScene, 1);

    layout->insertLayout (0, layout2, 1);

    CSVFilter::FilterBox *filterBox = new CSVFilter::FilterBox (document.getData(), this);

    layout->insertWidget (0, filterBox);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);

    mScene->setNavigation (&m1st);
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

void CSVWorld::SceneSubView::selectNavigationMode (const std::string& mode)
{
    if (mode=="1st")
        mScene->setNavigation (&m1st);
    else if (mode=="free")
        mScene->setNavigation (&mFree);
    else if (mode=="orbit")
        mScene->setNavigation (&mOrbit);
}
