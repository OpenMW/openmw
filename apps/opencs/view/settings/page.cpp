#include "page.hpp"

#include <QLabel>

#include "view.hpp"
#include "booleanview.hpp"
#include "textview.hpp"
#include "listview.hpp"
#include "rangeview.hpp"

#include "../../model/settings/usersettings.hpp"
#include "../../model/settings/connector.hpp"
#include "../../model/settings/support.hpp"

#include "settingwindow.hpp"

QMap <CSVSettings::ViewType, CSVSettings::IViewFactory *>
                                            CSVSettings::Page::mViewFactories;

CSVSettings::Page::Page (const QString &pageName, QList <CSMSettings::Setting *> settingList,
    SettingWindow *parent, const QString& label)
: mParent(parent), mIsEditorPage (false), Frame(false, "", parent), mLabel (label)
{
    setObjectName (pageName);

    if (mViewFactories.size() == 0)
        buildFactories();

    setVLayout();
    setupViews (settingList);
}

void CSVSettings::Page::setupViews
                                (QList <CSMSettings::Setting *> &settingList)
{
    foreach (CSMSettings::Setting *setting, settingList)
        addView (setting);
}

void CSVSettings::Page::addView (CSMSettings::Setting *setting)
{
    if (setting->viewType() == ViewType_Undefined)
    {
        if(setting->specialValueText() != "")
        {
            // hack to put a label
            addWidget(new QLabel(setting->specialValueText()),
                setting->viewRow(), setting->viewColumn(),
                setting->rowSpan(), setting->columnSpan());
            return;
        }
        else
            return;
    }

    View *view = mViewFactories[setting->viewType()]->createView(setting, this);

    if (!view)
        return;

    mViews.append (view);

    addWidget (view, setting->viewRow(), setting->viewColumn(),
               setting->rowSpan(), setting->columnSpan() );

    //if this page is an editor page, connect each of it's views up to the
    //UserSettings singleton for signaling back to OpenCS
    if (setting->isEditorSetting()) {
        connect (view, SIGNAL (viewUpdated(const QString&, const QStringList&)),
                 &CSMSettings::UserSettings::instance(),
                 SLOT (updateUserSetting (const QString &, const QStringList &)));
    }
}

CSVSettings::View *CSVSettings::Page::findView (const QString &page,
                                                const QString &setting) const
{

    //if this is not the page we're looking for,
    //appeal to the parent setting window to find the appropriate view
    if (page != objectName())
        return mParent->findView (page, setting);

    //otherwise, return the matching view
    for (int i = 0; i < mViews.size(); i++)
    {
        View *view = mViews.at(i);

        if (view->parentPage()->objectName() != page)
            continue;

        if (view->objectName() == setting)
            return view;
    }

    return 0;
}

void CSVSettings::Page::buildFactories()
{
    mViewFactories[ViewType_Boolean] = new BooleanViewFactory (this);
    mViewFactories[ViewType_Text] = new TextViewFactory (this);
    mViewFactories[ViewType_List] = new ListViewFactory (this);
    mViewFactories[ViewType_Range] = new RangeViewFactory (this);
}

QString CSVSettings::Page::getLabel() const
{
    return mLabel;
}
