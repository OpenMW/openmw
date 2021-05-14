#include "scenetoolrun.hpp"

#include <iterator>

#include <QFrame>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QApplication>

void CSVWidget::SceneToolRun::adjustToolTips()
{
    QString toolTip = mToolTip;

    if (mSelected==mProfiles.end())
        toolTip += "<p>No debug profile selected (function disabled)";
    else
    {
        toolTip += "<p>Debug profile: " + QString::fromUtf8 (mSelected->c_str());
        toolTip += "<p>(right click to switch to a different profile)";
    }

    setToolTip (toolTip);
}

void CSVWidget::SceneToolRun::updateIcon()
{
    setDisabled (mSelected==mProfiles.end());
}

void CSVWidget::SceneToolRun::updatePanel()
{
    mTable->setRowCount (static_cast<int>(mProfiles.size()));

    int i = 0;

    for (std::set<std::string>::const_iterator iter (mProfiles.begin()); iter!=mProfiles.end();
        ++iter, ++i)
    {
        mTable->setItem (i, 0, new QTableWidgetItem (QString::fromUtf8 (iter->c_str())));

        mTable->setItem (i, 1, new QTableWidgetItem (
            QApplication::style()->standardIcon (QStyle::SP_TitleBarCloseButton), ""));
    }
}

CSVWidget::SceneToolRun::SceneToolRun (SceneToolbar *parent, const QString& toolTip,
    const QString& icon, const std::vector<std::string>& profiles)
: SceneTool (parent, Type_TopAction), mProfiles (profiles.begin(), profiles.end()),
  mSelected (mProfiles.begin()), mToolTip (toolTip)
{
    setIcon (QIcon (icon));
    updateIcon();
    adjustToolTips();

    mPanel = new QFrame (this, Qt::Popup);

    QHBoxLayout *layout = new QHBoxLayout (mPanel);

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    mTable = new QTableWidget (0, 2, this);

    mTable->setShowGrid (false);
    mTable->verticalHeader()->hide();
    mTable->horizontalHeader()->hide();
    mTable->horizontalHeader()->setSectionResizeMode (0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode (1, QHeaderView::ResizeToContents);
    mTable->setSelectionMode (QAbstractItemView::NoSelection);

    layout->addWidget (mTable);

    connect (mTable, SIGNAL (clicked (const QModelIndex&)),
        this, SLOT (clicked (const QModelIndex&)));
}

void CSVWidget::SceneToolRun::showPanel (const QPoint& position)
{
    updatePanel();

    mPanel->move (position);
    mPanel->show();
}

void CSVWidget::SceneToolRun::activate()
{
    if (mSelected!=mProfiles.end())
        emit runRequest (*mSelected);
}

void CSVWidget::SceneToolRun::removeProfile (const std::string& profile)
{
    std::set<std::string>::iterator iter = mProfiles.find (profile);

    if (iter!=mProfiles.end())
    {
        if (iter==mSelected)
        {
            if (iter!=mProfiles.begin())
                --mSelected;
            else
                ++mSelected;
        }

        mProfiles.erase (iter);

        if (mSelected==mProfiles.end())
            updateIcon();

        adjustToolTips();
    }
}

void CSVWidget::SceneToolRun::addProfile (const std::string& profile)
{
    std::set<std::string>::iterator iter = mProfiles.find (profile);

    if (iter==mProfiles.end())
    {
        mProfiles.insert (profile);

        if (mSelected==mProfiles.end())
        {
            mSelected = mProfiles.begin();
            updateIcon();
        }

        adjustToolTips();
    }
}

void CSVWidget::SceneToolRun::clicked (const QModelIndex& index)
{
    if (index.column()==0)
    {
        // select profile
        mSelected = mProfiles.begin();
        std::advance (mSelected, index.row());
        mPanel->hide();
        adjustToolTips();
    }
    else if (index.column()==1)
    {
        // remove profile from list
        std::set<std::string>::iterator iter = mProfiles.begin();
        std::advance (iter, index.row());
        removeProfile (*iter);
        updatePanel();
    }
}
