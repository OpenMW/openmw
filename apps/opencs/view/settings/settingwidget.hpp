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

        explicit SettingWidget (WidgetDef &def, QLayout *layout, QWidget* parent = 0)
            : AbstractWidget (layout, parent), mWidget (new T1 (parent))
        {
            mWidget->setText(def.caption);
            build (mWidget, def, true);
            mWidget->setChecked(def.isDefault);

            connect (mWidget, SIGNAL (toggled (bool)),
                     this, SLOT (slotUpdateItem (bool)));
        }

        QWidget *widget()   { return mWidget; }

    private:

        void updateWidget (const QString &value)
        {
            if ( value == mWidget->objectName() && !mWidget->isChecked() )
                mWidget->setChecked (true);
        }
    };

    /// spin box template
    template <>
    class SettingWidget <QSpinBox>: public AbstractWidget
    {

        QSpinBox *mWidget;

    public:

        SettingWidget (WidgetDef &def, QLayout *layout, QWidget *parent = 0)
            : AbstractWidget (layout, parent), mWidget (new QSpinBox (parent))
        {
            def.caption += tr(" (%1 to %2)").arg(def.minMax->left).arg(def.minMax->right);

            mWidget->setMaximum     (def.minMax->right.toInt());
            mWidget->setMinimum     (def.minMax->left.toInt());
            mWidget->setValue       (def.value.toInt());

            build (mWidget, def);

            connect (mWidget, SIGNAL (valueChanged (int)),
                     this, SLOT (slotUpdateItem (int)));

            mWidget->setAlignment (getAlignment(def.valueAlignment));


        }

        QWidget *widget()   { return mWidget; }

    private:

        void updateWidget (const QString &value)
        {
            int intVal = value.toInt();

            if (intVal >= mWidget->minimum() && intVal <= mWidget->maximum() && intVal != mWidget->value())
                mWidget->setValue (intVal);
        }

    signals:

    };

    /// combo box template
    template <>
    class SettingWidget <QComboBox>: public CSVSettings::AbstractWidget
    {

        QComboBox *mWidget;


    public:

        explicit SettingWidget(WidgetDef &def, QLayout *layout, QWidget *parent = 0)
            : AbstractWidget (layout, parent), mWidget (new QComboBox (parent))
        {
            int i = 0;

            foreach (QString item, *(def.valueList))
            {
                mWidget->addItem (item);

                if (item == def.value)
                    mWidget->setCurrentIndex(i);

                i++;
            }

            build (mWidget, def);

            connect (mWidget, SIGNAL (currentIndexChanged (const QString &)),
                     this, SLOT (slotUpdateItem (const QString &)));

            //center the combo box items
            mWidget->setEditable (true);
            mWidget->lineEdit()->setReadOnly (true);
            mWidget->lineEdit()->setAlignment (getAlignment(def.valueAlignment));

            QFlags<Qt::AlignmentFlag> alignment = mWidget->lineEdit()->alignment();

            for (int j = 0; j < mWidget->count(); j++)
                mWidget->setItemData (j, QVariant(alignment), Qt::TextAlignmentRole);
        }

        QWidget *widget()   { return mWidget; }

    private:

        void updateWidget (const QString &value)
        {
            if (mWidget->currentText() != value)
                mWidget->setCurrentIndex(mWidget->findText(value));
        }

    };

    /// line edit template
    template <>
    class SettingWidget <QLineEdit>: public CSVSettings::AbstractWidget
    {

        QLineEdit *mWidget;

    public:

        explicit SettingWidget(WidgetDef &def, QLayout *layout, QWidget *parent = 0)
            : AbstractWidget (layout, parent), mWidget (new QLineEdit (parent))
        {
            if (!def.inputMask.isEmpty())
                mWidget->setInputMask (def.inputMask);

            mWidget->setText (def.value);

            build (mWidget, def);

            connect (mWidget, SIGNAL (textChanged (const QString &)),
                     this, SLOT (slotUpdateItem (const QString &)));

            mWidget->setAlignment (getAlignment(def.valueAlignment));
        }

        QWidget *widget()   { return mWidget; }

        void updateWidget (const QString &value)
        {
            if (mWidget->text() != value)
            mWidget->setText(value);
        }
    };

    /// list widget template
    /// \todo Not fully implemented.  Only widget supporting multi-valued settings
    template <>
    class SettingWidget <QListWidget>: public CSVSettings::AbstractWidget
    {

        QListWidget *mWidget;

    public:

        explicit SettingWidget(WidgetDef &def, QLayout *layout, QWidget *parent = 0 )
            : AbstractWidget (layout, parent), mWidget (new QListWidget (parent))
        {
            int i = 0;

            foreach (QString item, *(def.valueList))
            {
                mWidget->addItem (item);

                if (item == def.value) {}
                i++;
            }
            build (mWidget, def);
        }

        QWidget *widget()   { return mWidget; }

    private:
        void updateWidget (const QString &value) {}
    };

}
#endif // SETTINGWIDGET_HPP
