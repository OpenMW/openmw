#ifndef CSVSETTINGS_VIEW_HPP
#define CSVSETTINGS_VIEW_HPP

#include <QWidget>

#include "support.hpp"

class QAbstractItemModel;
class QGroupBox;
class QDataWidgetMapper;

namespace CSMSettings
{
    class Setting;
    class DefinitionModel;
}

namespace CSVSettings
{
    class View : public QWidget
    {
        Q_OBJECT

        QAbstractItemModel *mModel;

        QGroupBox *mViewFrame;

        const CSMSettings::Setting *mSetting;

    public:

        explicit View(QAbstractItemModel *model,
                      const CSMSettings::Setting *setting, QWidget *parent = 0);

        QGroupBox *viewFrame() const                    { return mViewFrame; }

    protected:

        QAbstractItemModel *model()                         { return mModel; }
        QStringList valueList() const;
        bool isMultiValue() const;
    };

    class IViewFactory
    {
    public:
        virtual View *createView (CSMSettings::DefinitionModel &model,
                                  const CSMSettings::Setting *setting) = 0;
    };
}
#endif // SCVSETTINGS_VIEW_HPP
