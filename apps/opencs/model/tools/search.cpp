#include "search.hpp"

#include <stdexcept>
#include <sstream>

#include "../doc/messages.hpp"
#include "../doc/document.hpp"

#include "../world/idtablebase.hpp"
#include "../world/columnbase.hpp"
#include "../world/universalid.hpp"
#include "../world/commands.hpp"

void CSMTools::Search::searchTextCell (const CSMWorld::IdTableBase *model,
    const QModelIndex& index, const CSMWorld::UniversalId& id, bool writable,
    CSMDoc::Messages& messages) const
{
    // using QString here for easier handling of case folding.
    
    QString search = QString::fromUtf8 (mText.c_str());
    QString text = model->data (index).toString();

    int pos = 0;

    Qt::CaseSensitivity caseSensitivity = mCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
    while ((pos = text.indexOf (search, pos, caseSensitivity))!=-1)
    {
        std::ostringstream hint;
        hint
            << (writable ? 'R' : 'r')
            <<": "
            << model->getColumnId (index.column())
            << " " << pos
            << " " << search.length();
        
        messages.add (id, formatDescription (text, pos, search.length()).toUtf8().data(), hint.str());

        pos += search.length();
    }
}

void CSMTools::Search::searchRegExCell (const CSMWorld::IdTableBase *model,
    const QModelIndex& index, const CSMWorld::UniversalId& id, bool writable,
    CSMDoc::Messages& messages) const
{
    QString text = model->data (index).toString();

    int pos = 0;

    while ((pos = mRegExp.indexIn (text, pos))!=-1)
    {
        int length = mRegExp.matchedLength();
        
        std::ostringstream hint;
        hint
            << (writable ? 'R' : 'r')
            <<": "
            << model->getColumnId (index.column())
            << " " << pos
            << " " << length;
        
        messages.add (id, formatDescription (text, pos, length).toUtf8().data(), hint.str());

        pos += length;
    }
}

void CSMTools::Search::searchRecordStateCell (const CSMWorld::IdTableBase *model,
    const QModelIndex& index, const CSMWorld::UniversalId& id, bool writable, CSMDoc::Messages& messages) const
{
    if (writable)
        throw std::logic_error ("Record state can not be modified by search and replace");
        
    int data = model->data (index).toInt();

    if (data==mValue)
    {
        std::vector<std::pair<int,std::string>> states =
            CSMWorld::Columns::getEnums (CSMWorld::Columns::ColumnId_Modification);

        const std::string hint = "r: " + std::to_string(model->getColumnId(index.column()));
        messages.add (id, states.at(data).second, hint);
    }
}

QString CSMTools::Search::formatDescription (const QString& description, int pos, int length) const
{
    QString text (description);

    // split
    QString highlight = flatten (text.mid (pos, length));
    QString before = flatten (mPaddingBefore>=pos ?
        text.mid (0, pos) : text.mid (pos-mPaddingBefore, mPaddingBefore));
    QString after = flatten (text.mid (pos+length, mPaddingAfter));

    // compensate for Windows nonsense
    text.remove ('\r');

    // join
    text = before + "<b>" + highlight + "</b>" + after;

    // improve layout for single line display
    text.replace ("\n", "&lt;CR>");
    text.replace ('\t', ' ');
    
    return text; 
}

QString CSMTools::Search::flatten (const QString& text) const
{
    QString flat (text);

    flat.replace ("&", "&amp;");
    flat.replace ("<", "&lt;");

    return flat;
}

CSMTools::Search::Search() : mType (Type_None), mValue (0), mCase (false), mIdColumn (0), mTypeColumn (0),
    mPaddingBefore (10), mPaddingAfter (10) {}

CSMTools::Search::Search (Type type, bool caseSensitive, const std::string& value)
: mType (type), mText (value), mValue (0), mCase (caseSensitive), mIdColumn (0), mTypeColumn (0), mPaddingBefore (10), mPaddingAfter (10)
{
    if (type!=Type_Text && type!=Type_Id)
        throw std::logic_error ("Invalid search parameter (string)");
}

CSMTools::Search::Search (Type type, bool caseSensitive, const QRegExp& value)
: mType (type), mRegExp (value), mValue (0), mCase (caseSensitive), mIdColumn (0), mTypeColumn (0), mPaddingBefore (10), mPaddingAfter (10)
{
    mRegExp.setCaseSensitivity(mCase ? Qt::CaseSensitive : Qt::CaseInsensitive);
    if (type!=Type_TextRegEx && type!=Type_IdRegEx)
        throw std::logic_error ("Invalid search parameter (RegExp)");
}

