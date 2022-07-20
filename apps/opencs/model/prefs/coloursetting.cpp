#include "coloursetting.hpp"

#include <QLabel>
#include <QMutexLocker>
#include <QString>

#include <components/settings/settings.hpp>

#include "../../view/widget/coloreditor.hpp"

#include "category.hpp"
#include "state.hpp"

CSMPrefs::ColourSetting::ColourSetting (Category *parent,
  QMutex *mutex, const std::string& key, const std::string& label, QColor default_)
: Setting (parent, mutex, key, label), mDefault (default_), mWidget(nullptr)
{}

CSMPrefs::ColourSetting& CSMPrefs::ColourSetting::setTooltip (const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

std::pair<QWidget *, QWidget *> CSMPrefs::ColourSetting::makeWidgets (QWidget *parent)
{
    QLabel *label = new QLabel (QString::fromUtf8 (getLabel().c_str()), parent);

    mWidget = new CSVWidget::ColorEditor (mDefault, parent);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8 (mTooltip.c_str());
        label->setToolTip (tooltip);
        mWidget->setToolTip (tooltip);
    }

    connect (mWidget, SIGNAL (pickingFinished()), this, SLOT (valueChanged()));

    return std::make_pair (label, mWidget);
}

void CSMPrefs::ColourSetting::updateWidget()
{
    if (mWidget)
    {
        mWidget->setColor(QString::fromStdString
            (Settings::Manager::getString(getKey(), getParent()->getKey())));
    }
}

void CSMPrefs::ColourSetting::valueChanged()
{
    CSVWidget::ColorEditor& widget = dynamic_cast<CSVWidget::ColorEditor&> (*sender());
    {
        QMutexLocker lock (getMutex());
        Settings::Manager::setString (getKey(), getParent()->getKey(), widget.color().name().toUtf8().data());
    }

    getParent()->getState()->update (*this);
}
