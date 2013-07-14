#include "abstractwidget.hpp"

#include <QLayout>
#include <QLabel>

void CSVSettings::AbstractWidget::build(QWidget *widget, WidgetDef &def, bool noLabel)
{
    if (!mLayout)
        createLayout(def.orientation, true);

    buildLabelAndWidget (widget, def, noLabel);

}

void CSVSettings::AbstractWidget::buildLabelAndWidget (QWidget *widget, WidgetDef &def, bool noLabel)
{
    if (def.widgetWidth > -1)
        widget->setFixedWidth (def.widgetWidth);

    if (!(def.caption.isEmpty() || noLabel) )
    {
        QLabel *label = new QLabel (def.caption, &dynamic_cast<QWidget &>( *parent()));
        label->setBuddy (widget);
        mLayout->addWidget (label);

        if (def.labelWidth > -1)
            label->setFixedWidth(def.labelWidth);
    }

    mLayout->addWidget (widget);
    mLayout->setAlignment (widget, getAlignment (def.widgetAlignment));
}

void CSVSettings::AbstractWidget::createLayout
        (Orientation direction, bool isZeroMargin)
{
    if (direction == Orient_Vertical)
        mLayout = new QVBoxLayout ();
    else
        mLayout = new QHBoxLayout ();

    if (isZeroMargin)
        mLayout->setContentsMargins(0, 0, 0, 0);
}

QFlags<Qt::AlignmentFlag> CSVSettings::AbstractWidget::getAlignment (CSVSettings::Alignment flag)
{
    return QFlags<Qt::AlignmentFlag>(static_cast<int>(flag));
}

QLayout *CSVSettings::AbstractWidget::getLayout()
{
    return mLayout;
}

void CSVSettings::AbstractWidget::slotUpdateWidget (const QString &value)
{
    updateWidget (value);
}

void CSVSettings::AbstractWidget::slotUpdateItem(const QString &value)
{
    emit signalUpdateItem (value);
}

void CSVSettings::AbstractWidget::slotUpdateItem(bool value)
{
    if (value)
        emit signalUpdateItem (widget()->objectName());
}

void CSVSettings::AbstractWidget::slotUpdateItem(int value)
{
    emit signalUpdateItem (QString::number(value));
}

void CSVSettings::AbstractWidget::slotUpdateItem (QListWidgetItem* current, QListWidgetItem* previous)
{}
