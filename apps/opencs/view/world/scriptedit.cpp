#include "scriptedit.hpp"

#include <algorithm>

#include <QDragEnterEvent>
#include <QRegExp>
#include <QString>
#include <QPainter>
#include <QTextDocumentFragment>
#include <QMenu>

#include "../../model/doc/document.hpp"

#include "../../model/world/universalid.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/prefs/shortcut.hpp"

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

CSVWorld::ScriptEdit::ScriptEdit(
    const CSMDoc::Document& document,
    ScriptHighlighter::Mode mode,
    QWidget* parent
) : QPlainTextEdit(parent),
    mChangeLocked(0),
    mShowLineNum(false),
    mLineNumberArea(0),
    mDefaultFont(font()),
    mMonoFont(QFont("Monospace")),
    mTabCharCount(4),
    mMarkOccurrences(true),
    mDocument(document),
    mWhiteListQoutes("^[a-z|_]{1}[a-z|0-9|_]{0,}$", Qt::CaseInsensitive)
{
    wrapLines(false);
    setTabWidth();
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
    
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(markOccurrences()));

    mCommentAction = new QAction (tr ("Comment Selection"), this);
    connect(mCommentAction, SIGNAL (triggered()), this, SLOT (commentSelection()));
    CSMPrefs::Shortcut *commentShortcut = new CSMPrefs::Shortcut("script-editor-comment", this);
    commentShortcut->associateAction(mCommentAction);

    mUncommentAction = new QAction (tr ("Uncomment Selection"), this);
    connect(mUncommentAction, SIGNAL (triggered()), this, SLOT (uncommentSelection()));
    CSMPrefs::Shortcut *uncommentShortcut = new CSMPrefs::Shortcut("script-editor-uncomment", this);
    uncommentShortcut->associateAction(mUncommentAction);

    mHighlighter = new ScriptHighlighter (document.getData(), mode, ScriptEdit::document());

    connect (&document.getData(), SIGNAL (idListChanged()), this, SLOT (idListChanged()));

    connect (&mUpdateTimer, SIGNAL (timeout()), this, SLOT (updateHighlighting()));

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));
    {
        ChangeLock lock (*this);
        CSMPrefs::get()["Scripts"].update();
    }

    mUpdateTimer.setSingleShot (true);

    // TODO: provide a font selector dialogue
    mMonoFont.setStyleHint(QFont::TypeWriter);

    mLineNumberArea = new LineNumberArea(this);
    updateLineNumberAreaWidth(0);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
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

void CSVWorld::ScriptEdit::setTabWidth()
{
    // Set tab width to specified number of characters using current font.
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    setTabStopDistance(mTabCharCount * fontMetrics().horizontalAdvance(' '));
#else
    setTabStopWidth(mTabCharCount * fontMetrics().width(' '));
#endif

}

void CSVWorld::ScriptEdit::wrapLines(bool wrap)
{
    if (wrap)
    {
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
    }
    else
    {
        setLineWrapMode(QPlainTextEdit::NoWrap);
    }
}

void CSVWorld::ScriptEdit::settingChanged(const CSMPrefs::Setting *setting)
{
    // Determine which setting was changed.
    if (mHighlighter->settingChanged(setting))
    {
        updateHighlighting();
    }
    else if (*setting == "Scripts/mono-font")
    {
        setFont(setting->isTrue() ? mMonoFont : mDefaultFont);
        setTabWidth();
    }
    else if (*setting == "Scripts/show-linenum")
    {
        showLineNum(setting->isTrue());
    }
    else if (*setting == "Scripts/wrap-lines")
    {
        wrapLines(setting->isTrue());
    }
    else if (*setting == "Scripts/tab-width")
    {
        mTabCharCount = setting->toInt();
        setTabWidth();
    }
    else if (*setting == "Scripts/highlight-occurrences")
    {
        mMarkOccurrences = setting->isTrue();
        mHighlighter->setMarkedWord("");
        updateHighlighting();
        mHighlighter->setMarkOccurrences(mMarkOccurrences);
    }
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

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
#else
    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;
#endif

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

void CSVWorld::ScriptEdit::markOccurrences()
{
    if (mMarkOccurrences)
    {
        QTextCursor cursor = textCursor();

        // prevent infinite recursion with cursor.select(),
        // which ends up calling this function again
        // could be fixed with blockSignals, but mDocument is const
        disconnect(this, SIGNAL(cursorPositionChanged()), this, 0);
        cursor.select(QTextCursor::WordUnderCursor);
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(markOccurrences()));

        QString word = cursor.selectedText();
        mHighlighter->setMarkedWord(word.toStdString());
        mHighlighter->rehighlight();
    }
}
  
void CSVWorld::ScriptEdit::commentSelection()
{
    QTextCursor begin = textCursor();
    QTextCursor end = begin;
    begin.setPosition(begin.selectionStart());
    begin.movePosition(QTextCursor::StartOfLine);

    end.setPosition(end.selectionEnd());
    end.movePosition(QTextCursor::EndOfLine);

    begin.beginEditBlock();

    for (; begin < end; begin.movePosition(QTextCursor::EndOfLine), begin.movePosition(QTextCursor::Right))
    {
        begin.insertText(";");
    }

    begin.endEditBlock();
}

void CSVWorld::ScriptEdit::uncommentSelection()
{
    QTextCursor begin = textCursor();
    QTextCursor end = begin;
    begin.setPosition(begin.selectionStart());
    begin.movePosition(QTextCursor::StartOfLine);

    end.setPosition(end.selectionEnd());
    end.movePosition(QTextCursor::EndOfLine);

    begin.beginEditBlock();

    for (; begin < end; begin.movePosition(QTextCursor::EndOfLine), begin.movePosition(QTextCursor::Right)) {
        begin.select(QTextCursor::LineUnderCursor);
        QString line = begin.selectedText();

        if (line.size() == 0)
            continue;

        // get first nonspace character in line
        int index;
        for (index = 0; index != line.size(); ++index)
        {
            if (!line[index].isSpace())
                break;
        }

        if (index != line.size() && line[index] == ';')
        {
            // remove the semicolon
            line.remove(index, 1);
            // put the line back
            begin.insertText(line);
        }
    }

    begin.endEditBlock();
}

void CSVWorld::ScriptEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    mLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CSVWorld::ScriptEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    // remove redo/undo since they are disabled
    QList<QAction*> menuActions = menu->actions();
    for (QList<QAction*>::iterator i = menuActions.begin(); i < menuActions.end(); ++i)
    {
        if ((*i)->text().contains("Undo") || (*i)->text().contains("Redo"))
        {
            (*i)->setVisible(false);
        }
    }
    menu->addAction(mCommentAction);
    menu->addAction(mUncommentAction);

    menu->exec(event->globalPos());
    delete menu;
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
