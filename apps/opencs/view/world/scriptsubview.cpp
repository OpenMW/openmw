#include "scriptsubview.hpp"

#include <stdexcept>

#include <QStatusBar>
#include <QStackedLayout>
#include <QSplitter>
#include <QTimer>

#include <components/debug/debuglog.hpp>

#include "../../model/doc/document.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/prefs/state.hpp"

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
        mCompileDelay->start (CSMPrefs::get()["Scripts"]["compile-delay"].toInt());
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
        adjustSplitter();
        mEditor->setEnabled (false);
    }
    else
    {
        mEditor->setEnabled (true);
        recompile();
    }
}

void CSVWorld::ScriptSubView::adjustSplitter()
{
    QList<int> sizes;

    if (mErrors->rowCount())
    {
        if (mErrors->height())
            return; // keep old height if the error panel was already open

        sizes << (mMain->height()-mErrorHeight-mMain->handleWidth()) << mErrorHeight;
    }
    else
    {
        if (mErrors->height())
            mErrorHeight = mErrors->height();

        sizes << 1 << 0;
    }

    mMain->setSizes (sizes);
}

CSVWorld::ScriptSubView::ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id), mDocument (document), mColumn (-1), mBottom(0), mButtons (0),
  mCommandDispatcher (document, CSMWorld::UniversalId::getParentType (id.getType())),
  mErrorHeight (CSMPrefs::get()["Scripts"]["error-height"].toInt())
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

    QList<int> sizes;
    sizes << 1 << 0;
    mMain->setSizes (sizes);

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

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));
    CSMPrefs::get()["Scripts"].update();
}

void CSVWorld::ScriptSubView::setStatusBar (bool show)
{
    mBottom->setStatusBar (show);
}

void CSVWorld::ScriptSubView::settingChanged (const CSMPrefs::Setting *setting)
{
    if (*setting=="Scripts/toolbar")
    {
        if (setting->isTrue())
        {
            addButtonBar();
        }
        else if (mButtons)
        {
            mLayout.removeWidget (mButtons);
            delete mButtons;
            mButtons = 0;
        }
    }
    else if (*setting=="Scripts/compile-delay")
    {
        mCompileDelay->setInterval (setting->toInt());
    }
    else  if (*setting=="Scripts/warnings")
        recompile();
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
    switch(hint[0])
    {
        case 'R':
        case 'r':
        {
            QModelIndex index = mModel->getModelIndex (getUniversalId().getId(), mColumn);
            QString source = mModel->data (index).toString();
            unsigned stringSize = source.length();
            unsigned pos, dummy;
            if (!(stream >> c >> dummy >> pos) )
                return;

            if (pos > stringSize)
            {
                Log(Debug::Warning) << "CSVWorld::ScriptSubView: requested position is higher than actual string length";
                pos = stringSize;
            }

            for (unsigned i = 0; i <= pos; ++i)
            {
                if (source[i] == '\n')
                {
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

    bool oldSignalsState =  mEditor->blockSignals( true );
    mEditor->setPlainText( mModel->data(mModel->index(row, mColumn)).toString() );
    mEditor->blockSignals( oldSignalsState );

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

    adjustSplitter();
}
