#ifndef CSVSETTINGS_LISTVIEW_HPP
#define CSVSETTINGS_LISTVIEW_HPP

#include "view.hpp"


class QStringListModel;
class QComboBox;
class QAbstractItemView;

namespace CSVSettings
{
    class ListView : public View
    {

        QAbstractItemView *mAbstractItemView;
        QComboBox *mComboBox;

    public:
        explicit ListView (CSMSettings::Setting *setting,
                            Page *parent);

    protected:

        void build (CSMSettings::Setting *setting);
        void slotUpdateView (QStringList list);
        void showEvent ( QShowEvent * event );

    protected:

        void slotTextEdited (QString value);
    };

    class ListViewFactory : public QObject, public IViewFactory
    {
        Q_OBJECT

    public:
        explicit ListViewFactory (QWidget *parent = 0)
            : QObject (parent)
        {}

        ListView *createView (CSMSettings::Setting *setting,
                              Page *parent);
    };
}
#endif // CSVSETTINGS_LISTVIEW_HPP
