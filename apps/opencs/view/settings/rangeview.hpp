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

        QWidget *mRangeWidget;
        CSMSettings::SettingType mRangeType;

    public:
        explicit RangeView (CSMSettings::Setting *setting,
                              Page *parent);

    protected:

        ///virtual function called through View
        void updateView (bool signalUpdate = true) const;

        ///construct a slider-based view
        void buildSlider (CSMSettings::Setting *setting);

        ///construct a spinbox-based view
        void buildSpinBox (CSMSettings::Setting *setting);

    private slots:

        ///responds to valueChanged signals
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
