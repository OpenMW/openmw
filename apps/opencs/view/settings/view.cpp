#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>

#include "view.hpp"
#include "support.hpp"
#include "../../model/settings/setting.hpp"
#include "settingbox.hpp"
#include "../../model/settings/selector.hpp"
#include "page.hpp"

#include <QDebug>
CSVSettings::View::View(CSMSettings::Setting *setting,
                        Page *parent)

    : mSetting (setting), mSelector (0), QWidget(parent)
{
    mViewFrame = new SettingBox(true, setting->name(), parent);
    mViewFrame->setFlat (true);
    mViewFrame->setHLayout();

    setObjectName (setting->page() + '.' + setting->name() + '.' + "View");
}

void CSVSettings::View::showEvent ( QShowEvent * event )
{
    Page *parentPage = static_cast<Page *> (parent());

    if (!mSelector)
    {
        mSelector = parentPage->selector(mSetting->name());

        connect (mSelector, SIGNAL (modelUpdate (QStringList)),
                this, SLOT (slotUpdateView (QStringList)));

    }

    qDebug () << objectName() << "::View::showEvent() connection complete";

    if (mSetting->proxyLists().size() > 0)
        parentPage->buildProxy (mSetting, mSelector);

    qDebug () << objectName() << "::View::showEvent() proxy constructed";

    if (mSelector->proxyIndex() != 0)
    {
        qDebug () << objectName() << "::View::showEvent() selector about to refresh...";
        mSelector->refresh();
    }

    qDebug () << objectName() << "::View::showEvent() selector refreshed--------------------------";
}
