#include "scriptsubview.hpp"

#include <stdexcept>

#include <QStatusBar>
#include <QStackedLayout>
#include <QSplitter>
#include <QTimer>

#include "../../model/doc/document.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/settings/usersettings.hpp"

#include "scriptedit.hpp"
#include "recordbuttonbar.hpp"
#include "tablebottombox.hpp"
#include "genericcreator.hpp"
#include "scripterrortable.hpp"

void CSVWorld::ScriptSubView::addButtonBar()
{
    if (mButtons)
        return;

    mButtons = new RecordButtonBar (getUniversalId(), *mModel, mBottom, &mCommandDispatcher, this);

    mLayout.insertWidget (1, mButtons);

    connect (mButtons, SIGNAL (switchToRow (int)), this, SLOT (switchToRow (int)));

    connect (this, SIGNAL (universalIdChanged (const CSMWorld::UniversalId&)),
        mButtons, SLOT (universalIdChanged (const CSMWorld::UniversalId&)));
}

void CSVWorld::ScriptSubView::recompile()
{
    if (!mCompileDelay->isActive() && !isDeleted())
        mCompileDelay->start (
            CSMSettings::UserSettings::instance().setting ("script-editor/compile-delay").toInt());
}

bool CSVWorld::ScriptSubView::isDeleted() const
{
    return mModel->data (mModel->getModelIndex (getUniversalId().getId(), mStateColumn)).toInt()
        ==CSMWorld::RecordBase::State_Deleted;
}

void CSVWorld::ScriptSubView::updateDeletedState()
{
    if (isDeleted())
    {
        mErrors->clear();
        mEditor->setEnabled (false);
    }
    else
    {
        mEditor->setEnabled (true);
        recompile();
    }
}

CSVWorld::ScriptSubView::ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id), mDocument (document), mColumn (-1), mBottom(0), mButtons (0),
  mCommandDispatcher (document, CSMWorld::UniversalId::getParentType (id.getType()))
{
    std::vector<std::string> selection (1, id.getId());
    mCommandDispatcher.setSelection (selection);

    mMain = new QSplitter (this);
    mMain->setOrientation (Qt::Vertical);
    mLayout.addWidget (mMain, 2);

    mEditor = new ScriptEdit (mDocument, ScriptHighlighter::Mode_General, this);
    mMain->addWidget (mEditor);
    mMain->setCollapsible (0, false);

    mErrors = new ScriptErrorTable (document, this);
    mMain->addWidget (mErrors);

    QWidget *widget = new QWidget (this);;
    widget->setLayout (&mLayout);
    setWidget (widget);

    mModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Scripts));

    mColumn = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_ScriptText);
    mIdColumn = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    mStateColumn = mModel->findColumnIndex (CSMWorld::Columns::ColumnId_Modification);

    QString source = mModel->data (mModel->getModelIndex (id.getId(), mColumn)).toString();

    mEditor->setPlainText (source);
    // bottom box and buttons
    mBottom = new TableBottomBox (CreatorFactory<GenericCreator>(), document, id, this);

    if (CSMSettings::UserSettings::instance().setting ("script-editor/toolbar", QString("true")) == "true")
        addButtonBar();

    connect (mBottom, SIGNAL (requestFocus (const std::string&)),
        this, SLOT (switchToId (const std::string&)));

    mLayout.addWidget (mBottom);

    // signals
    connect (mEditor, SIGNAL (textChanged()), this, SLOT (textChanged()));

    connect (mModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (dataChanged (const QModelIndex&, const QModelIndex&)));

    connect (mModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (rowsAboutToBeRemoved (const QModelIndex&, int, int)));

    updateStatusBar();
    connect(mEditor, SIGNAL(cursorPositionChanged()), this, SLOT(updateStatusBar()));

    mErrors->update (source.toUtf8().constData());

    connect (mErrors, SIGNAL (highlightError (int, int)),
        this, SLOT (highlightError (int, int)));

    mCompileDelay = new QTimer (this);
    mCompileDelay->setSingleShot (true);
    connect (mCompileDelay, SIGNAL (timeout()), this, SLOT (updateRequest()));

    updateDeletedState();
}

void CSVWorld::ScriptSubView::updateUserSetting (const QString& name, const QStringList& value)
{
    if (name == "script-editor/show-linenum")
    {
        std::string showLinenum = value.at(0).toUtf8().constData();
        mEditor->showLineNum(showLinenum == "true");
        mBottom->setVisible(showLinenum == "true");
    }
    else if (name == "script-editor/mono-font")
    {
        mEditor->setMonoFont (value.at(0)==QString ("true"));
    }
    else if (name=="script-editor/toolbar")
    {
        if (value.at(0)==QString ("true"))
        {
            addButtonBar();
        }
        else
        {
            if (mButtons)
            {
                mLayout.removeWidget (mButtons);
                delete mButtons;
                mButtons = 0;
            }
        }
    }
    else if (name=="script-editor/compile-delay")
    {
        mCompileDelay->setInterval (value.at (0).toInt());
    }

    if (mButtons)
        mButtons->updateUserSetting (name, value);

    mErrors->updateUserSetting (name, value);

    if (name=="script-editor/warnings")
        recompile();
}

