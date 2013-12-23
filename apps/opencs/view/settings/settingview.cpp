#include <QSortFilterProxyModel>
#include <QDataWidgetMapper>
#include <QHBoxLayout>

#include "settingview.hpp"
#include "settingfactory.hpp"
#include "apps/opencs/model/settings/setting.hpp"
//#include "apps/opencs/model/settings/binarywidgetmodel.hpp"
#include "support.hpp"

#include <QDebug>

CSVSettings::SettingView::SettingView (const CSMSettings::Setting *setting, QWidget *parent) :
    QGroupBox(parent), mSettingFilter (0), mSetting(setting)
{
    setupView(mSetting->isHorizontal());

    setObjectName (mSetting->name());
    setTitle (mSetting->name());
 }

void CSVSettings::SettingView::buildWidgets ()
{
    SettingViewComponent svComponent = createWidget(mSetting->widgetType());
    QString settingName = mSetting->name();

    if (svComponent.abstractButton())
    {
        foreach (const QString &text, mSetting->valueList())
        {
            svComponent.widget()->setObjectName(text);
            installWidget(svComponent, text);
            installWidgetMapper(svComponent, settingName, mSettingFilter);

            svComponent = createWidget(mSetting->widgetType());
        }
        svComponent.widget()->deleteLater();
    }
    else
    {
       svComponent.widget()->setObjectName(settingName);
       installWidget(svComponent, settingName);
       installWidgetMapper(svComponent, settingName, mSettingFilter);
    }

}

void CSVSettings::SettingView::setModel (QSortFilterProxyModel *settingModel)
{
    mSettingFilter = new QSortFilterProxyModel (this);
    mSettingFilter->setFilterFixedString (objectName());
    mSettingFilter->setFilterKeyColumn (0);
    mSettingFilter->setSourceModel(settingModel);
    mSettingFilter->setDynamicSortFilter (true);

    qDebug() << "SettingView::setModel()::building widget based on section: "
             << objectName() << "; records: " << mSettingFilter->rowCount();

    buildWidgets();
}

CSVSettings::SettingViewComponent
    CSVSettings::SettingView::createWidget(WidgetType type)
{
    SettingViewComponent svc;

    switch (type)
    {
    case Widget_CheckBox:
        svc.create<QCheckBox>(this);
        break;

    case Widget_ComboBox:
        svc.create<QComboBox>(this);
        break;

    case Widget_LineEdit:
        svc.create<QLineEdit>(this);
        break;

    case Widget_ListBox:
        svc.create<QListWidget>(this);
        break;

    case Widget_RadioButton:
        svc.create<QRadioButton>(this);
        break;

    case Widget_SpinBox:
        svc.create<QSpinBox>(this);
        break;

    case Widget_ToggleButton:
        break;

    default:
        break;
    }

    return svc;
}

void CSVSettings::SettingView::installWidget(SettingViewComponent &component,
                                       const QString &text)
{
    if (!component.abstractButton())
    {
        QLabel *label = new QLabel (text, component.widget());
        label->setBuddy (component.widget());
        layout()->addWidget (label);
    }
    else
        component.abstractButton()->setText(text);

    layout()->addWidget(component.widget());
}

void CSVSettings::SettingView::installWidgetMapper(SettingViewComponent &component,
                                                   const QString &settingName,
                                                   QSortFilterProxyModel *model)
{/*
    QSortFilterProxyModel *filter = 0;

    if (component.abstractButton())
        filter = new CSMSettings::BinaryWidgetModel
                (component.widget()->objectName(), this);

    else
        filter = new QSortFilterProxyModel();

    QDataWidgetMapper *mapper = new QDataWidgetMapper (component.widget());

    filter->setSourceModel(model);
    filter->setFilterFixedString(settingName);
    filter->setDynamicSortFilter(true);

    mapper->setModel (filter);
    mapper->addMapping (component.widget(), 2);
    mapper->toFirst();
*/
}
//view layout code


const QString CSVSettings::SettingView::INVISIBLE_BOX_STYLE =
        QString::fromUtf8("QSettingBox { border: 0px; padding 0px; margin: 0px;}");

void CSVSettings::SettingView::setupView(bool isHorizontal)
{
    setFlat (true);

    if (isHorizontal)
        setLayout (new QHBoxLayout);
    else
        setLayout (new QVBoxLayout);
}

void CSVSettings::SettingView::setTitle (const QString &title)
{
    //if (borderVisibile() )
    {
        QGroupBox::setTitle (title);
        QGroupBox::setCheckable(false);
        //set minimum width to accommodate title, if needed
        //1.5 multiplier to account for bold title.
        QFontMetrics fm (font());
        int minWidth = fm.width(QGroupBox::title());
        setMinimumWidth (minWidth * 1.5);
    }
}
