#include "previewsubview.hpp"

#include <QHBoxLayout>

#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/ref.hpp>
#include <apps/opencs/model/world/refcollection.hpp>
#include <apps/opencs/model/world/refidcollection.hpp>
#include <apps/opencs/view/doc/subview.hpp>

#include "../render/previewwidget.hpp"

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolmode.hpp"

#include "../../model/doc/document.hpp"

CSVWorld::PreviewSubView::PreviewSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    : SubView(id)
    , mTitle(id.toString().c_str())
{
    QHBoxLayout* layout = new QHBoxLayout;

    if (document.getData().getReferenceables().searchId(ESM::RefId::stringRefId(id.getId())) == -1)
    {
        std::string referenceableId = document.getData()
                                          .getReferences()
                                          .getRecord(ESM::RefId::stringRefId(id.getId()))
                                          .get()
                                          .mRefID.getRefIdString();

        referenceableIdChanged(referenceableId);

        mScene = new CSVRender::PreviewWidget(document.getData(), id.getId(), false, this);
    }
    else
        mScene = new CSVRender::PreviewWidget(document.getData(), id.getId(), true, this);

    mScene->setExterior(true);

    CSVWidget::SceneToolbar* toolbar = new CSVWidget::SceneToolbar(48 + 6, this);

    CSVWidget::SceneToolMode* lightingTool = mScene->makeLightingSelector(toolbar);
    toolbar->addTool(lightingTool);

    layout->addWidget(toolbar, 0);

    layout->addWidget(mScene, 1);

    QWidget* widget = new QWidget;

    widget->setLayout(layout);

    setWidget(widget);

    connect(mScene, &CSVRender::PreviewWidget::closeRequest, this, qOverload<>(&PreviewSubView::closeRequest));
    connect(mScene, &CSVRender::PreviewWidget::referenceableIdChanged, this, &PreviewSubView::referenceableIdChanged);
    connect(mScene, &CSVRender::PreviewWidget::focusToolbarRequest, toolbar,
        qOverload<>(&CSVWidget::SceneToolbar::setFocus));
    connect(
        toolbar, &CSVWidget::SceneToolbar::focusSceneRequest, mScene, qOverload<>(&CSVRender::PreviewWidget::setFocus));
}

void CSVWorld::PreviewSubView::setEditLock(bool locked) {}

std::string CSVWorld::PreviewSubView::getTitle() const
{
    return mTitle;
}

void CSVWorld::PreviewSubView::referenceableIdChanged(const std::string& id)
{
    if (id.empty())
        mTitle = "Preview: Reference to <nothing>";
    else
        mTitle = "Preview: Reference to " + id;

    setWindowTitle(QString::fromUtf8(mTitle.c_str()));

    emit updateTitle();
}
