
#include "genericcreator.hpp"

#include <memory>

#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QUndoStack>
#include <QLabel>
#include <QComboBox>

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

    if (mNamespace)
        mNamespace->setText (QString::fromUtf8 (getNamespace().c_str()));
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
    return getNamespace() + mId->text().toUtf8().constData();
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

std::string CSVWorld::GenericCreator::getNamespace() const
{
    CSMWorld::Scope scope = CSMWorld::Scope_Content;

    if (mScope)
    {
        scope = static_cast<CSMWorld::Scope> (mScope->itemData (mScope->currentIndex()).toInt());
    }
    else
    {
        if (mScopes & CSMWorld::Scope_Project)
            scope = CSMWorld::Scope_Project;
        else if (mScopes & CSMWorld::Scope_Session)
            scope = CSMWorld::Scope_Session;
    }

    switch (scope)
    {
        case CSMWorld::Scope_Content: return "";
        case CSMWorld::Scope_Project: return "project::";
        case CSMWorld::Scope_Session: return "session::";
    }

    return "";
}

CSVWorld::GenericCreator::GenericCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id, bool relaxedIdRules)
: mData (data), mUndoStack (undoStack), mListId (id), mLocked (false), mCloneMode (false),
  mClonedType (CSMWorld::UniversalId::Type_None), mScopes (CSMWorld::Scope_Content), mScope (0),
  mScopeLabel (0), mNamespace (0)
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
        std::string id = getId();

        if (mCloneMode)
        {
            std::auto_ptr<CSMWorld::CloneCommand> command (new CSMWorld::CloneCommand (
                dynamic_cast<CSMWorld::IdTable&> (*mData.getTableModel(mListId)), mClonedId, id, mClonedType));

            mUndoStack.push(command.release());

        }
        else
        {
            std::auto_ptr<CSMWorld::CreateCommand> command (new CSMWorld::CreateCommand (
            dynamic_cast<CSMWorld::IdTable&> (*mData.getTableModel (mListId)), id));

            configureCreateCommand (*command);

            mUndoStack.push (command.release());
        }

        emit done();
        emit requestFocus(id);
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

void CSVWorld::GenericCreator::setScope (unsigned int scope)
{
    mScopes = scope;
    int count = (mScopes & CSMWorld::Scope_Content) + (mScopes & CSMWorld::Scope_Project) +
        (mScopes & CSMWorld::Scope_Session);

    // namespace widget
    if (count>1 || (count>0 && !(mScopes & CSMWorld::Scope_Content)))
    {
        if (!mNamespace)
        {
            mNamespace = new QLabel ("::", this);
            insertAtBeginning (mNamespace, false);
        }
    }
    else
    {
        delete mNamespace;
        mNamespace = 0;
    }

    // scope selector widget
    if (count>1)
    {
        mScope = new QComboBox (this);
        insertAtBeginning (mScope, false);

        if (mScopes & CSMWorld::Scope_Content)
            mScope->addItem ("Content", static_cast<int> (CSMWorld::Scope_Content));

        if (mScopes & CSMWorld::Scope_Project)
            mScope->addItem ("Project", static_cast<int> (CSMWorld::Scope_Project));

        if (mScopes & CSMWorld::Scope_Session)
            mScope->addItem ("Session", static_cast<int> (CSMWorld::Scope_Session));

        connect (mScope, SIGNAL (currentIndexChanged (int)), this, SLOT (scopeChanged (int)));

        mScopeLabel = new QLabel ("Scope", this);
        insertAtBeginning (mScopeLabel, false);

        mScope->setCurrentIndex (0);
    }
    else
    {
        delete mScope;
        mScope = 0;

        delete mScopeLabel;
        mScopeLabel = 0;
    }
}

void CSVWorld::GenericCreator::scopeChanged (int index)
{
    update();
}