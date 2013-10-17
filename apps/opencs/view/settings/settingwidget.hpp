#ifndef SETTINGWIDGET_HPP
#define SETTINGWIDGET_HPP

#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QComboBox>
#include <QListWidget>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "abstractwidget.hpp"

namespace CSVSettings
{
    /// Generic template for radiobuttons / checkboxes
    template <typename T1>
    class SettingWidget : public AbstractWidget
    {

        T1 *mWidget;

    public:

        explicit SettingWidget (const QString &name, QLayout *layout,  QWidget* parent = 0)
            : AbstractWidget (name, layout, parent), mWidget (new T1 (parent))
        {
            setWidget (mWidget);

            mWidget->setText(name);
            build ("");
            //mWidget->setChecked(isDefault);
        }
    };

    /// spin box template
    template <>
    class SettingWidget <QSpinBox>: public AbstractWidget
    {

        QSpinBox *mWidget;

    public:

        explicit SettingWidget (const QString &name, QLayout *layout,  QWidget* parent = 0)
            : AbstractWidget (name, layout, parent), mWidget (new QSpinBox (parent))
        {
            //NEEDS TO BE MOVED TO INSTANCING CODE
//            def.caption += tr(" (%1 to %2)").arg(def.minMax->left).arg(def.minMax->right);

            setWidget (mWidget);
            build (name);

            //mWidget->setAlignment (getAlignment(align));
        }
    };

    /// combo box template
    template <>
    class SettingWidget <QComboBox>: public CSVSettings::AbstractWidget
    {

        QComboBox *mWidget;


    public:

        explicit SettingWidget (const QString &name, QLayout *layout,  QWidget* parent = 0)
            : AbstractWidget (name, layout, parent), mWidget (new QComboBox (parent))
        {
            setWidget (mWidget);
            build (name);

            //center the combo box items
            mWidget->setEditable (true);
            mWidget->lineEdit()->setReadOnly (true);
            //mWidget->lineEdit()->setAlignment (getAlignment(align));
            //mWidget->setModel(model);
        }

        bool isListWidget() { return true; }
    };

    /// line edit template
    template <>
    class SettingWidget <QLineEdit>: public CSVSettings::AbstractWidget
    {

        QLineEdit *mWidget;

    public:

        explicit SettingWidget (const QString &name, QLayout *layout,  QWidget* parent = 0)
            : AbstractWidget (name, layout, parent), mWidget (new QLineEdit (parent))
        {
            int column = 2;

            setWidget(mWidget, column);
            build (name);

           // mWidget->setAlignment (getAlignment(align));
        }

        void setInputMask (const QString &mask)
        { mWidget->setInputMask (mask); }
    };

    /// list widget template
    /// \todo Not fully implemented.  Only widget supporting multi-valued settings
    template <>
    class SettingWidget <QListWidget>: public CSVSettings::AbstractWidget
    {

        QListWidget *mWidget;

    public:

        explicit SettingWidget (const QString &name, QLayout *layout,  QWidget* parent = 0)
            : AbstractWidget (name, layout, parent), mWidget (new QListWidget (parent))
        {
            setWidget (mWidget);
            build (name);
        }

        static bool isListWidget() { return true; }
    };

}
#endif // SETTINGWIDGET_HPP
