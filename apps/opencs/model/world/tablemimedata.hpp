/*This class provides way to construct mimedata object holding the reference to the 
* universalid. universalid is used in the majority of the tables to store type, id, argument types*/

#ifndef TABLEMIMEDATA_H
#define TABLEMIMEDATA_H

#include <vector>

#include <qt4/QtCore/QMimeData>
#include <QStringList>

#include "universalid.hpp"


namespace CSMWorld
{
    class TableMimeData : public QMimeData
    {
        public:
            TableMimeData(UniversalId id);
            TableMimeData(std::vector<UniversalId>& id);
            ~TableMimeData();
            virtual QStringList formats() const;
            UniversalId getId(unsigned int index) const;
            std::string getIcon() const;

        private:
            std::vector<UniversalId> mUniversalId;
            QStringList mObjectsFormats;
    };
}
#endif // TABLEMIMEDATA_H
