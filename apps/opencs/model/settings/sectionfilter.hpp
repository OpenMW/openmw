#ifndef SECTIONFILTER_HPP
#define SECTIONFILTER_HPP

#include <QSortFilterProxyModel>
#include "usersettings.hpp"

namespace CSMSettings
{
    class SectionFilter : public QSortFilterProxyModel
    {

    public:
        explicit SectionFilter (const QString &sectionName, QObject *parent);

        void createSetting (const QString &name, const QString &value,
                            const QStringList &valueList);
    };
}
#endif // SECTIONFILTER_HPP
