#include "scenetooltoggle.hpp"

#include <stdexcept>

#include <QHBoxLayout>
#include <QFrame>
#include <QIcon>
#include <QPainter>

#include "scenetoolbar.hpp"
#include "pushbutton.hpp"

void CSVWidget::SceneToolToggle::adjustToolTip()
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

void CSVWidget::SceneToolToggle::adjustIcon()
{
    unsigned int selection = getSelectionMask();
    if (!selection)
        setIcon (QIcon (QString::fromUtf8 (mEmptyIcon.c_str())));
    else
    {
        QPixmap pixmap (48, 48);
        pixmap.fill (QColor (0, 0, 0, 0));

        {
            QPainter painter (&pixmap);

            for (std::map<PushButton *, ButtonDesc>::const_iterator iter (mButtons.begin());
                iter!=mButtons.end(); ++iter)
                if (iter->first->isChecked())
                {
                    painter.drawImage (getIconBox (iter->second.mIndex),
                        QImage (QString::fromUtf8 (iter->second.mSmallIcon.c_str())));
                }
        }

        setIcon (pixmap);
    }
}

QRect CSVWidget::SceneToolToggle::getIconBox (int index) const
{
    // layout for a 3x3 grid
    int xMax = 3;
    int yMax = 3;

    // icon size
    int xBorder = 1;
    int yBorder = 1;

    int iconXSize = (mIconSize-xBorder*(xMax+1))/xMax;
    int iconYSize = (mIconSize-yBorder*(yMax+1))/yMax;

    int y = index / xMax;
    int x = index % xMax;

    int total = static_cast<int>(mButtons.size());

    int actualYIcons = total/xMax;

    if (total % xMax)
        ++actualYIcons;

    if (actualYIcons!=yMax)
    {
        // space out icons vertically, if there aren't enough to populate all rows
        int diff = yMax - actualYIcons;
        yBorder += (diff*(yBorder+iconXSize)) / (actualYIcons+1);
    }

    if (y==actualYIcons-1)
    {
        // generating the last row of icons
        int actualXIcons = total % xMax;

        if (actualXIcons)
        {
            // space out icons horizontally, if there aren't enough to fill the last row
            int diff = xMax - actualXIcons;

            xBorder += (diff*(xBorder+iconXSize)) / (actualXIcons+1);
        }
    }

    return QRect ((iconXSize+xBorder)*x+xBorder, (iconYSize+yBorder)*y+yBorder,
        iconXSize, iconYSize);
}

CSVWidget::SceneToolToggle::SceneToolToggle (SceneToolbar *parent, const QString& toolTip,
    const std::string& emptyIcon)
: SceneTool (parent), mEmptyIcon (emptyIcon), mButtonSize (parent->getButtonSize()),
  mIconSize (parent->getIconSize()), mToolTip (toolTip), mFirst (nullptr)
{
    mPanel = new QFrame (this, Qt::Popup);

    mLayout = new QHBoxLayout (mPanel);

    mLayout->setContentsMargins (QMargins (0, 0, 0, 0));

    mPanel->setLayout (mLayout);
}

void CSVWidget::SceneToolToggle::showPanel (const QPoint& position)
{
    mPanel->move (position);
    mPanel->show();

    if (mFirst)
        mFirst->setFocus (Qt::OtherFocusReason);
}

void CSVWidget::SceneToolToggle::addButton (const std::string& icon, unsigned int mask,
    const std::string& smallIcon, const QString& name, const QString& tooltip)
{
    if (mButtons.size()>=9)
        throw std::runtime_error ("Exceeded number of buttons in toggle type tool");

    PushButton *button = new PushButton (QIcon (QPixmap (icon.c_str())),
        PushButton::Type_Toggle, tooltip.isEmpty() ? name: tooltip, mPanel);

    button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setIconSize (QSize (mIconSize, mIconSize));
    button->setFixedSize (mButtonSize, mButtonSize);

    mLayout->addWidget (button);

    ButtonDesc desc;
    desc.mMask = mask;
    desc.mSmallIcon = smallIcon;
    desc.mName = name;
    desc.mIndex = static_cast<int>(mButtons.size());

    mButtons.insert (std::make_pair (button, desc));

    connect (button, SIGNAL (clicked()), this, SLOT (selected()));

    if (mButtons.size()==1)
        mFirst = button;
}

unsigned int CSVWidget::SceneToolToggle::getSelectionMask() const
{
    unsigned int selection = 0;

    for (std::map<PushButton *, ButtonDesc>::const_iterator iter (mButtons.begin());
        iter!=mButtons.end(); ++iter)
        if (iter->first->isChecked())
            selection |= iter->second.mMask;

    return selection;
}

void CSVWidget::SceneToolToggle::setSelectionMask (unsigned int selection)
{
    for (std::map<PushButton *, ButtonDesc>::iterator iter (mButtons.begin());
        iter!=mButtons.end(); ++iter)
        iter->first->setChecked (selection & iter->second.mMask);

    adjustToolTip();
    adjustIcon();
}

void CSVWidget::SceneToolToggle::selected()
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
