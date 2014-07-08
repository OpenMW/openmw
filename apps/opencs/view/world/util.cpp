
#include "util.hpp"

#include <stdexcept>

#include <QUndoStack>
#include <QMetaProperty>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QEvent>

#include "../../model/world/commands.hpp"
#include "../../model/world/tablemimedata.hpp"

CSVWorld::NastyTableModelHack::NastyTableModelHack (QAbstractItemModel& model)
: mModel (model)
{}

int CSVWorld::NastyTableModelHack::rowCount (const QModelIndex & parent) const
{
    return mModel.rowCount (parent);
}

int CSVWorld::NastyTableModelHack::columnCount (const QModelIndex & parent) const
{
    return mModel.columnCount (parent);
}

QVariant CSVWorld::NastyTableModelHack::data  (const QModelIndex & index, int role) const
{
    return mModel.data (index, role);
}

bool CSVWorld::NastyTableModelHack::setData ( const QModelIndex &index, const QVariant &value, int role)
{
    mData = value;
    return true;
}

QVariant CSVWorld::NastyTableModelHack::getData() const
{
    return mData;
}


CSVWorld::CommandDelegateFactory::~CommandDelegateFactory() {}


CSVWorld::CommandDelegateFactoryCollection *CSVWorld::CommandDelegateFactoryCollection::sThis = 0;

CSVWorld::CommandDelegateFactoryCollection::CommandDelegateFactoryCollection()
{
    if (sThis)
        throw std::logic_error ("multiple instances of CSVWorld::CommandDelegateFactoryCollection");

    sThis = this;
}

CSVWorld::CommandDelegateFactoryCollection::~CommandDelegateFactoryCollection()
{
    sThis = 0;

    for (std::map<CSMWorld::ColumnBase::Display, CommandDelegateFactory *>::iterator iter (
        mFactories.begin());
        iter!=mFactories.end(); ++iter)
         delete iter->second;
}

void CSVWorld::CommandDelegateFactoryCollection::add (CSMWorld::ColumnBase::Display display,
    CommandDelegateFactory *factory)
{
    mFactories.insert (std::make_pair (display, factory));
}

CSVWorld::CommandDelegate *CSVWorld::CommandDelegateFactoryCollection::makeDelegate (
    CSMWorld::ColumnBase::Display display, QUndoStack& undoStack, QObject *parent) const
{
    std::map<CSMWorld::ColumnBase::Display, CommandDelegateFactory *>::const_iterator iter =
        mFactories.find (display);

    if (iter!=mFactories.end())
        return iter->second->makeDelegate (undoStack, parent);

    return new CommandDelegate (undoStack, parent);
}

const CSVWorld::CommandDelegateFactoryCollection& CSVWorld::CommandDelegateFactoryCollection::get()
{
    if (!sThis)
        throw std::logic_error ("no instance of CSVWorld::CommandDelegateFactoryCollection");

    return *sThis;
}


QUndoStack& CSVWorld::CommandDelegate::getUndoStack() const
{
    return mUndoStack;
}

void CSVWorld::CommandDelegate::setModelDataImp (QWidget *editor, QAbstractItemModel *model,
    const QModelIndex& index) const
{
    NastyTableModelHack hack (*model);
    QStyledItemDelegate::setModelData (editor, &hack, index);

    QVariant new_ = hack.getData();

    if (model->data (index)!=new_)
        mUndoStack.push (new CSMWorld::ModifyCommand (*model, index, new_));
}

CSVWorld::CommandDelegate::CommandDelegate (QUndoStack& undoStack, QObject *parent)
: QStyledItemDelegate (parent), mUndoStack (undoStack), mEditLock (false)
{}

void CSVWorld::CommandDelegate::setModelData (QWidget *editor, QAbstractItemModel *model,
        const QModelIndex& index) const
{
    if (!mEditLock)
    {
        setModelDataImp (editor, model, index);
    }

    ///< \todo provide some kind of feedback to the user, indicating that editing is currently not possible.
}

