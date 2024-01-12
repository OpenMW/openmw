#include "coloursetting.hpp"

#include <QLabel>
#include <QMutexLocker>
#include <QString>

#include <components/settings/settings.hpp>

#include <apps/opencs/model/prefs/setting.hpp>

#include "../../view/widget/coloreditor.hpp"

#include "category.hpp"
#include "state.hpp"

CSMPrefs::ColourSetting::ColourSetting(
    Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index)
    : TypedSetting(parent, mutex, key, label, index)
    , mWidget(nullptr)
{
}

CSMPrefs::ColourSetting& CSMPrefs::ColourSetting::setTooltip(const std::string& tooltip)
{
    mTooltip = tooltip;
    return *this;
}

CSMPrefs::SettingWidgets CSMPrefs::ColourSetting::makeWidgets(QWidget* parent)
{
    QLabel* label = new QLabel(getLabel(), parent);

    mWidget = new CSVWidget::ColorEditor(toColor(), parent);

    if (!mTooltip.empty())
    {
        QString tooltip = QString::fromUtf8(mTooltip.c_str());
        label->setToolTip(tooltip);
        mWidget->setToolTip(tooltip);
    }

    connect(mWidget, &CSVWidget::ColorEditor::pickingFinished, this, &ColourSetting::valueChanged);

    return SettingWidgets{ .mLabel = label, .mInput = mWidget };
}

void CSMPrefs::ColourSetting::updateWidget()
{
    if (mWidget)
        mWidget->setColor(toColor());
}

void CSMPrefs::ColourSetting::valueChanged()
{
    CSVWidget::ColorEditor& widget = dynamic_cast<CSVWidget::ColorEditor&>(*sender());
    setValue(widget.color().name().toStdString());
    getParent()->getState()->update(*this);
}
