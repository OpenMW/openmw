
#include "genericcreator.hpp"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>

CSVWorld::GenericCreator::GenericCreator()
{
    QHBoxLayout *layout = new QHBoxLayout;

    QLineEdit *name = new QLineEdit;
    layout->addWidget (name, 1);

    QPushButton *createButton = new QPushButton ("Create");
    layout->addWidget (createButton);

    QPushButton *cancelButton = new QPushButton ("Cancel");
    layout->addWidget (cancelButton);

    connect (cancelButton, SIGNAL (clicked (bool)), this, SIGNAL (done()));

    setLayout (layout);
}

void CSVWorld::GenericCreator::reset()
{

}