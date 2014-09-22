
#include "previewsubview.hpp"

#include <QHBoxLayout>

#include "../render/previewwidget.hpp"

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolmode.hpp"

#include "../../model/settings/usersettings.hpp"

CSVWorld::PreviewSubView::PreviewSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id)
{
    QHBoxLayout *layout = new QHBoxLayout;

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    if (document.getData().getReferenceables().searchId (id.getId())==-1)
    {
        std::string referenceableId =
            document.getData().getReferences().getRecord (id.getId()).get().mRefID;

        referenceableIdChanged (referenceableId);

        mScene =
            new CSVRender::PreviewWidget (document.getData(), id.getId(), false, this);
    }
    else
        mScene = new CSVRender::PreviewWidget (document.getData(), id.getId(), true, this);

    CSVWidget::SceneToolbar *toolbar = new CSVWidget::SceneToolbar (48+6, this);

    CSVWidget::SceneToolMode *lightingTool = mScene->makeLightingSelector (toolbar);
    toolbar->addTool (lightingTool);

    layout->addWidget (toolbar, 0);

    layout->addWidget (mScene, 1);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    int minWidth = 325;
    if(CSMSettings::UserSettings::instance().hasSettingDefinitions("SubView/minimum width"))
        minWidth = CSMSettings::UserSettings::instance().settingValue("SubView/minimum width").toInt();
    else
        CSMSettings::UserSettings::instance().setDefinitions("SubView/minimum width", (QStringList() << "minWidth"));
    widget->setMinimumWidth(minWidth);

    setWidget (widget);

    connect (mScene, SIGNAL (closeRequest()), this, SLOT (closeRequest()));
    connect (mScene, SIGNAL (referenceableIdChanged (const std::string&)),
        this, SLOT (referenceableIdChanged (const std::string&)));
    connect (mScene, SIGNAL (focusToolbarRequest()), toolbar, SLOT (setFocus()));
    connect (toolbar, SIGNAL (focusSceneRequest()), mScene, SLOT (setFocus()));
}

void CSVWorld::PreviewSubView::setEditLock (bool locked) {}

void CSVWorld::PreviewSubView::closeRequest()
{
    deleteLater();
}

void CSVWorld::PreviewSubView::referenceableIdChanged (const std::string& id)
{
    if (id.empty())
        setWindowTitle ("Preview: Reference to <nothing>");
    else
        setWindowTitle (("Preview: Reference to " + id).c_str());
}
