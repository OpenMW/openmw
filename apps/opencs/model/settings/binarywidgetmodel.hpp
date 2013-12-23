#ifndef BINARYWIDGETMODEL_HPP
#define BINARYWIDGETMODEL_HPP

#include <QStringList>
#include <QObject>

namespace CSMSettings
{
    class BinaryWidgetModel : public QObject
    {
        Q_OBJECT

        QString mMatchValue;
        QStringList &mValueList;

    public:

        explicit BinaryWidgetModel(QStringList &values, QObject *parent = 0);

        void setMatchValue  (const QString &value)
            { mMatchValue = value; }

        int rowCount () const
            { return mValueList.count(); }

        QStringList values () const
            { return mValueList; }

        bool insertItem     (const QString &item);
        bool removeItem     (const QString &item);
    };
}
#endif // BINARYWIDGETMODEL_HPP
