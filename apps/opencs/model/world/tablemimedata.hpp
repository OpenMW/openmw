#ifndef TABLEMIMEDATA_H
#define TABLEMIMEDATA_H

#include <string>
#include <vector>

#include <QModelIndex>
#include <QStringList>
#include <QtCore/QMimeData>

#include "columnbase.hpp"
#include "universalid.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class DragRecordTable;
}

namespace CSMWorld
{

    /// \brief Subclass of QmimeData, augmented to contain and transport UniversalIds.
    ///
    /// This class provides way to construct mimedata object holding the universalid copy
    /// Universalid is used in the majority of the tables to store type, id, argument types.
    /// This way universalid grants a way to retrieve record from the concrete table.
    /// Please note, that tablemimedata object can hold multiple universalIds in the vector.

    class TableMimeData : public QMimeData
    {
        std::vector<UniversalId> mUniversalId;
        QStringList mObjectsFormats;
        const CSMDoc::Document& mDocument;
        const CSVWorld::DragRecordTable* mTableOfDragStart;
        QModelIndex mIndexAtDragStart;

    public:
        TableMimeData(UniversalId id, const CSMDoc::Document& document);

        TableMimeData(const std::vector<UniversalId>& id, const CSMDoc::Document& document);

        ~TableMimeData() override = default;

        QStringList formats() const override;

        std::string getIcon() const;

        std::vector<UniversalId> getData() const;

        bool holdsType(UniversalId::Type type) const;

        bool holdsType(CSMWorld::ColumnBase::Display type) const;

        bool fromDocument(const CSMDoc::Document& document) const;

        UniversalId returnMatching(UniversalId::Type type) const;

        const CSMDoc::Document* getDocumentPtr() const;

        UniversalId returnMatching(CSMWorld::ColumnBase::Display type) const;

        void setIndexAtDragStart(const QModelIndex& index) { mIndexAtDragStart = index; }

        void setTableOfDragStart(const CSVWorld::DragRecordTable* const table) { mTableOfDragStart = table; }

        const QModelIndex getIndexAtDragStart() const { return mIndexAtDragStart; }

        const CSVWorld::DragRecordTable* getTableOfDragStart() const { return mTableOfDragStart; }

        static CSMWorld::UniversalId::Type convertEnums(CSMWorld::ColumnBase::Display type);

        static CSMWorld::ColumnBase::Display convertEnums(CSMWorld::UniversalId::Type type);

        static bool isReferencable(CSMWorld::UniversalId::Type type);

    private:
        bool isReferencable(CSMWorld::ColumnBase::Display type) const;
    };
}
#endif // TABLEMIMEDATA_H
