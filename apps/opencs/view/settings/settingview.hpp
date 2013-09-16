#ifndef SETTINGVIEW_HPP
#define SETTINGVIEW_HPP

#include <QObject>

#include "support.hpp"
#include "../../model/settings/setting.hpp"

//--------------------------
//  TODO
//
//  SettingView manages the view of the setting model data.
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
//
//  a section view would also be a possible class, managing a base filterproxymodel that filters the setting model on the section.
//  The section view might also manage the arrangement of the settingsviews, (horizontal / vertical) and would represent a page.

class QSortFilterProxyModel;

namespace CSVSettings
{
    class SettingBox;

    template <typename T1>
    class SettingView : public QObject
    {
        Q_OBJECT

        QSortFilterProxyModel *mSettingFilter;
        SettingBox *mBox;

    public:
       explicit SettingView (const QString &viewName, QSortFilterProxyModel *settingModel, QObject *parent = 0) :
            QObject(parent),
            mBox (new SettingBox (Orient_Vertical, false, this)),
            mSettingFilter (buildFilter (viewName, settingModel))
        {
            QStringList valueList = settingModel->data (settingModel->index(0, 4, QModelIndex()), Qt::DisplayRole);

            if (valueList.size() > 0)
            {
                AbstractWidget *widget = new SettingWidget<T1>(valueList.at(0), mSettingFilter, mBox->layout(), this);

                if (widget->isMultiSelect())
                {
                    foreach (const QString &listItem, valueList)
                        new SettingWidget<T1> (listItem, mSettingFilter, mBox->layout(), this);

                    return;
                }
            }

            // pass-through to create widgets that are either multi-select or single-select with no value list
            SettingWidget *widget = new SettingWidget<T1> (viewName, mSettingFilter, mBox->layout(), this);
        }

    private:

        void buildFilter (const QString &viewName, QSortFilterProxyModel *settingModel);
    };
}
#endif // SETTINGVIEW_HPP
