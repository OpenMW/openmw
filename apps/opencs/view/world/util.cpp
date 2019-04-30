#include "util.hpp"

#include <limits>
#include <stdexcept>

#include <QUndoStack>
#include <QMetaProperty>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QEvent>
#include <QItemEditorFactory>

#include "../../model/world/commands.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/commanddispatcher.hpp"

#include "../widget/coloreditor.hpp"
#include "../widget/droplineedit.hpp"

#include "dialoguespinbox.hpp"
#include "scriptedit.hpp"

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
    CSMWorld::ColumnBase::Display display, CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent) const
{
    std::map<CSMWorld::ColumnBase::Display, CommandDelegateFactory *>::const_iterator iter =
        mFactories.find (display);

    if (iter!=mFactories.end())
        return iter->second->makeDelegate (dispatcher, document, parent);

    return new CommandDelegate (dispatcher, document, parent);
}

const CSVWorld::CommandDelegateFactoryCollection& CSVWorld::CommandDelegateFactoryCollection::get()
{
    if (!sThis)
        throw std::logic_error ("no instance of CSVWorld::CommandDelegateFactoryCollection");

    return *sThis;
}


QUndoStack& CSVWorld::CommandDelegate::getUndoStack() const
{
    return mDocument.getUndoStack();
}

CSMDoc::Document& CSVWorld::CommandDelegate::getDocument() const
{
    return mDocument;
}

CSMWorld::ColumnBase::Display CSVWorld::CommandDelegate::getDisplayTypeFromIndex(const QModelIndex &index) const
{
    int rawDisplay = index.data(CSMWorld::ColumnBase::Role_Display).toInt();
    return static_cast<CSMWorld::ColumnBase::Display>(rawDisplay);
}

void CSVWorld::CommandDelegate::setModelDataImp (QWidget *editor, QAbstractItemModel *model,
    const QModelIndex& index) const
{
    if (!mCommandDispatcher)
        return;

    QVariant variant;

    // Color columns use a custom editor, so we need to fetch selected color from it.
    CSVWidget::ColorEditor *colorEditor = qobject_cast<CSVWidget::ColorEditor *>(editor);
    if (colorEditor != nullptr)
    {
        variant = colorEditor->colorInt();
    }
    else
    {
        NastyTableModelHack hack (*model);
        QStyledItemDelegate::setModelData (editor, &hack, index);
        variant = hack.getData();
    }

    if ((model->data (index)!=variant) && (model->flags(index) & Qt::ItemIsEditable))
        mCommandDispatcher->executeModify (model, index, variant);
}

CSVWorld::CommandDelegate::CommandDelegate (CSMWorld::CommandDispatcher *commandDispatcher,
    CSMDoc::Document& document, QObject *parent)