CSMTools::Search::Search (Type type, bool caseSensitive, int value)
: mType (type), mValue (value), mCase (caseSensitive), mIdColumn (0), mTypeColumn (0), mPaddingBefore (10), mPaddingAfter (10)
{
    if (type!=Type_RecordState)
        throw std::logic_error ("invalid search parameter (int)");
}

void CSMTools::Search::configure (const CSMWorld::IdTableBase *model)
{
    mColumns.clear();

    int columns = model->columnCount();

    for (int i=0; i<columns; ++i)
    {
        CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display> (
            model->headerData (
            i,  Qt::Horizontal, static_cast<int> (CSMWorld::ColumnBase::Role_Display)).toInt());

        bool consider = false;

        switch (mType)
        {
            case Type_Text:
            case Type_TextRegEx:

                if (CSMWorld::ColumnBase::isText (display) ||
                    CSMWorld::ColumnBase::isScript (display))
                {
                    consider = true;
                }

                break;
                
            case Type_Id:
            case Type_IdRegEx:

                if (CSMWorld::ColumnBase::isId (display) ||
                    CSMWorld::ColumnBase::isScript (display))
                {
                    consider = true;
                }

                break;
            
            case Type_RecordState:

                if (display==CSMWorld::ColumnBase::Display_RecordState)
                    consider = true;

                break;

            case Type_None:

                break;
        }

        if (consider)
            mColumns.insert (i);
    }

    mIdColumn = model->findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    mTypeColumn = model->findColumnIndex (CSMWorld::Columns::ColumnId_RecordType);
}

void CSMTools::Search::searchRow (const CSMWorld::IdTableBase *model, int row,
    CSMDoc::Messages& messages) const
{
    for (std::set<int>::const_iterator iter (mColumns.begin()); iter!=mColumns.end(); ++iter)
    {
        QModelIndex index = model->index (row, *iter);

        CSMWorld::UniversalId::Type type = static_cast<CSMWorld::UniversalId::Type> (
            model->data (model->index (row, mTypeColumn)).toInt());
        
        CSMWorld::UniversalId id (
            type, model->data (model->index (row, mIdColumn)).toString().toUtf8().data());

        bool writable = model->flags (index) & Qt::ItemIsEditable;
            
        switch (mType)
        {
            case Type_Text:
            case Type_Id:

                searchTextCell (model, index, id, writable, messages);
                break;
            
            case Type_TextRegEx:
            case Type_IdRegEx:

                searchRegExCell (model, index, id, writable, messages);
                break;
            
            case Type_RecordState:

                searchRecordStateCell (model, index, id, writable, messages);
                break;

            case Type_None:

                break;
        }
    }
}

void CSMTools::Search::setPadding (int before, int after)
{
    mPaddingBefore = before;
    mPaddingAfter = after;
}

void CSMTools::Search::replace (CSMDoc::Document& document, CSMWorld::IdTableBase *model,
    const CSMWorld::UniversalId& id, const std::string& messageHint,
    const std::string& replaceText) const
{
    std::istringstream stream (messageHint.c_str());

    char hint, ignore;
    int columnId, pos, length;

    if (stream >> hint >> ignore >> columnId >> pos >> length)
    {
        int column =
            model->findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (columnId));
            
        QModelIndex index = model->getModelIndex (id.getId(), column);

        std::string text = model->data (index).toString().toUtf8().constData();

        std::string before = text.substr (0, pos);
        std::string after = text.substr (pos+length);

        std::string newText = before + replaceText + after;
        
        document.getUndoStack().push (
            new CSMWorld::ModifyCommand (*model, index, QString::fromUtf8 (newText.c_str())));
    }
}

bool CSMTools::Search::verify (CSMDoc::Document& document, CSMWorld::IdTableBase *model,
    const CSMWorld::UniversalId& id, const std::string& messageHint) const
{
    CSMDoc::Messages messages (CSMDoc::Message::Severity_Info);

    int row = model->getModelIndex (id.getId(),
        model->findColumnIndex (CSMWorld::Columns::ColumnId_Id)).row();
    
    searchRow (model, row, messages);

    for (CSMDoc::Messages::Iterator iter (messages.begin()); iter!=messages.end(); ++iter)
        if (iter->mHint==messageHint)
            return true;

    return false;
}
                
