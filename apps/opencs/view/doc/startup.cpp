
#include "startup.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRect>
#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QPushButton>

QPushButton *CSVDoc::StartupDialogue::addButton (const QString& label, const QIcon& icon)
{
    int column = mColumn--;

    QPushButton *button = new QPushButton (this);

    button->setIcon (QIcon (icon));

    button->setSizePolicy (QSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred));

    mLayout->addWidget (button, 0, column);

    mLayout->addWidget (new QLabel (label, this), 1, column, Qt::AlignCenter);

    int width = mLayout->itemAtPosition (1, column)->widget()->sizeHint().width();

    if (width>mWidth)
        mWidth = width;

    return button;
}


QWidget *CSVDoc::StartupDialogue::createButtons()
{
    QWidget *widget = new QWidget (this);

    mLayout = new QGridLayout (widget);

    /// \todo add icons
    QPushButton *loadDocument = addButton ("Edit A Content File", QIcon (":startup/edit-content"));
    connect (loadDocument, SIGNAL (clicked()), this, SIGNAL (loadDocument()));

    QPushButton *createAddon = addButton ("Create A New Addon", QIcon (":startup/create-addon"));
    connect (createAddon, SIGNAL (clicked()), this, SIGNAL (createAddon()));

    QPushButton *createGame = addButton ("Create A New Game", QIcon (":startup/create-game"));
    connect (createGame, SIGNAL (clicked()), this, SIGNAL (createGame()));

    for (int i=0; i<3; ++i)
        mLayout->setColumnMinimumWidth (i, mWidth);

    mLayout->setRowMinimumHeight (0, mWidth);

    mLayout->setSizeConstraint (QLayout::SetMinimumSize);
    mLayout->setHorizontalSpacing (32);

    mLayout->setContentsMargins (16, 16, 16, 8);

    loadDocument->setIconSize (QSize (mWidth, mWidth));
    createGame->setIconSize (QSize (mWidth, mWidth));
    createAddon->setIconSize (QSize (mWidth, mWidth));

    widget->setLayout (mLayout);

    return widget;
}

QWidget *CSVDoc::StartupDialogue::createTools()
{
    QWidget *widget = new QWidget (this);

    QHBoxLayout *layout = new QHBoxLayout (widget);
    layout->setDirection (QBoxLayout::RightToLeft);
    layout->setContentsMargins (4, 4, 4, 4);

    QPushButton *config = new QPushButton (widget);

    config->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    config->setIcon (QIcon (":startup/configure"));

    layout->addWidget (config);

    layout->addWidget (new QWidget, 1); // dummy widget; stops buttons from taking all the space

    widget->setLayout (layout);

    connect (config, SIGNAL (clicked()), this, SIGNAL (editConfig()));

    return widget;
}

CSVDoc::StartupDialogue::StartupDialogue() : mWidth (0), mColumn (2)
{
    setWindowTitle ("Open CS");

    QVBoxLayout *layout = new QVBoxLayout (this);

    layout->setContentsMargins (0, 0, 0, 0);

    layout->addWidget (createButtons());
    layout->addWidget (createTools());

    setLayout (layout);

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
}
