
#include "scenetoolrun.hpp"

#include <QFrame>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QApplication>

void CSVWidget::SceneToolRun::adjustToolTips()
{
    QString toolTip = mToolTip;

    if (mCurrentIndex==-1)
        toolTip += "<p>No debug profile selected (function disabled)";
    else
    {
        toolTip += "<p>Debug profile: " + QString::fromUtf8 (mProfiles[mCurrentIndex].c_str());
        toolTip += "<p>(right click to switch to a different profile)";
    }

    setToolTip (toolTip);
}

void CSVWidget::SceneToolRun::updateIcon()
{
    setIcon (QIcon (mCurrentIndex==-1 ? mIconDisabled : mIcon));
}

void CSVWidget::SceneToolRun::updatePanel()
{
    mTable->setRowCount (mProfiles.size());

    for (int i=0; i<static_cast<int> (mProfiles.size()); ++i)
    {
        mTable->setItem (i, 0, new QTableWidgetItem (QString::fromUtf8 (mProfiles[i].c_str())));

        mTable->setItem (i, 1, new QTableWidgetItem (
            QApplication::style()->standardIcon (QStyle::SP_TitleBarCloseButton), ""));
    }
}

CSVWidget::SceneToolRun::SceneToolRun (SceneToolbar *parent, const QString& toolTip,
    const QString& icon, const QString& iconDisabled, const std::vector<std::string>& profiles)
: SceneTool (parent, Type_TopAction), mProfiles (profiles),
  mCurrentIndex (profiles.empty() ? -1 : 0), mToolTip (toolTip), mIcon (icon),
  mIconDisabled (iconDisabled)
{
    updateIcon();
    adjustToolTips();

    mPanel = new QFrame (this, Qt::Popup);

    QHBoxLayout *layout = new QHBoxLayout (mPanel);

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    mTable = new QTableWidget (0, 2, this);

    mTable->setShowGrid (false);
    mTable->verticalHeader()->hide();
    mTable->horizontalHeader()->hide();
    mTable->horizontalHeader()->setResizeMode (0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setResizeMode (1, QHeaderView::ResizeToContents);
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

void CSVWidget::SceneToolRun::clicked (const QModelIndex& index)
{
    if (index.column()==0)
    {
        // select profile
        mCurrentIndex = index.row();
        mPanel->hide();
        adjustToolTips();
    }
    else if (index.column()==1)
    {
        // remove profile from list
        removeProfile (mProfiles.at (index.row()));
        updatePanel();
    }
}