/*This class provides way to construct mimedata object holding the reference to the 
* universalid. universalid is used in the majority of the tables to store type, id, argument types*/

#ifndef TABLEMIMEDATA_H
#define TABLEMIMEDATA_H

#include <vector>

#include <QtCore/QMimeData>
#include <QStringList>

#include "universalid.hpp"
#include "columnbase.hpp"


namespace CSMWorld
{
    class TableMimeData : public QMimeData
    {
        public:
            TableMimeData(UniversalId id);
            TableMimeData(std::vector<UniversalId>& id);
            ~TableMimeData();
            virtual QStringList formats() const;
            std::string getIcon() const;
            std::vector<UniversalId> getData() const;
            bool holdsType(UniversalId::Type type) const;
            bool holdsType(CSMWorld::ColumnBase::Display type);
            UniversalId returnMatching(UniversalId::Type type) const;
            UniversalId returnMatching(CSMWorld::ColumnBase::Display type);

        private:
            std::vector<UniversalId> mUniversalId;
            QStringList mObjectsFormats;

            CSMWorld::UniversalId::Type convertEnums(CSMWorld::ColumnBase::Display type);
    };
}
#endif // TABLEMIMEDATA_H
