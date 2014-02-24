#ifndef CSVSETTINGS_BOOLEANVIEW_HPP
#define CSVSETTINGS_BOOELANVIEW_HPP

#include <QList>
#include <QWidget>

#include "view.hpp"
#include "support.hpp"

class QDataWidgetMapper;
class QAbstractItemModel;
class QSortFilterProxyModel;
class QStandardItemModel;
class QStringListModel;

namespace CSMSettings {
    class Adapter;
}

namespace CSVSettings
{
    class BooleanView : public View
    {
        QList <QWidget *> mWidgets;

        QStringListModel *mModel;

    public:
        explicit BooleanView (const CSMSettings::Setting &setting,
                              CSMSettings::Adapter *adapter,
                              QWidget *parent = 0);


    protected:

        void createView (const CSMSettings::Setting &setting);
        void createModel (const CSMSettings::Setting &setting);
    };

    class BooleanViewFactory : public QObject, public IViewFactory
    {
        Q_OBJECT

    public:
        explicit BooleanViewFactory (QWidget *parent = 0)
            : QObject (parent)
        {}

        BooleanView *createView (QStandardItemModel *model,
                                 const CSMSettings::Setting &setting);
    };
}
#endif // CSVSETTINGS_BOOLEANVIEW_HPP
