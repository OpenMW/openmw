/*This class provides way to construct mimedata object holding the reference to the 
* universalid. universalid is used in the majority of the tables to store type, id, argument types*/

#ifndef TABLEMIMEDATA_H
#define TABLEMIMEDATA_H

#include <qt4/QtCore/QMimeData>
#include <QVariant>

namespace CSMWorld
{
    class UniversalId;
    class TableMimeData : public QMimeData
    {
        public:
            TableMimeData(UniversalId& UniversalId);
            ~TableMimeData();
            virtual QStringList formats() const;
            UniversalId& getId();

        private:
            QStringList mSupportedFormats;
            UniversalId& mUniversalId;
    };
}
#endif // TABLEMIMEDATA_H
