#ifndef CSVSETTINGS_BOOLEANVIEW_HPP
#define CSVSETTINGS_BOOLEANVIEW_HPP

#include <QWidget>
#include <QAbstractButton>

#include "view.hpp"
#include "../../model/settings/support.hpp"

class QStringListModel;

namespace CSVSettings
{
    class BooleanView : public View
    {
        Q_OBJECT

        QMap <QString, QAbstractButton *> mButtons;
        enum CSMSettings::SettingType mType;

    public:
        explicit BooleanView (CSMSettings::Setting *setting,
                              Page *parent);

    protected:
        void updateView (bool signalUpdate = true) const;

    private slots:
        void slotToggled (bool state);
    };

    class BooleanViewFactory : public QObject, public IViewFactory
    {
        Q_OBJECT

    public:
        explicit BooleanViewFactory (QWidget *parent = 0)
            : QObject (parent)
        {}

        BooleanView *createView (CSMSettings::Setting *setting,
                                 Page *parent);
    };
}
#endif // CSVSETTINGS_BOOLEANVIEW_HPP
