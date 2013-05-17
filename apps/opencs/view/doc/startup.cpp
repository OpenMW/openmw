
#include "startup.hpp"

#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRect>

CSVDoc::StartupDialogue::StartupDialogue()
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    QPushButton *createDocument = new QPushButton ("new", this);
    connect (createDocument, SIGNAL (clicked()), this, SIGNAL (createDocument()));
    layout->addWidget (createDocument);

    QPushButton *loadDocument = new QPushButton ("load", this);
    connect (loadDocument, SIGNAL (clicked()), this, SIGNAL (loadDocument()));
    layout->addWidget (loadDocument);

    setLayout (layout);

    QRect scr = QApplication::desktop()->screenGeometry();
    QRect rect = geometry();
    move (scr.center().x() - rect.center().x(), scr.center().y() - rect.center().y());
}