QWidget *CSVWorld::CommandDelegate::createEditor (QWidget *parent, const QStyleOptionViewItem& option,
    const QModelIndex& index, CSMWorld::ColumnBase::Display display) const
{
    QVariant variant = index.data();
    if (!variant.isValid())
    {
        variant = index.data(Qt::DisplayRole);
        if (!variant.isValid())
        {
            return 0;
        }
    }

    switch (display)
    {
        case CSMWorld::ColumnBase::Display_Colour:

            return new QLineEdit(parent);

        case CSMWorld::ColumnBase::Display_Integer:

            return new QSpinBox(parent);

        case CSMWorld::ColumnBase::Display_Var:

            return new QLineEdit(parent);

        case CSMWorld::ColumnBase::Display_Float:

            return new QDoubleSpinBox(parent);

        case CSMWorld::ColumnBase::Display_LongString:

            return new QTextEdit(parent);

        case CSMWorld::ColumnBase::Display_Boolean:

            return new QCheckBox(parent);

        case CSMWorld::ColumnBase::Display_String:
        case CSMWorld::ColumnBase::Display_Skill:
        case CSMWorld::ColumnBase::Display_Script:
        case CSMWorld::ColumnBase::Display_Race:
        case CSMWorld::ColumnBase::Display_Class:
        case CSMWorld::ColumnBase::Display_Faction:
        case CSMWorld::ColumnBase::Display_Miscellaneous:
        case CSMWorld::ColumnBase::Display_Sound:
        case CSMWorld::ColumnBase::Display_Mesh:
        case CSMWorld::ColumnBase::Display_Icon:
        case CSMWorld::ColumnBase::Display_Music:
        case CSMWorld::ColumnBase::Display_SoundRes:
        case CSMWorld::ColumnBase::Display_Texture:
        case CSMWorld::ColumnBase::Display_Video:

            return new DropLineEdit(parent);

        default:

            return QStyledItemDelegate::createEditor (parent, option, index);
    }
}

void CSVWorld::CommandDelegate::setEditLock (bool locked)
{
    mEditLock = locked;
}

bool CSVWorld::CommandDelegate::isEditLocked() const
{
    return mEditLock;
}

void CSVWorld::CommandDelegate::setEditorData (QWidget *editor, const QModelIndex& index, bool tryDisplay) const
{
    QVariant v = index.data(Qt::EditRole);
    if (tryDisplay)
    {
        if (!v.isValid())
        {
            v = index.data(Qt::DisplayRole);
            if (!v.isValid())
            {
                return;
            }
        }
        QPlainTextEdit* plainTextEdit = qobject_cast<QPlainTextEdit*>(editor);
        if(plainTextEdit) //for some reason it is easier to brake the loop here
        {
            if(plainTextEdit->toPlainText() == v.toString())
            {
                return;
            }
        }
    }

    QByteArray n = editor->metaObject()->userProperty().name();

    if (n == "dateTime") {
        if (editor->inherits("QTimeEdit"))
            n = "time";
        else if (editor->inherits("QDateEdit"))
            n = "date";
    }

    if (!n.isEmpty()) {
        if (!v.isValid())
            v = QVariant(editor->property(n).userType(), (const void *)0);
        editor->setProperty(n, v);
    }

}

CSVWorld::DropLineEdit::DropLineEdit(QWidget* parent) :
QLineEdit(parent)
{
    setAcceptDrops(true);
}

void CSVWorld::DropLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void CSVWorld::DropLineEdit::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void CSVWorld::DropLineEdit::dropEvent(QDropEvent *event)
{
    const CSMWorld::TableMimeData* data(dynamic_cast<const CSMWorld::TableMimeData*>(event->mimeData()));
    if (!data) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    emit tableMimeDataDropped(data->getData(), data->getDocumentPtr());
    //WIP
}
