#include "scenetooltoggle2.hpp"

#include <stdexcept>
#include <sstream>

#include <QHBoxLayout>
#include <QFrame>
#include <QIcon>
#include <QPainter>

#include "scenetoolbar.hpp"
#include "pushbutton.hpp"

void CSVWidget::SceneToolToggle2::adjustToolTip()
{
    QString toolTip = mToolTip;

    toolTip += "<p>Currently enabled: ";

    bool first = true;

    for (std::map<PushButton *, ButtonDesc>::const_iterator iter (mButtons.begin());
        iter!=mButtons.end(); ++iter)
        if (iter->first->isChecked())
        {
            if (!first)
                toolTip += ", ";
            else
                first = false;

            toolTip += iter->second.mName;
        }

    if (first)
        toolTip += "none";

    toolTip += "<p>(left click to alter selection)";

    setToolTip (toolTip);
}

void CSVWidget::SceneToolToggle2::adjustIcon()
{
    std::ostringstream stream;
    stream << mCompositeIcon << getSelection();
    setIcon (QIcon (QString::fromUtf8 (stream.str().c_str())));
}

CSVWidget::SceneToolToggle2::SceneToolToggle2 (SceneToolbar *parent, const QString& toolTip,
    const std::string& compositeIcon, const std::string& singleIcon)
: SceneTool (parent), mCompositeIcon (compositeIcon), mSingleIcon (singleIcon),
  mButtonSize (parent->getButtonSize()), mIconSize (parent->getIconSize()), mToolTip (toolTip),
  mFirst (0)
{
    mPanel = new QFrame (this, Qt::Popup);

    mLayout = new QHBoxLayout (mPanel);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    mPanel->setLayout (mLayout);
}

void CSVWidget::SceneToolToggle2::showPanel (const QPoint& position)
{
    mPanel->move (position);
    mPanel->show();

    if (mFirst)
        mFirst->setFocus (Qt::OtherFocusReason);
}

void CSVWidget::SceneToolToggle2::addButton (unsigned int id,
    const QString& name, const QString& tooltip, bool disabled)
{
    std::ostringstream stream;
    stream << mSingleIcon << id;

    PushButton *button = new PushButton (QIcon (QPixmap (stream.str().c_str())),
        PushButton::Type_Toggle, tooltip.isEmpty() ? name: tooltip, mPanel);

    button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setIconSize (QSize (mIconSize, mIconSize));
    button->setFixedSize (mButtonSize, mButtonSize);

    if (disabled)
        button->setDisabled (true);

    mLayout->addWidget (button);

    ButtonDesc desc;
    desc.mId = id;
    desc.mName = name;
    desc.mIndex = mButtons.size();

    mButtons.insert (std::make_pair (button, desc));

    connect (button, SIGNAL (clicked()), this, SLOT (selected()));

    if (mButtons.size()==1 && !disabled)
        mFirst = button;
}

unsigned int CSVWidget::SceneToolToggle2::getSelection() const
{
    unsigned int selection = 0;

    for (std::map<PushButton *, ButtonDesc>::const_iterator iter (mButtons.begin());
        iter!=mButtons.end(); ++iter)
        if (iter->first->isChecked())
            selection |= iter->second.mId;

    return selection;
}

void CSVWidget::SceneToolToggle2::setSelection (unsigned int selection)
{
    for (std::map<PushButton *, ButtonDesc>::iterator iter (mButtons.begin());
        iter!=mButtons.end(); ++iter)
        iter->first->setChecked (selection & iter->second.mId);

    adjustToolTip();
    adjustIcon();
}

void CSVWidget::SceneToolToggle2::selected()
{
    std::map<PushButton *, ButtonDesc>::const_iterator iter =
        mButtons.find (dynamic_cast<PushButton *> (sender()));

    if (iter!=mButtons.end())
    {
        if (!iter->first->hasKeepOpen())
            mPanel->hide();

        adjustToolTip();
        adjustIcon();

        emit selectionChanged();
    }
}
