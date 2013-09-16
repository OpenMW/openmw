#include <QSortFilterProxyModel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "settingpage.hpp"
#include "../../model/settings/settingmodel.hpp"
#include "settingbox.hpp"

CSVSettings::SettingPage::SettingPage(const QString &pageName, const CSMSettings::SettingModel *model, QWidget *parent) :
    QWidget(parent)
{
    mSectionFilter = new QSortFilterProxyModel (this);
    mSectionFilter->setFilterFixedString (pageName);
    mSectionFilter->setFilterKeyColumn (1);
    mSectionFilter->setDynamicSortFilter (true);

    mBox = new SettingBox (Orient_Horizontal, false, this);
}

void CSVSettings::SettingPage::addView (WidgetType widgType, const QString &viewName)
{
    SettingView *view = 0;

    switch (widgType)
    {
    case Widget_CheckBox:
        view = CreateView<QCheckBox> (viewName);
        break;
    case Widget_ComboBox:
        view = CreateView<QComboBox> (viewName);
        break;
    case Widget_LineEdit:
        view = createView<QLineEdit> (viewName);
        break;
    case Widget_ListBox:
        view = createView<QListView> (viewName);
        break;
    case Widget_RadioButton:
        view = createView<QRadioButton> (viewName);
        break;
    case Widget_SpinBox:
        view = createView<QSpinBox> (viewName);
        break;
    case Widget_ToggleButton:
        view = createView<QToggleButton> (viewName);
        break;

    default:
        break;
    }

    mViews.append (view);
    mBox->layout()->addWiget (view);
    new SettingView (widgType, viewName, mSectionFilter, this);
}
