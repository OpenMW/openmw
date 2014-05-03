#ifndef CSVSETTINGS_RANGEVIEW_HPP
#define CSVSETTINGS_RANGEVIEW_HPP

#include "view.hpp"
#include "../../model/settings/support.hpp"

class QStringListModel;
class QAbstractSpinBox;

namespace CSVSettings
{
    class RangeView : public View
    {
        Q_OBJECT

        QAbstractSpinBox *mRangeWidget;
        CSMSettings::SettingType mRangeType;

    public:
        explicit RangeView (CSMSettings::Setting *setting,
                              Page *parent);

    protected:
        void updateView (bool signalUpdate = true) const;

        void buildSpinBox (CSMSettings::Setting *setting);

    private slots:

        void slotUpdateView (int value);
        void slotUpdateView (double value);

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
