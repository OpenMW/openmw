
#include "scenetoolmode.hpp"

#include <QHBoxLayout>
#include <QFrame>
#include <QSignalMapper>

#include "scenetoolbar.hpp"
#include "pushbutton.hpp"

void CSVWidget::SceneToolMode::adjustToolTip (const PushButton *activeMode)
{
    QString toolTip = mToolTip;

    toolTip += "<p>Currently selected: " + activeMode->getBaseToolTip();

    toolTip += "<p>(left click to change mode)";

    setToolTip (toolTip);
}

CSVWidget::SceneToolMode::SceneToolMode (SceneToolbar *parent, const QString& toolTip)
: SceneTool (parent), mButtonSize (parent->getButtonSize()), mIconSize (parent->getIconSize()),
  mToolTip (toolTip), mFirst (0)
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
    PushButton *button = new PushButton (QIcon (QPixmap (icon.c_str())), PushButton::Type_Mode,
        tooltip, mPanel);
    button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setIconSize (QSize (mIconSize, mIconSize));
    button->setFixedSize (mButtonSize, mButtonSize);

    mLayout->addWidget (button);

    mButtons.insert (std::make_pair (button, id));

    connect (button, SIGNAL (clicked()), this, SLOT (selected()));

    if (mButtons.size()==1)
    {
        mFirst = button;
        setIcon (button->icon());
        button->setChecked (true);
        adjustToolTip (button);
    }
}

void CSVWidget::SceneToolMode::selected()
{
    std::map<PushButton *, std::string>::const_iterator iter =
        mButtons.find (dynamic_cast<PushButton *> (sender()));

    if (iter!=mButtons.end())
    {
        if (!iter->first->hasKeepOpen())
            mPanel->hide();

        for (std::map<PushButton *, std::string>::const_iterator iter2 = mButtons.begin();
            iter2!=mButtons.end(); ++iter2)
            iter2->first->setChecked (iter2==iter);

        setIcon (iter->first->icon());
        adjustToolTip (iter->first);
        emit modeChanged (iter->second);
    }
}