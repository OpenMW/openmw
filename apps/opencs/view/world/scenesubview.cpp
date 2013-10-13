
#include "scenesubview.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "../../model/doc/document.hpp"

#include "../filter/filterbox.hpp"

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
// test
SceneToolMode *tool = new SceneToolMode (toolbar);
tool->addButton (":door.png", "a");
tool->addButton (":GMST.png", "b");
tool->addButton (":Info.png", "c");
toolbar->addTool (tool);
toolbar->addTool (new SceneToolMode (toolbar));
toolbar->addTool (new SceneToolMode (toolbar));
toolbar->addTool (new SceneToolMode (toolbar));
    layout2->addWidget (toolbar, 0);

    /// \todo replace with rendering widget
    QPalette palette2 (palette());
    palette2.setColor (QPalette::Background, Qt::white);
    QLabel *placeholder = new QLabel ("Here goes the 3D scene", this);
    placeholder->setAutoFillBackground (true);
    placeholder->setPalette (palette2);
    placeholder->setAlignment (Qt::AlignHCenter);

    layout2->addWidget (placeholder, 1);

    layout->insertLayout (0, layout2, 1);

    CSVFilter::FilterBox *filterBox = new CSVFilter::FilterBox (document.getData(), this);

    layout->insertWidget (0, filterBox);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);
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