#include "scriptsubview.hpp"

#include <stdexcept>

#include <QStatusBar>
#include <QStackedLayout>

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

void CSVWorld::ScriptSubView::addButtonBar()
{
    if (mButtons)
        return;

    mButtons = new RecordButtonBar (getUniversalId(), *mModel, 0, &mCommandDispatcher, this);

    mLayout.insertWidget (1, mButtons);

    connect (mButtons, SIGNAL (switchToRow (int)), this, SLOT (switchToRow (int)));

    connect (this, SIGNAL (universalIdChanged (const CSMWorld::UniversalId&)),
        mButtons, SLOT (universalIdChanged (const CSMWorld::UniversalId&)));
}

CSVWorld::ScriptSubView::ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id), mDocument (document), mColumn (-1), mBottom(0), mButtons (0),
  mCommandDispatcher (document, CSMWorld::UniversalId::getParentType (id.getType()))
{
    std::vector<std::string> selection (1, id.getId());
    mCommandDispatcher.setSelection (selection);

    mLayout.addWidget (mEditor = new ScriptEdit (mDocument, ScriptHighlighter::Mode_General, this), 2);

    QWidget *widget = new QWidget (this);;
    widget->setLayout (&mLayout);
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
    if (CSMSettings::UserSettings::instance().setting ("script-editor/toolbar", QString("true")) == "true")
        addButtonBar();

    // bottom box
    mBottom = new TableBottomBox (CreatorFactory<GenericCreator>(), document, id, this);

    connect (mBottom, SIGNAL (requestFocus (const std::string&)),
        this, SLOT (requestFocus (const std::string&)));

    mLayout.addWidget (mBottom);

    // signals
    connect (mEditor, SIGNAL (textChanged()), this, SLOT (textChanged()));

    connect (mModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (dataChanged (const QModelIndex&, const QModelIndex&)));

    connect (mModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (rowsAboutToBeRemoved (const QModelIndex&, int, int)));

    updateStatusBar();
    connect(mEditor, SIGNAL(cursorPositionChanged()), this, SLOT(updateStatusBar()));
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

    if (mButtons)
        mButtons->updateUserSetting (name, value);
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
