#ifndef SETTINGVIEW_HPP
#define SETTINGVIEW_HPP

#include <QGroupBox>
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QListWidget>
#include <QMetaObject>
#include <QMetaMethod>

#include "support.hpp"
#include "../../model/settings/setting.hpp"

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
class QDataWidgetMapper;

namespace CSVSettings
{
    class SettingViewComponent
    {
        QWidget *mWidget;
        QAbstractButton *mAbstractButton;

    public:

        SettingViewComponent()
            : mWidget (0), mAbstractButton(0)
        {}

        template <typename T>
        void create(QWidget *parent)
        {
            T *widget = new T(parent);

            mWidget = widget;
            mAbstractButton = dynamic_cast<QAbstractButton *>(widget);
        }

        inline QWidget* widget() const { return mWidget; }
        inline QAbstractButton *abstractButton() const {return mAbstractButton;}
    };

    class SettingView : public QGroupBox
    {
        Q_OBJECT

        QSortFilterProxyModel *mSettingFilter;
        static const QString INVISIBLE_BOX_STYLE;
        QList <QWidget *> mViewWidgets;
        const CSMSettings::Setting *mSetting;

    public:

        explicit SettingView (const CSMSettings::Setting *setting, QWidget *parent = 0);

        void setModel (QSortFilterProxyModel *settingModel);
        void setTitle (const QString &title);

    private:

        void setupView(bool isHorizontal);
        void buildWidgets();
        SettingViewComponent createWidget(WidgetType type);
        void installWidget(SettingViewComponent &component, const QString &text);
        void installWidgetMapper(SettingViewComponent &component,
                                 const QString &settingName,
                                 QSortFilterProxyModel *model);
    };
}
#endif // SETTINGVIEW_HPP
