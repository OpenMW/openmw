
#include "previewsubview.hpp"

#include <QHBoxLayout>

#include "../render/scenewidget.hpp"

#include "scenetoolbar.hpp"

#include "../render/previewwidget.hpp"

CSVWorld::PreviewSubView::PreviewSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id)
{
    QHBoxLayout *layout = new QHBoxLayout;

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    if (document.getData().getReferenceables().searchId (id.getId())==-1)
    {
        std::string referenceableId =
            document.getData().getReferences().getRecord (id.getId()).get().mRefID;

        setWindowTitle (("Preview: Reference to " + referenceableId).c_str());

        mScene =
            new CSVRender::PreviewWidget (document.getData(), referenceableId, id.getId(), this);
    }
    else
        mScene = new CSVRender::PreviewWidget (document.getData(), id.getId(), this);

    SceneToolbar *toolbar = new SceneToolbar (48, this);

    layout->addWidget (toolbar, 0);

    layout->addWidget (mScene, 1);

    QWidget *widget = new QWidget;

    widget->setLayout (layout);

    setWidget (widget);

    connect (mScene, SIGNAL (closeRequest()), this, SLOT (closeRequest()));
}

void CSVWorld::PreviewSubView::setEditLock (bool locked) {}

void CSVWorld::PreviewSubView::closeRequest()
{
    deleteLater();
}