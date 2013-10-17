#ifndef SETTINGVIEW_HPP
#define SETTINGVIEW_HPP

#include <QGroupBox>
#include <QSortFilterProxyModel>

#include "support.hpp"
#include "../../model/settings/setting.hpp"
#include "abstractwidget.hpp"
#include "widgetfactory.hpp"

#include <QDebug>

//--------------------------
//  TODO
//
//  SettingView manages the view of the setting model data, representing only one setting in the model.
//  It takes a reference to the setting model, as well as the section name and setting name
//  that it represents.  From here, it creates two FilterProxyModels which filter the view
//  first by section, then by setting.
//
//  The setting view instantiates the setting widget that the user requests by enum
//  (line edit, check box, combo box, list view, toggle button, radio button, spin box, etc)
//  The view then evaluates the setting meta data to determine how to set up the widget.
//
//  Thus a radiobutton setting would likely have a value list which would require a button for each value.
//  However, only one value would be selectable, and the setting view would manage this interaction.
//
//  another example: A checkbox/toggle button widget with a multi-valued setting would represent a series of bit flags...

class QSortFilterProxyModel;

namespace CSVSettings
{

    class SettingView : public QGroupBox
    {
        Q_OBJECT

        QSortFilterProxyModel *mSettingFilter;
        QList<QModelIndex> mSettingIndices;
        static const QString INVISIBLE_BOX_STYLE;
        QString mVisibleBoxStyle;
        WidgetFactory *mWidgetFactory;
        QList <ViewWidget *> mViewWidgets;

    public:

        explicit SettingView (const QString &name, WidgetType widgetType, bool ishorizontal, QWidget *parent);

        void setModel (QSortFilterProxyModel *settingModel);

        void setTitle (const QString &title);
        void setBorderVisibility (bool value);
        bool borderVisibile() const;

    private:

        void setupView(bool isHorizontal = false);
        void setMinimumWidth();
        void buildWidgets();
        void makeFactory(WidgetType widgetType);
    };
}
#endif // SETTINGVIEW_HPP
