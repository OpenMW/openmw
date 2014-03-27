#ifndef CSMSETTINGS_SETTINGFILTERMODEL_HPP
#define CSMSETTINGS_SETTINGFILTERMODEL_HPP

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace CSMSettings
{
    class SettingFilterModel : public QSortFilterProxyModel
    {
        Q_OBJECT

        typedef QPair <int, QString> ExpPair;
        typedef QList <ExpPair> Filter;

        QMap <QString, Filter> mFilters;
        Filter mCurrentFilter;
        QStandardItemModel mModel;

    public:
        explicit SettingFilterModel(QObject *parent = 0);

        void addFilterExpression (const QString &filterName, int column,
                                                    const QString &expression);

        void setFilterExpression (const QString &filterName, int column,
                                                    const QString & expression);

        void setCurrentFilter (const QString &filterName);

        void addModelColumn (const QStringList &list);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

        //void createFilter (const QString &filterName, const ExpList & expList);

    signals:

    public slots:

    };
}
#endif // CSMSETTINGS_SETTINGFILTERMODEL_HPP
