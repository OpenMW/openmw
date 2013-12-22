#ifndef CSVSETTINGS_SETTINGFACTORY_HPP
#define CSVSETTINGS_SETTINGFACTORY_HPP

#include <QLabel>

#include "support.hpp"

class QWidget;
class QSortFilterProxyModel;
class QDataWidgetMapper;
class QLayout;
class QString;
class QAbstractButton;


namespace CSMSettings { class Setting; }

namespace CSVSettings
{
    class SettingFactory : public QObject
    {

        QWidget *mParent;

    //Private Classes
    private:

        class Setting : public QWidget
        {
            bool mHasLabel;
            bool mIsBinary;
            QWidget *mWidget;

        public:

            template <typename T>
            static Setting* create(const QString &name,
                                QSortFilterProxyModel *model, QWidget *parent)
            {
                Setting *setting = new Setting(parent);
                setting->configureView(name, new T(parent));
                setting->configureModel(model);

                return setting;
            }

            inline bool hasLabel() const { return mHasLabel; }
            inline bool isBinary() const { return mIsBinary; }

        private:

            explicit Setting (QWidget *parent = 0);

            void configureView(const QString &name, QWidget *widget);
            void configureModel(QSortFilterProxyModel *model);
        };

    public:

        explicit SettingFactory (QWidget *parent  = 0)
            : QObject (parent), mParent(parent)
        {}

        Setting *createSetting(WidgetType type, QSortFilterProxyModel *model,
                               const QString &settingFieldName);

        // need support functions for:
        //
        // setInputMask (configureWidget)
        // alignment
    };
}
#endif // CSVSETTINGS_SETTINGFACTORY_HPP
