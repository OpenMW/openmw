
#include "genericcreator.hpp"

#include <memory>

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

void CSVWorld::GenericCreator::setManualEditing (bool enabled)
{
    mId->setVisible (enabled);
}

void CSVWorld::GenericCreator::insertAtBeginning (QWidget *widget, bool stretched)
{
    mLayout->insertWidget (0, widget, stretched ? 1 : 0);
}

void CSVWorld::GenericCreator::insertBeforeButtons (QWidget *widget, bool stretched)
{
    mLayout->insertWidget (mLayout->count()-2, widget, stretched ? 1 : 0);
}

std::string CSVWorld::GenericCreator::getId() const
{
    return mId->text().toUtf8().constData();
}

void CSVWorld::GenericCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const {}

CSMWorld::Data& CSVWorld::GenericCreator::getData() const
{
    return mData;
}

const CSMWorld::UniversalId& CSVWorld::GenericCreator::getCollectionId() const
{
    return mListId;
}

CSVWorld::GenericCreator::GenericCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id, bool relaxedIdRules):

    mData (data),
    mUndoStack (undoStack),
    mListId (id),
    mLocked (false),
    mCloneMode(false),
    mClonedType(CSMWorld::UniversalId::Type_None)

{
    mLayout = new QHBoxLayout;
    mLayout->setContentsMargins (0, 0, 0, 0);

    mId = new QLineEdit;
    mId->setValidator (new IdValidator (relaxedIdRules, this));
    mLayout->addWidget (mId, 1);

    mCreate = new QPushButton ("Create");
    mLayout->addWidget (mCreate);

    QPushButton *cancelButton = new QPushButton ("Cancel");
    mLayout->addWidget (cancelButton);

    setLayout (mLayout);

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
    mCloneMode = false;
    mId->setText ("");
    update();
}

std::string CSVWorld::GenericCreator::getErrors() const
{
    std::string errors;

    std::string id = getId();

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
        if (mCloneMode)
        {
            std::string id = getId();
            std::auto_ptr<CSMWorld::CloneCommand> command (new CSMWorld::CloneCommand (
                dynamic_cast<CSMWorld::IdTable&> (*mData.getTableModel(mListId)), mClonedId, id, mClonedType));

            mUndoStack.push(command.release());

            emit done();
            emit requestFocus(id);
        } else {
            std::string id = getId();

            std::auto_ptr<CSMWorld::CreateCommand> command (new CSMWorld::CreateCommand (
            dynamic_cast<CSMWorld::IdTable&> (*mData.getTableModel (mListId)), id));

            configureCreateCommand (*command);

            mUndoStack.push (command.release());

            emit done();
            emit requestFocus (id);
        }
    }
}

void CSVWorld::GenericCreator::cloneMode(const std::string& originId,
                                         const CSMWorld::UniversalId::Type type)
{
    mCloneMode = true;
    mClonedId = originId;
    mClonedType = type;
}

void CSVWorld::GenericCreator::toggleWidgets(bool active)
{
}
