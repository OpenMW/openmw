
#include "genericcreator.hpp"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QUndoStack>

#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"

#include "idvalidator.hpp"

void CSVWorld::GenericCreator::update()
{
    mErrors = getErrors();

    mCreate->setToolTip (QString::fromUtf8 (mErrors.c_str()));
    mId->setToolTip (QString::fromUtf8 (mErrors.c_str()));

    mCreate->setEnabled (mErrors.empty() && !mLocked);
}

CSVWorld::GenericCreator::GenericCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id)
: mData (data), mUndoStack (undoStack), mListId (id), mLocked (false)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins (0, 0, 0, 0);

    mId = new QLineEdit;
    mId->setValidator (new IdValidator (this));
    layout->addWidget (mId, 1);

    mCreate = new QPushButton ("Create");
    layout->addWidget (mCreate);

    QPushButton *cancelButton = new QPushButton ("Cancel");
    layout->addWidget (cancelButton);

    setLayout (layout);

    connect (cancelButton, SIGNAL (clicked (bool)), this, SIGNAL (done()));
    connect (mCreate, SIGNAL (clicked (bool)), this, SLOT (create()));

    connect (mId, SIGNAL (textChanged (const QString&)), this, SLOT (textChanged (const QString&)));
}

void CSVWorld::GenericCreator::setEditLock (bool locked)
{
    mLocked = locked;
    update();
}

void CSVWorld::GenericCreator::reset()
{
    mId->setText ("");
    update();
}

std::string CSVWorld::GenericCreator::getErrors() const
{
    std::string errors;

    std::string id = mId->text().toUtf8().constData();

    if (id.empty())
    {
        errors = "Missing ID";
    }
    else if (mData.hasId (id))
    {
        errors = "ID is already in use";
    }

    return errors;
}

void CSVWorld::GenericCreator::textChanged (const QString& text)
{
    update();
}

void CSVWorld::GenericCreator::create()
{
    if (!mLocked)
    {
        mUndoStack.push (new CSMWorld::CreateCommand (
            dynamic_cast<CSMWorld::IdTable&> (*mData.getTableModel (mListId)),
            mId->text().toUtf8().constData()));

        emit done();
    }
}