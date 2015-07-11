#include "scriptsubview.hpp"

#include <stdexcept>

#include <QStatusBar>
#include <QStackedLayout>
#include <QLabel>

#include "../../model/doc/document.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/settings/usersettings.hpp"

#include "scriptedit.hpp"
#include "recordbuttonbar.hpp"

CSVWorld::ScriptSubView::ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id), mDocument (document), mColumn (-1), mBottom(0), mStatus(0),
  mCommandDispatcher (document, CSMWorld::UniversalId::getParentType (id.getType()))
{
    std::vector<std::string> selection (1, id.getId());
    mCommandDispatcher.setSelection (selection);

    QVBoxLayout *layout = new QVBoxLayout;

    layout->addWidget (mEditor = new ScriptEdit (mDocument, ScriptHighlighter::Mode_General, this), 2);

    QWidget *widget = new QWidget (this);;
    widget->setLayout (layout);
    setWidget (widget);

    mModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Scripts));

    for (int i=0; i<mModel->columnCount(); ++i)
        if (mModel->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display)==
            CSMWorld::ColumnBase::Display_ScriptFile)
        {
            mColumn = i;
            break;
        }

    if (mColumn==-1)
        throw std::logic_error ("Can't find script column");

    mEditor->setPlainText (mModel->data (mModel->getModelIndex (id.getId(), mColumn)).toString());

    // buttons
    mButtons = new RecordButtonBar (id, *mModel, 0, &mCommandDispatcher, this);

    layout->addWidget (mButtons);

    // status bar
    QStatusBar *statusBar = new QStatusBar(mBottom);
    mStatus = new QLabel(mBottom);
    statusBar->addWidget (mStatus);

    mBottom = new QWidget(this);
    QStackedLayout *bottmLayout = new QStackedLayout(mBottom);
    bottmLayout->setContentsMargins (0, 0, 0, 0);
    bottmLayout->addWidget (statusBar);
    mBottom->setLayout (bottmLayout);

    layout->addWidget (mBottom, 0);

    // signals
    connect (mEditor, SIGNAL (textChanged()), this, SLOT (textChanged()));

    connect (mModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (dataChanged (const QModelIndex&, const QModelIndex&)));

    connect (mModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (rowsAboutToBeRemoved (const QModelIndex&, int, int)));

    connect (mButtons, SIGNAL (switchToRow (int)), this, SLOT (switchToRow (int)));

    connect (this, SIGNAL (universalIdChanged (const CSMWorld::UniversalId&)),
        mButtons, SLOT (universalIdChanged (const CSMWorld::UniversalId&)));

    updateStatusBar();
    connect(mEditor, SIGNAL(cursorPositionChanged()), this, SLOT(updateStatusBar()));
}

void CSVWorld::ScriptSubView::updateUserSetting (const QString& name, const QStringList& value)
{
    if (name == "script-editor/show-linenum")
    {
        std::string showLinenum = value.at(0).toStdString();
        mEditor->showLineNum(showLinenum == "true");
        mBottom->setVisible(showLinenum == "true");
    }
    else if (name == "script-editor/mono-font")
    {
        mEditor->setMonoFont(value.at(0).toStdString() == "true");
    }

    mButtons->updateUserSetting (name, value);
}

void CSVWorld::ScriptSubView::updateStatusBar ()
{
    std::ostringstream stream;

    stream << "(" << mEditor->textCursor().blockNumber() + 1 << ", "
        << mEditor->textCursor().columnNumber() + 1 << ")";

    mStatus->setText (QString::fromUtf8 (stream.str().c_str()));
}

void CSVWorld::ScriptSubView::setEditLock (bool locked)
{
    mEditor->setReadOnly (locked);
    mButtons->setEditLock (locked);
    mCommandDispatcher.setEditLock (locked);
}

void CSVWorld::ScriptSubView::useHint (const std::string& hint)
{
    if (hint.empty())
        return;

    if (hint[0]=='l')
    {
        std::istringstream stream (hint.c_str()+1);

        char ignore;
        int line;
        int column;

        if (stream >> ignore >> line >> column)
        {
            QTextCursor cursor = mEditor->textCursor();

            cursor.movePosition (QTextCursor::Start);
            if (cursor.movePosition (QTextCursor::Down, QTextCursor::MoveAnchor, line))
                cursor.movePosition (QTextCursor::Right, QTextCursor::MoveAnchor, column);

            mEditor->setFocus();
            mEditor->setTextCursor (cursor);
        }
    }
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
        emit closeRequest();
}

void CSVWorld::ScriptSubView::switchToRow (int row)
{
    int idColumn = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    std::string id = mModel->data (mModel->index (row, idColumn)).toString().toUtf8().constData();
    setUniversalId (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Script, id));

    mEditor->setPlainText (mModel->data (mModel->index (row, mColumn)).toString());

    std::vector<std::string> selection (1, id);
    mCommandDispatcher.setSelection (selection);
}
