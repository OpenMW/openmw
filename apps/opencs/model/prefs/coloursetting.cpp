
#include "coloursetting.hpp"

#include <QLabel>
#include <QMutexLocker>

#include <components/settings/settings.hpp>

#include "../../view/widget/coloreditor.hpp"

#include "category.hpp"
#include "state.hpp"

CSMPrefs::ColourSetting::ColourSetting (Category *parent, Settings::Manager *values,
  QMutex *mutex, const std::string& key, const std::string& label, QColor default_)
: Setting (parent, values, mutex, key, label), mDefault (default_)
{}

CSMPrefs::ColourSetting& CSMPrefs::ColourSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::ColourSetting::makeWidgets (QWidget *parent)
{
    QLabel *label = new QLabel (QString::fromUtf8 (getLabel().c_str()), parent);

    CSVWidget::ColorEditor *widget = new CSVWidget::ColorEditor (mDefault, parent);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        label->setToolTip (tooltip);
        widget->setToolTip (tooltip);
    }

    connect (widget, SIGNAL (pickingFinished()), this, SLOT (valueChanged()));

    return std::make_pair (label, widget);
}

void CSMPrefs::ColourSetting::valueChanged()
{
    CSVWidget::ColorEditor& widget = dynamic_cast<CSVWidget::ColorEditor&> (*sender());
    {
        QMutexLocker lock (getMutex());
        getValues().setString (getKey(), getParent()->getKey(), widget.color().name().toUtf8().data());
    }

    getParent()->getState()->update (*this);
}
