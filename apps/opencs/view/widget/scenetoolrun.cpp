#include "scenetoolrun.hpp"

#include <iterator>

#include <components/misc/scalableicon.hpp>

#include <apps/opencs/view/widget/pushbutton.hpp>
#include <apps/opencs/view/widget/scenetool.hpp>

#include "scenetoollistpanel.hpp"

class QPoint;

namespace CSVWidget
{
    class SceneToolbar;
}

void CSVWidget::SceneToolRun::adjustToolTips()
{
    QString toolTip = mToolTip;

    if (mSelected == mProfiles.end())
        toolTip += "<p>No debug profile selected (function disabled)";
    else
    {
        toolTip += "<p>Debug profile: " + QString::fromStdString(*mSelected);
        toolTip += "<p>(right click to switch to a different profile)";
    }

    setToolTip(toolTip);
}

void CSVWidget::SceneToolRun::updateIcon()
{
    setDisabled(mSelected == mProfiles.end());
}

void CSVWidget::SceneToolRun::updatePanel()
{
    std::vector<QStringList> rows;
    rows.reserve(mProfiles.size());

    for (const std::string& profile : mProfiles)
        rows.push_back(QStringList{ QString::fromStdString(profile) });

    mPanel->setRows(rows);
}

CSVWidget::SceneToolRun::SceneToolRun(
    SceneToolbar* parent, const QString& toolTip, const QString& icon, const std::vector<std::string>& profiles)
    : SceneTool(parent, Type_TopAction)
    , mProfiles(profiles.begin(), profiles.end())
    , mSelected(mProfiles.begin())
    , mToolTip(toolTip)
    , mPanel(new SceneToolListPanel(1, this))
{
    setIcon(Misc::ScalableIcon::load(icon));
    updateIcon();
    adjustToolTips();

    connect(mPanel, &SceneToolListPanel::selectRequest, this, &SceneToolRun::selectRow);
    connect(mPanel, &SceneToolListPanel::removeRequest, this, &SceneToolRun::removeRow);
}

void CSVWidget::SceneToolRun::showPanel(const QPoint& position)
{
    updatePanel();

    mPanel->showAt(position);
}

void CSVWidget::SceneToolRun::activate()
{
    if (mSelected != mProfiles.end())
        emit runRequest(*mSelected);
}

void CSVWidget::SceneToolRun::removeProfile(const std::string& profile)
{
    std::set<std::string>::iterator iter = mProfiles.find(profile);

    if (iter != mProfiles.end())
    {
        if (iter == mSelected)
        {
            if (iter != mProfiles.begin())
                --mSelected;
            else
                ++mSelected;
        }

        mProfiles.erase(iter);

        if (mSelected == mProfiles.end())
            updateIcon();

        adjustToolTips();
    }
}

void CSVWidget::SceneToolRun::addProfile(const std::string& profile)
{
    std::set<std::string>::iterator iter = mProfiles.find(profile);

    if (iter == mProfiles.end())
    {
        mProfiles.insert(profile);

        if (mSelected == mProfiles.end())
        {
            mSelected = mProfiles.begin();
            updateIcon();
        }

        adjustToolTips();
    }
}

void CSVWidget::SceneToolRun::selectRow(int row)
{
    mSelected = mProfiles.begin();
    std::advance(mSelected, row);
    adjustToolTips();
}

void CSVWidget::SceneToolRun::removeRow(int row)
{
    std::set<std::string>::iterator iter = mProfiles.begin();
    std::advance(iter, row);
    removeProfile(*iter);
    updatePanel();
}
