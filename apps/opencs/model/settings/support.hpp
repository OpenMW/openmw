#ifndef MODEL_SUPPORT_HPP
#define MODEL_SUPPORT_HPP

#include <QObject>
#include <QStringList>

class QLayout;
class QWidget;
class QListWidgetItem;

namespace CSMSettings
{
    class SettingContainer;

    typedef QList<SettingContainer *>          SettingList;
    typedef QMap<QString, SettingContainer *>  SettingMap;
    typedef QMap<QString, SettingMap *>        SectionMap;

    struct QStringPair
    {
        QStringPair(): left (""), right ("")
        {}

        QStringPair (const QString &leftValue, const QString &rightValue)
            : left (leftValue), right(rightValue)
        {}

        QStringPair (const QStringPair &pair)
            : left (pair.left), right (pair.right)
        {}

        QString left;
        QString right;

        bool isEmpty() const
        { return (left.isEmpty() && right.isEmpty()); }
    };
}
#endif // MODEL_SUPPORT_HPP
