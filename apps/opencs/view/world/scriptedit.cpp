#include "scriptedit.hpp"

#include <algorithm>

#include <QDragEnterEvent>
#include <QRegExp>
#include <QString>
#include <QPainter>
#include <QTextDocumentFragment>

#include "../../model/doc/document.hpp"

#include "../../model/world/universalid.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/settings/usersettings.hpp"


CSVWorld::ScriptEdit::ChangeLock::ChangeLock (ScriptEdit& edit) : mEdit (edit)
{
    ++mEdit.mChangeLocked;
}

CSVWorld::ScriptEdit::ChangeLock::~ChangeLock()
{
    --mEdit.mChangeLocked;
}

bool CSVWorld::ScriptEdit::event (QEvent *event)
{
    // ignore undo and redo shortcuts
    if (event->type()==QEvent::ShortcutOverride)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *> (event);

        if (keyEvent->matches (QKeySequence::Undo) || keyEvent->matches (QKeySequence::Redo))
            return true;
    }

    return QPlainTextEdit::event (event);
}

CSVWorld::ScriptEdit::ScriptEdit (const CSMDoc::Document& document, ScriptHighlighter::Mode mode,
    QWidget* parent)
    : QPlainTextEdit (parent),
    mChangeLocked (0),
    mShowLineNum(false),
    mLineNumberArea(0),
    mDefaultFont(font()),
    mMonoFont(QFont("Monospace")),
    mDocument (document),
    mWhiteListQoutes("^[a-z|_]{1}[a-z|0-9|_]{0,}$", Qt::CaseInsensitive)

{
//    setAcceptRichText (false);
    setLineWrapMode (QPlainTextEdit::NoWrap);
    setTabStopWidth (4);
    setUndoRedoEnabled (false); // we use OpenCS-wide undo/redo instead

    mAllowedTypes <<CSMWorld::UniversalId::Type_Journal
                  <<CSMWorld::UniversalId::Type_Global
                  <<CSMWorld::UniversalId::Type_Topic
                  <<CSMWorld::UniversalId::Type_Sound
                  <<CSMWorld::UniversalId::Type_Spell
                  <<CSMWorld::UniversalId::Type_Cell
                  <<CSMWorld::UniversalId::Type_Referenceable
                  <<CSMWorld::UniversalId::Type_Activator
                  <<CSMWorld::UniversalId::Type_Potion
                  <<CSMWorld::UniversalId::Type_Apparatus
                  <<CSMWorld::UniversalId::Type_Armor
                  <<CSMWorld::UniversalId::Type_Book
                  <<CSMWorld::UniversalId::Type_Clothing
                  <<CSMWorld::UniversalId::Type_Container
                  <<CSMWorld::UniversalId::Type_Creature
                  <<CSMWorld::UniversalId::Type_Door
                  <<CSMWorld::UniversalId::Type_Ingredient
                  <<CSMWorld::UniversalId::Type_CreatureLevelledList
                  <<CSMWorld::UniversalId::Type_ItemLevelledList
                  <<CSMWorld::UniversalId::Type_Light
                  <<CSMWorld::UniversalId::Type_Lockpick
                  <<CSMWorld::UniversalId::Type_Miscellaneous
                  <<CSMWorld::UniversalId::Type_Npc
                  <<CSMWorld::UniversalId::Type_Probe
                  <<CSMWorld::UniversalId::Type_Repair
                  <<CSMWorld::UniversalId::Type_Static
                  <<CSMWorld::UniversalId::Type_Weapon
                  <<CSMWorld::UniversalId::Type_Script
                  <<CSMWorld::UniversalId::Type_Region;

    mHighlighter = new ScriptHighlighter (document.getData(), mode, ScriptEdit::document());

    connect (&document.getData(), SIGNAL (idListChanged()), this, SLOT (idListChanged()));

    connect (&mUpdateTimer, SIGNAL (timeout()), this, SLOT (updateHighlighting()));

    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
    connect (&userSettings, SIGNAL (userSettingUpdated(const QString &, const QStringList &)),
             this, SLOT (updateUserSetting (const QString &, const QStringList &)));

    mUpdateTimer.setSingleShot (true);

    // TODO: provide a font selector dialogue
    mMonoFont.setStyleHint(QFont::TypeWriter);

    if (userSettings.setting("script-editor/mono-font", "true") == "true")
        setFont(mMonoFont);

    mLineNumberArea = new LineNumberArea(this);
    updateLineNumberAreaWidth(0);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));

    showLineNum(userSettings.settingValue("script-editor/show-linenum") == "true");
}

