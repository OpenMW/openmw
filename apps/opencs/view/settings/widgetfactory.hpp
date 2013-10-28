#ifndef CSVSETTINGS_WIDGETFACTORY_HPP
#define CSVSETTINGS_WIDGETFACTORY_HPP

#include "support.hpp"
z
class QWidget;
class QSortFilterProxyModel;
class QDataWidgetMapper;
class QLayout;
class QString;

namespace CSMSettings { class Setting; }

namespace CSVSettings
{
    class WidgetFactory : public QObject
    {

        QWidget *mParent;

    public:

        explicit WidgetFactory (QWidget *parent  = 0)
            : mHasLabel (false), mIsBinary (false), QObject (parent)
        {}

        QLayout *createLayout (QSortFilterProxyModel *model);

    private:

        const CSMSettings::Setting *getSetting(model) const;
        SettingWidget *createWidget (const QString &name, CSVSettings::WidgetType widgetType);

        QSortFilterProxyModel *buildModelConnections (QSortFilterProxyModel *model, SettingWidget *widget);

        void buildMapper                    (QWidget *widget, QSortFilterProxyModel *filter);
        QSortFilterProxyModel *buildModel   (QWidget *widget);
        QLayout *buildLayout                (bool isHorizontal);

        // need support functions for:
        //
        // setInputMask
        // alignment

    //Private Classes
    private:

        template <typename T>
        class SettingWidget : public QWidget
        {
            bool mHasLabel;
            bool mIsBinary;
            T *mWidget;

        public:
            explicit SettingWidget (const QString &name, QWidget *parent = 0);

            inline bool hasLabel() const { return mHasLabel; }
            inline bool isBinary() const { return mIsBinary; }

        };
    };
}
#endif // CSVSETTINGS_WIDGETFACTORY_HPP
