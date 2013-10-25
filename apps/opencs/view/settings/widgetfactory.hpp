#ifndef CSVSETTINGS_WIDGETFACTORY_HPP
#define CSVSETTINGS_WIDGETFACTORY_HPP

#include "settingbox.hpp"

class QSortFilterProxyModel;
class QDataWidgetMapper;

namespace CSVSettings {


    /// Helper class that manages the data mapping for the widget
    class SettingWidget : public QWidget
    {
        Q_OBJECT

        QDataWidgetMapper *mAdapter;
        QSortFilterProxyModel *mFilterProxy;
        QWidget *mWidget;

    public:

        explicit SettingWidget (QWidget *widget, QSortFilterProxyModel *model, QWidget *parent = 0);

    private:

        void buildWidgetView();
        void buildWidgetModel (QSortFilterProxyModel *model);

    };

    /// Base factory inherited by all typed widget factory classes
    class WidgetFactory
    {

        QWidget *mParent;
        QLayout *mLayout;
        QSortFilterProxyModel *mSourceModel;
        QDataWidgetMapper *mDataAdapter;

    public:
        virtual ~WidgetFactory() {}
        virtual QWidget *createWidget (const QString &name);

    };

    /// Typed Factories
    class CheckBoxFactory : public WidgetFactory
    {

    public:
        explicit CheckBoxFactory (QLayout *layout, QWidget *parent = 0);

        QWidget *createWidget (const QString &name);
    };

    class ComboBoxFactory : public WidgetFactory
    {

    public:
        explicit ComboBoxFactory (QLayout *layout, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class ListBoxFactory : public WidgetFactory
    {

    public:
        explicit ListBoxFactory (QLayout *layout, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class SpinBoxFactory : public WidgetFactory
    {

    public:
        explicit SpinBoxFactory (QLayout *layout, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class RadioButtonFactory : public WidgetFactory
    {

    public:
        explicit RadioButtonFactory (QLayout *layout, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class ToggleButtonFactory : public WidgetFactory
    {

    public:
        explicit ToggleButtonFactory (QLayout *layout, QWidget *parent = 0);

        QWidget *createWidget(const QString &name) {}
    };

    class LineEditFactory : public WidgetFactory
    {

    public:
        explicit LineEditFactory (QLayout *layout, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };
}

#endif // CSVSETTINGS_WIDGETFACTORY_HPP
