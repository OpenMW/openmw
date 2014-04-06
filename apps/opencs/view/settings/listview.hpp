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
        Q_OBJECT

        QAbstractItemView *mAbstractItemView;
        QComboBox *mComboBox;

    public:
        explicit ListView (CSMSettings::Setting *setting,
                            Page *parent);

    protected:

        void updateView () const;
        void showEvent ( QShowEvent * event );

    private:

        void buildModel(QWidget *widget);
        void buildView (QWidget *widget);
        QWidget *buildWidget();

    protected:

        void slotTextEdited (QString value);

    private slots:
        void slotIndexChanged (int idx) const;
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
