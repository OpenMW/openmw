#ifndef CSVSETTINGS_LISTVIEW_HPP
#define CSVSETTINGS_LISTVIEW_HPP

#include "view.hpp"

namespace CSVSettings
{
    class ListView : public View
    {

        QWidget *mListWidget;

    public:
        explicit ListView (QAbstractItemModel *listModel,
                            const CSMSettings::Setting *setting,
                            QWidget *parent = 0);

    protected:

        void build(const CSMSettings::Setting *setting);
    };

    class ListViewFactory : public QObject, public IViewFactory
    {
        Q_OBJECT

    public:
        explicit ListViewFactory (QWidget *parent = 0)
            : QObject (parent)
        {}

        ListView *createView (CSMSettings::DefinitionModel &model,
                               const CSMSettings::Setting *setting);
    };
}
#endif // CSVSETTINGS_LISTVIEW_HPP
