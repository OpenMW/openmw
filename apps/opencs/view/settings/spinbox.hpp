#ifndef CSVSETTINGS_SPINBOX_HPP
#define CSVSETTINGS_SPINBOX_HPP

#include <QObject>
#include <QStringList>
#include <QSpinBox>

namespace CSVSettings
{
    class SpinBox : public QSpinBox
    {
        Q_OBJECT

        QStringList mValueList;

    public:
        explicit SpinBox(QWidget *parent = 0);

        void setObjectName (const QString &name);

        void setValue (const QString &value);
        void setValueList (const QStringList &list);
        const QStringList &valueList() const            { return mValueList; }

    protected:

        QString textFromValue (int val) const;
        int valueFromText (const QString &text) const;
    };
}
#endif // CSVSETTINGS_SPINBOX_HPP
