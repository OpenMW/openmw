
#include "scenetoolmode.hpp"

#include <QHBoxLayout>
#include <QFrame>
#include <QSignalMapper>

#include "scenetoolbar.hpp"
#include "pushbutton.hpp"

CSVWidget::SceneToolMode::SceneToolMode (SceneToolbar *parent)
: SceneTool (parent), mButtonSize (parent->getButtonSize()), mIconSize (parent->getIconSize())
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

    if (!mButtons.empty())
        mButtons.begin()->first->setFocus (Qt::OtherFocusReason);
}

void CSVWidget::SceneToolMode::addButton (const std::string& icon, const std::string& id)
{
    PushButton *button = new PushButton (QIcon (QPixmap (icon.c_str())), false, mPanel);
    button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setIconSize (QSize (mIconSize, mIconSize));
    button->setFixedSize (mButtonSize, mButtonSize);

    mLayout->addWidget (button);

    mButtons.insert (std::make_pair (button, id));

    connect (button, SIGNAL (clicked()), this, SLOT (selected()));

    if (mButtons.size()==1)
    {
        setIcon (button->icon());
        button->setChecked (true);
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
        emit modeChanged (iter->second);
    }
}