#ifndef CSM_TOOLS_SEARCH_H
#define CSM_TOOLS_SEARCH_H

#include <string>
#include <set>

#include <QRegExp>
#include <QMetaType>

class QModelIndex;

namespace CSMDoc
{
    class Messages;
    class Document;
}

namespace CSMWorld
{
    class IdTableBase;
    class UniversalId;
}

namespace CSMTools
{
    class Search
    {
        public:

            enum Type
            {
                Type_Text = 0,
                Type_TextRegEx = 1,
                Type_Id = 2,
                Type_IdRegEx = 3,
                Type_RecordState = 4,
                Type_None
            };

        private:

            Type mType;
            std::string mText;
            QRegExp mRegExp;
            int mValue;
            bool mCase;
            std::set<int> mColumns;
            int mIdColumn;
            int mTypeColumn;
            int mPaddingBefore;
            int mPaddingAfter;

            void searchTextCell (const CSMWorld::IdTableBase *model, const QModelIndex& index,
                const CSMWorld::UniversalId& id, bool writable, CSMDoc::Messages& messages) const;

            void searchRegExCell (const CSMWorld::IdTableBase *model, const QModelIndex& index,
                const CSMWorld::UniversalId& id, bool writable, CSMDoc::Messages& messages) const;

            void searchRecordStateCell (const CSMWorld::IdTableBase *model,
                const QModelIndex& index, const CSMWorld::UniversalId& id, bool writable,
                CSMDoc::Messages& messages) const;

            QString formatDescription (const QString& description, int pos, int length) const;

            QString flatten (const QString& text) const;
            
        public:

            Search();
        
            Search (Type type, bool caseSensitive, const std::string& value);

            Search (Type type, bool caseSensitive, const QRegExp& value);

            Search (Type type, bool caseSensitive, int value);

            // Configure search for the specified model.
            void configure (const CSMWorld::IdTableBase *model);

            // Search row in \a model and store results in \a messages.
            //
            // \attention *this needs to be configured for \a model.
            void searchRow (const CSMWorld::IdTableBase *model, int row,
                CSMDoc::Messages& messages) const;

            void setPadding (int before, int after);

            // Configuring *this for the model is not necessary when calling this function.
            void replace (CSMDoc::Document& document, CSMWorld::IdTableBase *model,
                const CSMWorld::UniversalId& id, const std::string& messageHint,
                const std::string& replaceText) const;

            // Check if model still matches search results.
            bool verify (CSMDoc::Document& document, CSMWorld::IdTableBase *model,
                const CSMWorld::UniversalId& id, const std::string& messageHint) const;
    };
}

Q_DECLARE_METATYPE (CSMTools::Search)

#endif
