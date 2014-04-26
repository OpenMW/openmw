#ifndef CSVSETTINGS_RANGEVIEW_HPP
#define CSVSETTINGS_RANGEVIEW_HPP

#include <QWidget>
#include <QAbstractButton>

#include "view.hpp"
#include "../../model/settings/support.hpp"

class QStringListModel;

namespace CSVSettings
{
    class RangeView : public View
    {
        Q_OBJECT

        QMap <QString, QAbstractButton *> mButtons;

    public:
        explicit RangeView (CSMSettings::Setting *setting,
                              Page *parent);

    protected:
        void updateView (bool signalUpdate = true) const;

    private slots:
        void slotToggled (bool state);
    };

    class RangeViewFactory : public QObject, public IViewFactory
    {
        Q_OBJECT

    public:
        explicit RangeViewFactory (QWidget *parent = 0)
            : QObject (parent)
        {}

        RangeView *createView (CSMSettings::Setting *setting,
                                 Page *parent);
    };
}
#endif // CSVSETTINGS_RANGEVIEW_HPP
