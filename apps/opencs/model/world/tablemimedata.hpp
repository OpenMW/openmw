/*This class provides way to construct mimedata object holding the reference to the 
* universalid. universalid is used in the majority of the tables to store type, id, argument types*/

#ifndef TABLEMIMEDATA_H
#define TABLEMIMEDATA_H

#include <qt4/QtCore/QMimeData>
#include <QStringList>

#include "universalid.hpp"

class QStringList;

namespace CSMWorld
{
    class UniversalId;
    class TableMimeData : public QMimeData
    {
        public:
            TableMimeData(UniversalId id);
            ~TableMimeData();
            virtual QStringList formats() const;
            UniversalId& getId();

        private:
            QStringList mSupportedFormats;
            UniversalId mUniversalId;
    };
}
#endif // TABLEMIMEDATA_H
