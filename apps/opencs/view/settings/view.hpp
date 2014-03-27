#ifndef CSVSETTINGS_VIEW_HPP
#define CSVSETTINGS_VIEW_HPP

#include <QWidget>
#include <QMap>

#include "support.hpp"

class QGroupBox;
class QStringList;

namespace CSMSettings
{
    class Setting;
    class Selector;
}

namespace CSVSettings
{
    class SettingBox;
    class Page;

    class View : public QWidget
    {
        Q_OBJECT

        SettingBox *mViewFrame;
        CSMSettings::Selector *mSelector;
        CSMSettings::Setting *mSetting;
        Page *mParentPage;

    public:

        explicit View (CSMSettings::Setting *setting,
                       Page *parent);

        SettingBox *viewFrame() const                    { return mViewFrame; }
        CSMSettings::Selector *selector()               { return mSelector; }
        CSMSettings::Setting *setting() const              { return mSetting; }

    protected:
        void showEvent ( QShowEvent * event );

    public slots:
        virtual void slotUpdateView (QStringList values) = 0;
    };

    class IViewFactory
    {
    public:
        virtual View *createView (CSMSettings::Setting *setting,
                                  Page *parent) = 0;
    };
}
#endif // CSVSETTINGS_VIEW_HPP