void CSVWorld::ScriptEdit::updateUserSetting (const QString &name, const QStringList &list)
{
    if (mHighlighter->updateUserSetting (name, list))
        updateHighlighting();
}

void CSVWorld::ScriptEdit::showLineNum(bool show)
{
    if(show!=mShowLineNum)
    {
        mShowLineNum = show;
        updateLineNumberAreaWidth(0);
    }
}

void CSVWorld::ScriptEdit::setMonoFont(bool show)
{
    if(show)
        setFont(mMonoFont);
    else
        setFont(mDefaultFont);
}

bool CSVWorld::ScriptEdit::isChangeLocked() const
{
    return mChangeLocked!=0;
}

void CSVWorld::ScriptEdit::dragEnterEvent (QDragEnterEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime)
        QPlainTextEdit::dragEnterEvent(event);
    else
    {
        setTextCursor (cursorForPosition (event->pos()));
        event->acceptProposedAction();
    }
}

void CSVWorld::ScriptEdit::dragMoveEvent (QDragMoveEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime)
        QPlainTextEdit::dragMoveEvent(event);
    else
    {
        setTextCursor (cursorForPosition (event->pos()));
        event->accept();
    }
}

void CSVWorld::ScriptEdit::dropEvent (QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
    {
        QPlainTextEdit::dropEvent(event);
        return;
    }

    setTextCursor (cursorForPosition (event->pos()));

    if (mime->fromDocument (mDocument))
    {
        std::vector<CSMWorld::UniversalId> records (mime->getData());

        for (std::vector<CSMWorld::UniversalId>::iterator it = records.begin(); it != records.end(); ++it)
        {
            if (mAllowedTypes.contains (it->getType()))
            {
                if (stringNeedsQuote(it->getId()))
                {
                    insertPlainText(QString::fromUtf8 (('"' + it->getId() + '"').c_str()));
                } else {
                    insertPlainText(QString::fromUtf8 (it->getId().c_str()));
                }
            }
        }
    }
}

bool CSVWorld::ScriptEdit::stringNeedsQuote (const std::string& id) const
{
    const QString string(QString::fromUtf8(id.c_str())); //<regex> is only for c++11, so let's use qregexp for now.
    //I'm not quite sure when do we need to put quotes. To be safe we will use quotes for anything other than…
    return !(string.contains(mWhiteListQoutes));
}

void CSVWorld::ScriptEdit::idListChanged()
{
    mHighlighter->invalidateIds();

    if (!mUpdateTimer.isActive())
        mUpdateTimer.start (0);
}

void CSVWorld::ScriptEdit::updateHighlighting()
{
    if (isChangeLocked())
        return;

    ChangeLock lock (*this);

    mHighlighter->rehighlight();
}

int CSVWorld::ScriptEdit::lineNumberAreaWidth()
{
    if(!mShowLineNum)
        return 0;

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void CSVWorld::ScriptEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CSVWorld::ScriptEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        mLineNumberArea->scroll(0, dy);
    else
        mLineNumberArea->update(0, rect.y(), mLineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CSVWorld::ScriptEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    mLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CSVWorld::ScriptEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(mLineNumberArea);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    int startBlock = textCursor().blockNumber();
    int endBlock = textCursor().blockNumber();
    if(textCursor().hasSelection())
    {
        QString str = textCursor().selection().toPlainText();
        int offset = str.count("\n");
        if(textCursor().position() < textCursor().anchor())
            endBlock += offset;
        else
            startBlock -= offset;
    }
    painter.setBackgroundMode(Qt::OpaqueMode);
    QFont font = painter.font();
    QBrush background = painter.background();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QFont newFont = painter.font();
            QString number = QString::number(blockNumber + 1);
            if(blockNumber >= startBlock && blockNumber <= endBlock)
            {
                painter.setBackground(Qt::cyan);
                painter.setPen(Qt::darkMagenta);
                newFont.setBold(true);
            }
            else
            {
                painter.setBackground(background);
                painter.setPen(Qt::black);
            }
            painter.setFont(newFont);
            painter.drawText(0, top, mLineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
            painter.setFont(font);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

CSVWorld::LineNumberArea::LineNumberArea(ScriptEdit *editor) : QWidget(editor), mScriptEdit(editor)
{}

QSize CSVWorld::LineNumberArea::sizeHint() const
{
    return QSize(mScriptEdit->lineNumberAreaWidth(), 0);
}

void CSVWorld::LineNumberArea::paintEvent(QPaintEvent *event)
{
    mScriptEdit->lineNumberAreaPaintEvent(event);
}
