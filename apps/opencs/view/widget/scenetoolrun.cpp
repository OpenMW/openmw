
#include "scenetoolrun.hpp"

void CSVWidget::SceneToolRun::adjustToolTips()
{
    QString toolTip = mToolTip;

    if (mCurrentIndex==-1)
        toolTip += "<p>No debug profile selected (function disabled)";
    else
        toolTip += "<p>Debug profile: " + QString::fromUtf8 (mProfiles[mCurrentIndex].c_str());

    setToolTip (toolTip);
}

void CSVWidget::SceneToolRun::updateIcon()
{
    setIcon (QIcon (mCurrentIndex==-1 ? mIconDisabled : mIcon));
}

CSVWidget::SceneToolRun::SceneToolRun (SceneToolbar *parent, const QString& toolTip,
    const QString& icon, const QString& iconDisabled, const std::vector<std::string>& profiles)
: SceneTool (parent, Type_TopAction), mProfiles (profiles),
  mCurrentIndex (profiles.empty() ? -1 : 0), mToolTip (toolTip), mIcon (icon),
  mIconDisabled (iconDisabled)
{
    updateIcon();
    adjustToolTips();
}

void CSVWidget::SceneToolRun::showPanel (const QPoint& position)
{
    if (mCurrentIndex!=-1)
        emit runRequest (mProfiles[mCurrentIndex]);
}

void CSVWidget::SceneToolRun::removeProfile (const std::string& profile)
{
    std::pair<std::vector<std::string>::iterator, std::vector<std::string>::iterator>
        result = std::equal_range (mProfiles.begin(), mProfiles.end(), profile);

    if (result.first!=result.second)
    {
        mProfiles.erase (result.first);

        if (mCurrentIndex>=static_cast<int> (mProfiles.size()))
            --mCurrentIndex;

        if (mCurrentIndex==-1)
            updateIcon();

        adjustToolTips();
    }
}