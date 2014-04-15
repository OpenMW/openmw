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

        void updateView (bool signalUpdate = true) const;
        void showEvent ( QShowEvent * event );

        ///Receives signal from widget and signals viwUpdated()
        void slotTextEdited (QString value);

    private:

        ///Helper function to construct a model for an AbstractItemView
        void buildAbstractItemViewModel();

        ///Helper function to construct a model for a combobox
        void buildComboBoxModel();

        ///Helper function to build the view widget
        QWidget *buildWidget (bool isMultiLine, int width);

    private slots:

        ///Receives updates from single-select widgets (like combobox) and
        ///signals viewUpdated with the selected values.
        void emitItemViewUpdate (int idx);
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
