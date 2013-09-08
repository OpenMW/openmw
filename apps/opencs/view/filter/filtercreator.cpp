
#include "filtercreator.hpp"

#include <QComboBox>
#include <QLabel>

#include "../../model/filter/filter.hpp"

std::string CSVFilter::FilterCreator::getNamespace() const
{
    switch (mScope->currentIndex())
    {
        case CSMFilter::Filter::Scope_Project: return "project::";
        case CSMFilter::Filter::Scope_Session: return "session::";
    }

    return "";
}

void CSVFilter::FilterCreator::update()
{
    mNamespace->setText (QString::fromUtf8 (getNamespace().c_str()));
    GenericCreator::update();
}

std::string CSVFilter::FilterCreator::getId() const
{
    return getNamespace() + GenericCreator::getId();
}

CSVFilter::FilterCreator::FilterCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id)
: GenericCreator (data, undoStack, id)
{
    mNamespace = new QLabel ("::", this);
    insertAtBeginning (mNamespace, false);

    mScope = new QComboBox (this);

    mScope->addItem ("Project");
    mScope->addItem ("Session");
    /// \ŧodo re-enable for OpenMW 1.1
    // mScope->addItem ("Content");

    connect (mScope, SIGNAL (currentIndexChanged (int)), this, SLOT (setScope (int)));

    insertAtBeginning (mScope, false);

    QLabel *label = new QLabel ("Scope", this);
    insertAtBeginning (label, false);

    mScope->setCurrentIndex (1);
}

void CSVFilter::FilterCreator::reset()
{
    GenericCreator::reset();
}

void CSVFilter::FilterCreator::setScope (int index)
{
    update();
}