: QStyledItemDelegate (parent), mEditLock (false),
  mCommandDispatcher (commandDispatcher), mDocument (document)
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
    const QModelIndex& index) const
{
    CSMWorld::ColumnBase::Display display = getDisplayTypeFromIndex(index);

    // This createEditor() method is called implicitly from tables.
    // For boolean values in tables use the default editor (combobox).
    // Checkboxes is looking ugly in the table view.
    // TODO: Find a better solution?
    if (display == CSMWorld::ColumnBase::Display_Boolean)
    {
        return QItemEditorFactory::defaultFactory()->createEditor(QVariant::Bool, parent);
    }
    // For tables the pop-up of the color editor should appear immediately after the editor creation
    // (the third parameter of ColorEditor's constructor)
    else if (display == CSMWorld::ColumnBase::Display_Colour)
    {
        return new CSVWidget::ColorEditor(index.data().toInt(), parent, true);
    }
    return createEditor (parent, option, index, display);
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

    // NOTE: for each editor type (e.g. QLineEdit) there needs to be a corresponding
    // entry in CSVWorld::DialogueDelegateDispatcher::makeEditor()
    switch (display)
    {
        case CSMWorld::ColumnBase::Display_Colour:
        {
            return new CSVWidget::ColorEditor(variant.toInt(), parent);
        }
        case CSMWorld::ColumnBase::Display_Integer:
        {
            DialogueSpinBox *sb = new DialogueSpinBox(parent);
            sb->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            return sb;
        }

        case CSMWorld::ColumnBase::Display_SignedInteger8:
        {
            DialogueSpinBox *sb = new DialogueSpinBox(parent);
            sb->setRange(std::numeric_limits<signed char>::min(), std::numeric_limits<signed char>::max());
            return sb;
        }
        case CSMWorld::ColumnBase::Display_SignedInteger16:
        {
            DialogueSpinBox *sb = new DialogueSpinBox(parent);
            sb->setRange(std::numeric_limits<short>::min(), std::numeric_limits<short>::max());
            return sb;
        }

        case CSMWorld::ColumnBase::Display_UnsignedInteger8:
        {
            DialogueSpinBox *sb = new DialogueSpinBox(parent);
            sb->setRange(0, std::numeric_limits<unsigned char>::max());
            return sb;
        }

        case CSMWorld::ColumnBase::Display_UnsignedInteger16:
        {
            DialogueSpinBox *sb = new DialogueSpinBox(parent);
            sb->setRange(0, std::numeric_limits<unsigned short>::max());
            return sb;
        }

        case CSMWorld::ColumnBase::Display_Var:

            return new QLineEdit(parent);

        case CSMWorld::ColumnBase::Display_Float:
        {
            DialogueDoubleSpinBox *dsb = new DialogueDoubleSpinBox(parent);
            dsb->setRange(-std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
            dsb->setSingleStep(0.01f);
            dsb->setDecimals(3);
            return dsb;
        }

        case CSMWorld::ColumnBase::Display_Double:
        {
            DialogueDoubleSpinBox *dsb = new DialogueDoubleSpinBox(parent);
            dsb->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
            dsb->setSingleStep(0.01f);
            dsb->setDecimals(6);
            return dsb;
        }

        case CSMWorld::ColumnBase::Display_LongString:
        {
            QPlainTextEdit *edit = new QPlainTextEdit(parent);
            edit->setUndoRedoEnabled (false);
            return edit;
        }

        case CSMWorld::ColumnBase::Display_LongString256:
        {
            /// \todo implement size limit. QPlainTextEdit does not support a size limit.
            QPlainTextEdit *edit = new QPlainTextEdit(parent);
            edit->setUndoRedoEnabled (false);
            return edit;
        }

        case CSMWorld::ColumnBase::Display_Boolean:

            return new QCheckBox(parent);

        case CSMWorld::ColumnBase::Display_ScriptLines:

            return new ScriptEdit (mDocument, ScriptHighlighter::Mode_Console, parent);

        case CSMWorld::ColumnBase::Display_String:
        // For other Display types (that represent record IDs) with drop support IdCompletionDelegate is used

            return new CSVWidget::DropLineEdit(display, parent);

        case CSMWorld::ColumnBase::Display_String32:
        {
        // For other Display types (that represent record IDs) with drop support IdCompletionDelegate is used
            CSVWidget::DropLineEdit *widget = new CSVWidget::DropLineEdit(display, parent);
            widget->setMaxLength (32);
            return widget;
        }

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

void CSVWorld::CommandDelegate::setEditorData (QWidget *editor, const QModelIndex& index) const
{
    setEditorData (editor, index, false);
}

void CSVWorld::CommandDelegate::setEditorData (QWidget *editor, const QModelIndex& index, bool tryDisplay) const
{
    QVariant variant = index.data(Qt::EditRole);
    if (tryDisplay)
    {
        if (!variant.isValid())
        {
            variant = index.data(Qt::DisplayRole);
            if (!variant.isValid())
            {
                return;
            }
        }
        QPlainTextEdit* plainTextEdit = qobject_cast<QPlainTextEdit*>(editor);
        if(plainTextEdit) //for some reason it is easier to brake the loop here
        {
            if (plainTextEdit->toPlainText() == variant.toString())
            {
                return;
            }
        }
    }

    // Color columns use a custom editor, so we need explicitly set a data for it
    CSVWidget::ColorEditor *colorEditor = qobject_cast<CSVWidget::ColorEditor *>(editor);
    if (colorEditor != nullptr)
    {
        colorEditor->setColor(variant.toInt());
        return;
    }

    QByteArray n = editor->metaObject()->userProperty().name();

    if (n == "dateTime")
    {
        if (editor->inherits("QTimeEdit"))
            n = "time";
        else if (editor->inherits("QDateEdit"))
            n = "date";
    }

    if (!n.isEmpty())
    {
        if (!variant.isValid())
            variant = QVariant(editor->property(n).userType(), (const void *)0);
        editor->setProperty(n, variant);
    }

}

void CSVWorld::CommandDelegate::settingChanged (const CSMPrefs::Setting *setting) {}
