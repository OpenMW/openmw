#include "abstractwidget.hpp"

#include <QLayout>
#include <QLabel>

void CsSettings::AbstractWidget::build(QWidget *widget, WidgetDef &def, bool noLabel)
{
    if (!mLayout)
        createLayout(def.orientation, true);

    buildLabelAndWidget (widget, def, noLabel);

}

void CsSettings::AbstractWidget::buildLabelAndWidget (QWidget *widget, WidgetDef &def, bool noLabel)
{
    if (def.widgetWidth > -1)
        widget->setFixedWidth (def.widgetWidth);

    if (!(def.caption.isEmpty() || noLabel) )
    {
        QLabel *label = new QLabel (def.caption, dynamic_cast<QWidget*>(parent()));
        label->setBuddy (widget);
        mLayout->addWidget (label);

        if (def.labelWidth > -1)
            label->setFixedWidth(def.labelWidth);
    }

    mLayout->addWidget (widget);
    mLayout->setAlignment (widget, getAlignment (def.widgetAlignment));
}

void CsSettings::AbstractWidget::createLayout
        (OcsWidgetOrientation direction, bool isZeroMargin)
{
    if (direction == OCS_VERTICAL)
        mLayout = new QVBoxLayout ();
    else
        mLayout = new QHBoxLayout ();

    if (isZeroMargin)
        mLayout->setContentsMargins(0, 0, 0, 0);
}

QFlags<Qt::AlignmentFlag> CsSettings::AbstractWidget::getAlignment (CsSettings::OcsAlignment flag)
{
    return QFlags<Qt::AlignmentFlag>(static_cast<int>(flag));
}

QLayout *CsSettings::AbstractWidget::getLayout()
{
    return mLayout;
}

void CsSettings::AbstractWidget::slotUpdateWidget (const QString &value)
{
    updateWidget (value);
}

void CsSettings::AbstractWidget::slotUpdateItem(const QString &value)
{
    emit signalUpdateItem (value);
}

void CsSettings::AbstractWidget::slotUpdateItem(bool value)
{
    if (value)
        emit signalUpdateItem (widget()->objectName());
}

void CsSettings::AbstractWidget::slotUpdateItem(int value)
{
    emit signalUpdateItem (QString::number(value));
}

void CsSettings::AbstractWidget::slotUpdateItem (QListWidgetItem* current, QListWidgetItem* previous)
{}
