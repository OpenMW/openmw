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

        ///set the value displayed in the spin box
        void setValue (const QString &value);

        ///set the stringlist that's used as a list of pre-defined values
        ///to be displayed as the user scrolls
        void setValueList (const QStringList &list);

        ///returns the pre-defined value list.
        const QStringList &valueList() const            { return mValueList; }

    protected:

        ///converts an index value to corresponding text to be displayed
        QString textFromValue (int val) const;

        ///converts a text value to a corresponding index
        int valueFromText (const QString &text) const;
    };
}
#endif // CSVSETTINGS_SPINBOX_HPP