void CSVWorld::ScriptSubView::setStatusBar (bool show)
{
    mBottom->setStatusBar (show);
}

void CSVWorld::ScriptSubView::updateStatusBar ()
{
    mBottom->positionChanged (mEditor->textCursor().blockNumber() + 1,
        mEditor->textCursor().columnNumber() + 1);
}

void CSVWorld::ScriptSubView::setEditLock (bool locked)
{
    mEditor->setReadOnly (locked);

    if (mButtons)
        mButtons->setEditLock (locked);

    mCommandDispatcher.setEditLock (locked);
}

void CSVWorld::ScriptSubView::useHint (const std::string& hint)
{
    if (hint.empty())
        return;

    unsigned line = 0, column = 0;
    char c;
    std::istringstream stream (hint.c_str()+1);
    switch(hint[0]){
        case 'R':
        case 'r':
        {
            QModelIndex index = mModel->getModelIndex (getUniversalId().getId(), mColumn);
            QString source = mModel->data (index).toString();
            unsigned pos, dummy;
            if (!(stream >> c >> dummy >> pos) )
                return;

            for (unsigned i = 0; i <= pos; ++i){
                if (source[i] == '\n'){
                    ++line;
                    column = i+1;
                }
            }
            column = pos - column;
            break;
        }
        case 'l':
            if (!(stream >> c >> line >> column))
                    return;
    }

    QTextCursor cursor = mEditor->textCursor();

    cursor.movePosition (QTextCursor::Start);
    if (cursor.movePosition (QTextCursor::Down, QTextCursor::MoveAnchor, line))
        cursor.movePosition (QTextCursor::Right, QTextCursor::MoveAnchor, column);

    mEditor->setFocus();
    mEditor->setTextCursor (cursor);
}

void CSVWorld::ScriptSubView::textChanged()
{
    if (mEditor->isChangeLocked())
        return;

    ScriptEdit::ChangeLock lock (*mEditor);

    QString source = mEditor->toPlainText();

    mDocument.getUndoStack().push (new CSMWorld::ModifyCommand (*mModel,
        mModel->getModelIndex (getUniversalId().getId(), mColumn), source));

    recompile();
}

void CSVWorld::ScriptSubView::dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (mEditor->isChangeLocked())
        return;

    ScriptEdit::ChangeLock lock (*mEditor);

    bool updateRequired = false;

    for (int i=topLeft.row(); i<=bottomRight.row(); ++i)
    {
        std::string id = mModel->data (mModel->index (i, mIdColumn)).toString().toUtf8().constData();
        if (mErrors->clearLocals (id))
            updateRequired = true;
    }

    QModelIndex index = mModel->getModelIndex (getUniversalId().getId(), mColumn);

    if (index.row()>=topLeft.row() && index.row()<=bottomRight.row())
    {
        if (mStateColumn>=topLeft.column() && mStateColumn<=bottomRight.column())
            updateDeletedState();

        if (mColumn>=topLeft.column() && mColumn<=bottomRight.column())
        {
            QString source = mModel->data (index).toString();

            QTextCursor cursor = mEditor->textCursor();
            mEditor->setPlainText (source);
            mEditor->setTextCursor (cursor);

            updateRequired = true;
        }
    }

    if (updateRequired)
        recompile();
}

void CSVWorld::ScriptSubView::rowsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{
    bool updateRequired = false;

    for (int i=start; i<=end; ++i)
    {
        std::string id = mModel->data (mModel->index (i, mIdColumn)).toString().toUtf8().constData();
        if (mErrors->clearLocals (id))
            updateRequired = true;
    }

    if (updateRequired)
        recompile();

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

    updateDeletedState();
}

void CSVWorld::ScriptSubView::switchToId (const std::string& id)
{
    switchToRow (mModel->getModelIndex (id, 0).row());
}

void CSVWorld::ScriptSubView::highlightError (int line, int column)
{
    QTextCursor cursor = mEditor->textCursor();

    cursor.movePosition (QTextCursor::Start);
    if (cursor.movePosition (QTextCursor::Down, QTextCursor::MoveAnchor, line))
        cursor.movePosition (QTextCursor::Right, QTextCursor::MoveAnchor, column);

    mEditor->setFocus();
    mEditor->setTextCursor (cursor);
}

void CSVWorld::ScriptSubView::updateRequest()
{
    QModelIndex index = mModel->getModelIndex (getUniversalId().getId(), mColumn);

    QString source = mModel->data (index).toString();

    mErrors->update (source.toUtf8().constData());
}
