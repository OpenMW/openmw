
#include "previewsubview.hpp"

#include <QHBoxLayout>

#include "../render/previewwidget.hpp"

#include "scenetoolbar.hpp"
#include "scenetoolmode.hpp"

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
            new CSVRender::PreviewWidget (document.getData(), referenceableId, id.getId(), this);
    }
    else
        mScene = new CSVRender::PreviewWidget (document.getData(), id.getId(), this);

    SceneToolbar *toolbar = new SceneToolbar (48+6, this);

    SceneToolMode *lightingTool = mScene->makeLightingSelector (toolbar);
    toolbar->addTool (lightingTool);

    layout->addWidget (toolbar, 0);

    layout->addWidget (mScene, 1);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);

    connect (mScene, SIGNAL (closeRequest()), this, SLOT (closeRequest()));
    connect (mScene, SIGNAL (referenceableIdChanged (const std::string&)),
        this, SLOT (referenceableIdChanged (const std::string&)));
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