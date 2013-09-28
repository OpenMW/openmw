
#include "scenetoolmode.hpp"

#include <QHBoxLayout>
#include <QFrame>
#include <QSignalMapper>

CSVWorld::SceneToolMode::SceneToolMode (QWidget *parent)
: SceneTool (parent)
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
    button->setFixedSize (48, 48);

    mLayout->addWidget (button);

    mButtons.insert (std::make_pair (button, id));

    connect (button, SIGNAL (clicked()), this, SLOT (selected()));
}

void CSVWorld::SceneToolMode::selected()
{
    std::map<QPushButton *, std::string>::const_iterator iter =
        mButtons.find (dynamic_cast<QPushButton *> (sender()));

    if (iter!=mButtons.end())
    {
        mPanel->hide();

        emit updateIcon (iter->first->icon());
        emit modeChanged (iter->second);
    }
}