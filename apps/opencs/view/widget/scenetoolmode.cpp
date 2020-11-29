#include "scenetoolmode.hpp"

#include <QHBoxLayout>
#include <QFrame>
#include <QSignalMapper>
#include <QMenu>
#include <QContextMenuEvent>
#include <QEvent>

#include "scenetoolbar.hpp"
#include "modebutton.hpp"

void CSVWidget::SceneToolMode::contextMenuEvent (QContextMenuEvent *event)
{
    QMenu menu (this);
    if (createContextMenu (&menu))
        menu.exec (event->globalPos());
}

bool CSVWidget::SceneToolMode::createContextMenu (QMenu *menu)
{
    if (mCurrent)
        return mCurrent->createContextMenu (menu);

    return false;
}

void CSVWidget::SceneToolMode::adjustToolTip (const ModeButton *activeMode)
{
    QString toolTip = mToolTip;

    toolTip += "<p>Currently selected: " + activeMode->getBaseToolTip();

    toolTip += "<p>(left click to change mode)";

    if (createContextMenu (nullptr))
        toolTip += "<br>(right click to access context menu)";

    setToolTip (toolTip);
}

void CSVWidget::SceneToolMode::setButton (std::map<ModeButton *, std::string>::iterator iter)
{
    for (std::map<ModeButton *, std::string>::const_iterator iter2 = mButtons.begin();
        iter2!=mButtons.end(); ++iter2)
        iter2->first->setChecked (iter2==iter);

    setIcon (iter->first->icon());
    adjustToolTip (iter->first);

    if (mCurrent!=iter->first)
    {
        if (mCurrent)
            mCurrent->deactivate (mToolbar);

        mCurrent = iter->first;
        mCurrent->activate (mToolbar);
    }

    emit modeChanged (iter->second);
}

CSVWidget::SceneToolMode::SceneToolMode (SceneToolbar *parent, const QString& toolTip)
: SceneTool (parent), mButtonSize (parent->getButtonSize()), mIconSize (parent->getIconSize()),
  mToolTip (toolTip), mFirst (nullptr), mCurrent (nullptr), mToolbar (parent)
{
    mPanel = new QFrame (this, Qt::Popup);

    mLayout = new QHBoxLayout (mPanel);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    mPanel->setLayout (mLayout);
}

void CSVWidget::SceneToolMode::showPanel (const QPoint& position)
{
    mPanel->move (position);
    mPanel->show();

    if (mFirst)
        mFirst->setFocus (Qt::OtherFocusReason);
}

void CSVWidget::SceneToolMode::addButton (const std::string& icon, const std::string& id,
    const QString& tooltip)
{
    ModeButton *button = new ModeButton (QIcon (QPixmap (icon.c_str())), tooltip, mPanel);
    addButton (button, id);
}

void CSVWidget::SceneToolMode::addButton (ModeButton *button, const std::string& id)
{
    button->setParent (mPanel);

    button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setIconSize (QSize (mIconSize, mIconSize));
    button->setFixedSize (mButtonSize, mButtonSize);

    mLayout->addWidget (button);

    mButtons.insert (std::make_pair (button, id));

    connect (button, SIGNAL (clicked()), this, SLOT (selected()));

    if (mButtons.size()==1)
    {
        mFirst = mCurrent = button;
        setIcon (button->icon());
        button->setChecked (true);
        adjustToolTip (button);
        mCurrent->activate (mToolbar);
    }
}

CSVWidget::ModeButton *CSVWidget::SceneToolMode::getCurrent()
{
    return mCurrent;
}

std::string CSVWidget::SceneToolMode::getCurrentId() const
{
    return mButtons.find (mCurrent)->second;
}

void CSVWidget::SceneToolMode::setButton (const std::string& id)
{
    for (std::map<ModeButton *, std::string>::iterator iter = mButtons.begin();
        iter!=mButtons.end(); ++iter)
        if (iter->second==id)
        {
            setButton (iter);
            break;
        }
}

bool CSVWidget::SceneToolMode::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip)
    {
        adjustToolTip(mCurrent);
    }

    return SceneTool::event(event);
}

void CSVWidget::SceneToolMode::selected()
{
    std::map<ModeButton *, std::string>::iterator iter =
        mButtons.find (dynamic_cast<ModeButton *> (sender()));

    if (iter!=mButtons.end())
    {
        if (!iter->first->hasKeepOpen())
            mPanel->hide();

        setButton (iter);
    }
}
