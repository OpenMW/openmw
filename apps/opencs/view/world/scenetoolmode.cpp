
#include "scenetoolmode.hpp"

#include <QHBoxLayout>
#include <QFrame>
#include <QSignalMapper>

#include "scenetoolbar.hpp"

CSVWorld::SceneToolMode::SceneToolMode (SceneToolbar *parent)
: SceneTool (parent), mButtonSize (parent->getButtonSize()), mIconSize (parent->getIconSize())
{
    mPanel = new QFrame (this, Qt::Popup);

    mLayout = new QHBoxLayout (mPanel);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    mPanel->setLayout (mLayout);
}

void CSVWorld::SceneToolMode::showPanel (const QPoint& position)
{
    mPanel->move (position);
    mPanel->show();
}

void CSVWorld::SceneToolMode::addButton (const std::string& icon, const std::string& id)
{
    QPushButton *button = new QPushButton (QIcon (QPixmap (icon.c_str())), "", mPanel);
    button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setIconSize (QSize (mIconSize, mIconSize));
    button->setFixedSize (mButtonSize, mButtonSize);

    mLayout->addWidget (button);

    mButtons.insert (std::make_pair (button, id));

    connect (button, SIGNAL (clicked()), this, SLOT (selected()));

    if (mButtons.size()==1)
        setIcon (button->icon());
}

void CSVWorld::SceneToolMode::selected()
{
    std::map<QPushButton *, std::string>::const_iterator iter =
        mButtons.find (dynamic_cast<QPushButton *> (sender()));

    if (iter!=mButtons.end())
    {
        mPanel->hide();

        setIcon (iter->first->icon());
        emit modeChanged (iter->second);
    }
}