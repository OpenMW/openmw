
#include "scriptsubview.hpp"

#include <stdexcept>

#include <QTextEdit>

#include "../../model/doc/document.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"

#include "scripthighlighter.hpp"
#include "scriptedit.hpp"

CSVWorld::ScriptSubView::ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id), mDocument (document), mColumn (-1)
{
    setWidget (mEditor = new ScriptEdit (this, mDocument));

    mModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Scripts));

    for (int i=0; i<mModel->columnCount(); ++i)
        if (mModel->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display)==
            CSMWorld::ColumnBase::Display_Script)
        {
            mColumn = i;
            break;
        }

    if (mColumn==-1)
        throw std::logic_error ("Can't find script column");

    mEditor->setPlainText (mModel->data (mModel->getModelIndex (id.getId(), mColumn)).toString());

    connect (mEditor, SIGNAL (textChanged()), this, SLOT (textChanged()));

    connect (mModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (dataChanged (const QModelIndex&, const QModelIndex&)));

    connect (mModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (rowsAboutToBeRemoved (const QModelIndex&, int, int)));

    connect (&document.getData(), SIGNAL (idListChanged()), this, SLOT (idListChanged()));

    mHighlighter = new ScriptHighlighter (document.getData(), mEditor->document());

    connect (&mUpdateTimer, SIGNAL (timeout()), this, SLOT (updateHighlighting()));

    mUpdateTimer.setSingleShot (true);
}

void CSVWorld::ScriptSubView::setEditLock (bool locked)
{
    mEditor->setReadOnly (locked);
}

void CSVWorld::ScriptSubView::idListChanged()
{
    mHighlighter->invalidateIds();

    if (!mUpdateTimer.isActive())
        mUpdateTimer.start (0);
}

void CSVWorld::ScriptSubView::textChanged()
{
    if (mEditor->isChangeLocked())
        return;

    ScriptEdit::ChangeLock lock (*mEditor);

    mDocument.getUndoStack().push (new CSMWorld::ModifyCommand (*mModel,
        mModel->getModelIndex (getUniversalId().getId(), mColumn), mEditor->toPlainText()));
}

void CSVWorld::ScriptSubView::dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (mEditor->isChangeLocked())
        return;

    ScriptEdit::ChangeLock lock (*mEditor);

    QModelIndex index = mModel->getModelIndex (getUniversalId().getId(), mColumn);

    if (index.row()>=topLeft.row() && index.row()<=bottomRight.row() &&
        index.column()>=topLeft.column() && index.column()<=bottomRight.column())
    {
        QTextCursor cursor = mEditor->textCursor();
        mEditor->setPlainText (mModel->data (index).toString());
        mEditor->setTextCursor (cursor);
    }
}

void CSVWorld::ScriptSubView::rowsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{
    QModelIndex index = mModel->getModelIndex (getUniversalId().getId(), mColumn);

    if (!parent.isValid() && index.row()>=start && index.row()<=end)
        deleteLater();
}

void CSVWorld::ScriptSubView::updateHighlighting()
{
    if (mEditor->isChangeLocked())
        return;

    ScriptEdit::ChangeLock lock (*mEditor);

    mHighlighter->rehighlight();
}