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
}

namespace CSVSettings
{
    class SettingBox;

    class View : public QWidget
    {
        Q_OBJECT

        QAbstractItemModel *mModel;

        SettingBox *mViewFrame;

       QStringList mValueList;
        bool mIsMultiValue;

    public:

        explicit View (QAbstractItemModel *model,
                      const CSMSettings::Setting *setting, QWidget *parent = 0);

        SettingBox *viewFrame() const                    { return mViewFrame; }

    protected:

        QAbstractItemModel *model()                         { return mModel; }
        QStringList valueList() const;
        bool isMultiValue() const;
    };

    class IViewFactory
    {
    public:
        virtual View *createView (QStandardItemModel &model,
                                  const CSMSettings::Setting *setting) = 0;
    };
}
#endif // SCVSETTINGS_VIEW_HPP
