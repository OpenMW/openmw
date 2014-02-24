#ifndef CSVSETTINGS_VIEW_HPP
#define CSVSETTINGS_VIEW_HPP

#include <QWidget>

#include "support.hpp"

class QAbstractItemModel;
class QGroupBox;
class QDataWidgetMapper;
class QStandardItemModel;

namespace CSMSettings
{
    class Setting;
    class Adapter;
}

namespace CSVSettings
{
    class SettingBox;

    class View : public QWidget
    {
        Q_OBJECT

        CSMSettings::Adapter *mModel;

        SettingBox *mViewFrame;

       QStringList mValueList;
        bool mIsMultiValue;

    public:

        explicit View (const CSMSettings::Setting &setting,
                       CSMSettings::Adapter *adapter,
                       QWidget *parent = 0);

        SettingBox *viewFrame() const                    { return mViewFrame; }

    protected:

 //       QStandardItemModel *model()                         { return mModel; }
        QStringList valueList() const;
        bool isMultiValue() const;
    };

    class IViewFactory
    {
    public:
        virtual View *createView (const CSMSettings::Setting &setting) = 0;
    };
}
#endif // SCVSETTINGS_VIEW_HPP
