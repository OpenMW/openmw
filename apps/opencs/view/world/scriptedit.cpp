#include "scriptedit.hpp"

#include <algorithm>

#include <QDragEnterEvent>
#include <QRegExp>
#include <QString>

#include "../../model/world/universalid.hpp"
#include "../../model/world/tablemimedata.hpp"

CSVWorld::ScriptEdit::ScriptEdit (QWidget* parent, const CSMDoc::Document& document) :
    QTextEdit (parent),
    mDocument (document),
    mWhiteListQoutes("^[a-z|_]{1}[a-z|0-9|_]{0,}$", Qt::CaseInsensitive)
{
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
                  <<CSMWorld::UniversalId::Type_Weapon;
}

void CSVWorld::ScriptEdit::dragEnterEvent (QDragEnterEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (!mime)
        QTextEdit::dragEnterEvent(event);
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
        QTextEdit::dragMoveEvent(event);
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
        QTextEdit::dropEvent(event);
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
