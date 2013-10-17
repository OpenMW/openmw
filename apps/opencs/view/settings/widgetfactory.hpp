#ifndef CSVSETTINGS_WIDGETFACTORY_HPP
#define CSVSETTINGS_WIDGETFACTORY_HPP

#include <QSortFilterProxyModel>
#include <QDataWidgetMapper>

#include <QDebug>
#include "settingbox.hpp"

namespace CSVSettings {


    /// Helper class that manages the data mapping for the widget
    class ViewWidget : public QObject
    {
        Q_OBJECT

    public:

        explicit ViewWidget (QWidget *widget, QSortFilterProxyModel *model, QWidget *parent = 0);

    };

    /// Interface must be inherited by all typed widget factory classes
    class IWidgetFactory
    {

    public:
        virtual ~IWidgetFactory() {}
        virtual void createWidget(const QString &name) = 0;

    };

    /// Base class for typed widget factory classes.
    /// Provides common fields and methods
    class TypedWidgetFactory
    {

    protected:

        QWidget *mParent;
        QLayout *mLayout;
        QSortFilterProxyModel *mSourceModel;

    public:

        explicit TypedWidgetFactory (QLayout *layout, QSortFilterProxyModel *model, QWidget *parent = 0);

    private:
        void setupWidget(const QString &caption, QWidget *widget);
    };

    /// Typed Factories
    class CheckBoxFactory : public TypedWidgetFactory, public IWidgetFactory
    {

    public:
        explicit CheckBoxFactory (QLayout *layout, QWidget *parent = 0);

        void createWidget(const QString &name);
    };

    class ComboBoxFactory : public TypedWidgetFactory, public IWidgetFactory
    {

    public:
        explicit ComboBoxFactory (QLayout *layout, QWidget *parent = 0);

        void createWidget(const QString &name);
    };

    class ListBoxFactory : public TypedWidgetFactory, public IWidgetFactory
    {

    public:
        explicit ListBoxFactory (QLayout *layout, QWidget *parent = 0);

        void createWidget(const QString &name);
    };

    class SpinBoxFactory : public TypedWidgetFactory, public IWidgetFactory
    {

    public:
        explicit SpinBoxFactory (QLayout *layout, QWidget *parent = 0);

        void createWidget(const QString &name);
    };

    class RadioButtonFactory : public TypedWidgetFactory, public IWidgetFactory
    {

    public:
        explicit RadioButtonFactory (QLayout *layout, QWidget *parent = 0);

        void createWidget(const QString &name);
    };

    class ToggleButtonFactory : public TypedWidgetFactory, public IWidgetFactory
    {

    public:
        explicit ToggleButtonFactory (QLayout *layout, QWidget *parent = 0);

        void createWidget(const QString &name) {}
    };

    class LineEditFactory : public TypedWidgetFactory, public IWidgetFactory
    {

    public:
        explicit LineEditFactory (QLayout *layout, QWidget *parent = 0);

        void createWidget(const QString &name);
    };

    /// Factory class which provides constructor functions to generate
    /// typed factories that inherit the IWidgetFactory interface
    class WidgetFactory
    {
    public:

        static IWidgetFactory *checkBox (QLayout *layout, QWidget *parent)
        {
            return new CheckBoxFactory (layout, parent);
        }

        static IWidgetFactory *comboBox (QLayout *layout, QWidget *parent)
        {
            return new ComboBoxFactory (layout, parent);
        }

        static IWidgetFactory *listBox (QLayout *layout, QWidget *parent)
        {
            return new ListBoxFactory (layout, parent);
        }

        static IWidgetFactory *spinBox (QLayout *layout, QWidget *parent)
        {
            return new SpinBoxFactory (layout, parent);
        }

        static IWidgetFactory *radioButton (QLayout *layout, QWidget *parent)
        {
            return new CheckBoxFactory (layout, parent);
        }

        static IWidgetFactory *toggleButton (QLayout *layout, QWidget *parent)
        {
            //return new ToggleButtonFactory (layout, parent);
        }

        static IWidgetFactory *lineEdit (QLayout *layout, QWidget *parent)
        {
            return new LineEditFactory (layout, parent);
        }

    };
}

#endif // CSVSETTINGS_WIDGETFACTORY_HPP
