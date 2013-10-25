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

        explicit SettingWidget (QWidget *widget, QSortFilterProxyModel *model,
                                const QString &caption, QWidget *parent = 0);

    private:

        void buildWidgetView (const QString &caption);
        void buildWidgetModel (QSortFilterProxyModel *model);

    };

    /// Base factory inherited by all typed widget factory classes
    class WidgetFactory
    {

        QWidget *mParent;
        QLayout *mLayout;
        QSortFilterProxyModel *mSourceModel;

    public:
        explicit WidgetFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent);

        virtual ~WidgetFactory() {}
        virtual QWidget *createWidget (const QString &name, QWidget *widget);

    };

    /// Typed Factories
    class CheckBoxFactory : public WidgetFactory
    {

    public:
        explicit CheckBoxFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

        QWidget *createWidget (const QString &name);
    };

    class ComboBoxFactory : public WidgetFactory
    {

    public:
        explicit ComboBoxFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class ListBoxFactory : public WidgetFactory
    {

    public:
        explicit ListBoxFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class SpinBoxFactory : public WidgetFactory
    {

    public:
        explicit SpinBoxFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class RadioButtonFactory : public WidgetFactory
    {

    public:
        explicit RadioButtonFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };

    class ToggleButtonFactory : public WidgetFactory
    {

    public:
        explicit ToggleButtonFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

        QWidget *createWidget(const QString &name) {}
    };

    class LineEditFactory : public WidgetFactory
    {

    public:
        explicit LineEditFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

        QWidget *createWidget(const QString &name);
    };
}

#endif // CSVSETTINGS_WIDGETFACTORY_HPP
