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

CSVWorld::ScriptSubView::ScriptSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: SubView (id), mDocument (document), mColumn (-1), mBottom(0), mStatus(0)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    mBottom = new QWidget(this);
    QStackedLayout *bottmLayout = new QStackedLayout(mBottom);
    bottmLayout->setContentsMargins (0, 0, 0, 0);
    QStatusBar *statusBar = new QStatusBar(mBottom);
    mStatus = new QLabel(mBottom);
    statusBar->addWidget (mStatus);
    bottmLayout->addWidget (statusBar);
    mBottom->setLayout (bottmLayout);

    layout->addWidget (mBottom, 0);
    layout->insertWidget (0, mEditor = new ScriptEdit (mDocument, ScriptHighlighter::Mode_General, this), 2);

    QWidget *widget = new QWidget;
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
        std::string showLinenum = value.at(0).toStdString();
        mEditor->showLineNum(showLinenum == "true");
        mBottom->setVisible(showLinenum == "true");
    }
    else if (name == "script-editor/mono-font")
    {
        mEditor->setMonoFont(value.at(0).toStdString() == "true");
    }
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

