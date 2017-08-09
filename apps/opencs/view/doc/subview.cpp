#include "subview.hpp"

#include "view.hpp"

#include <QShortcut>
#include <QEvent>
#include <QKeyEvent>

bool CSVDoc::SubView::event (QEvent *event)
{
    if (event->type()==QEvent::ShortcutOverride)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *> (event);

        if (keyEvent->key()==Qt::Key_W && keyEvent->modifiers()==(Qt::ShiftModifier | Qt::ControlModifier))
            emit closeRequest();
            return true;
    }

    return QDockWidget::event (event);
}

CSVDoc::SubView::SubView (const CSMWorld::UniversalId& id)
 : mUniversalId (id)
{
    /// \todo  add a button to the title bar that clones this sub view

    setWindowTitle (QString::fromUtf8 (mUniversalId.toString().c_str()));
    setAttribute(Qt::WA_DeleteOnClose);
}

CSMWorld::UniversalId CSVDoc::SubView::getUniversalId() const
{
    return mUniversalId;
}

void CSVDoc::SubView::setStatusBar (bool show) {}

void CSVDoc::SubView::useHint (const std::string& hint) {}

void CSVDoc::SubView::setUniversalId (const CSMWorld::UniversalId& id)
{
    mUniversalId = id;
    setWindowTitle (QString::fromUtf8(mUniversalId.toString().c_str()));
    emit universalIdChanged (mUniversalId);
}

void CSVDoc::SubView::closeEvent (QCloseEvent *event)
{
    emit updateSubViewIndices (this);
}

std::string CSVDoc::SubView::getTitle() const
{
    return mUniversalId.toString();
}

void CSVDoc::SubView::closeRequest()
{
    emit closeRequest (this);
}
