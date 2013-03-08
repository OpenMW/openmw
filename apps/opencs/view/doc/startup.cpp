
#include "startup.hpp"

#include <QPushButton>
#include <QHBoxLayout>

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
}