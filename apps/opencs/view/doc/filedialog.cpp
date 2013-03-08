#include "filedialog.hpp"

#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>
#include <QRegExpValidator>
#include <QRegExp>
#include <QSpacerItem>
#include <QPushButton>
#include <QLabel>

#include <components/fileorderlist/model/datafilesmodel.hpp>
#include <components/fileorderlist/model/pluginsproxymodel.hpp>
#include <components/fileorderlist/model/esm/esmfile.hpp>

#include <components/fileorderlist/utils/lineedit.hpp>

FileDialog::FileDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    // Models
    mDataFilesModel = new DataFilesModel(this);

    mMastersProxyModel = new QSortFilterProxyModel();
    mMastersProxyModel->setFilterRegExp(QString("^.*\\.esm"));
    mMastersProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mMastersProxyModel->setSourceModel(mDataFilesModel);

    mPluginsProxyModel = new PluginsProxyModel();
    mPluginsProxyModel->setFilterRegExp(QString("^.*\\.esp"));
    mPluginsProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mPluginsProxyModel->setSourceModel(mDataFilesModel);

    mFilterProxyModel = new QSortFilterProxyModel();
    mFilterProxyModel->setDynamicSortFilter(true);
    mFilterProxyModel->setSourceModel(mPluginsProxyModel);

    QCheckBox checkBox;
    unsigned int height = checkBox.sizeHint().height() + 4;

    mastersTable->setModel(mMastersProxyModel);
    mastersTable->setObjectName("MastersTable");
    mastersTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mastersTable->setSortingEnabled(false);
    mastersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mastersTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mastersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mastersTable->setAlternatingRowColors(true);
    mastersTable->horizontalHeader()->setStretchLastSection(true);

    // Set the row height to the size of the checkboxes
    mastersTable->verticalHeader()->setDefaultSectionSize(height);
    mastersTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    mastersTable->verticalHeader()->hide();

    pluginsTable->setModel(mFilterProxyModel);
    pluginsTable->setObjectName("PluginsTable");
    pluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    pluginsTable->setSortingEnabled(false);
    pluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pluginsTable->setAlternatingRowColors(true);
    pluginsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    pluginsTable->horizontalHeader()->setStretchLastSection(true);

    pluginsTable->verticalHeader()->setDefaultSectionSize(height);
    pluginsTable->verticalHeader()->setResizeMode(QHeaderView::Fixed);

    // Hide the profile elements
    profileLabel->hide();
    profilesComboBox->hide();
    newProfileButton->hide();
    deleteProfileButton->hide();

    // Add some extra widgets
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    mNameLabel = new QLabel(tr("File Name:"), this);

    QRegExpValidator *validator = new QRegExpValidator(QRegExp("^[a-zA-Z0-9\\s]*$"));
    mNameLineEdit = new LineEdit(this);
    mNameLineEdit->setValidator(validator);

    nameLayout->addSpacerItem(spacer);
    nameLayout->addWidget(mNameLabel);
    nameLayout->addWidget(mNameLineEdit);

    mButtonBox = new QDialogButtonBox(this);

    mCreateButton = new QPushButton(tr("Create"), this);
    mCreateButton->setEnabled(false);

    verticalLayout->addLayout(nameLayout);
    verticalLayout->addWidget(mButtonBox);

    // Set sizes
    QList<int> sizeList;
    sizeList << 175;
    sizeList << 200;

    splitter->setSizes(sizeList);

    resize(600, 400);

    connect(mDataFilesModel, SIGNAL(layoutChanged()), this, SLOT(updateViews()));
    connect(mDataFilesModel, SIGNAL(checkedItemsChanged(QStringList)), this, SLOT(updateOpenButton(QStringList)));
    connect(mNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateCreateButton(QString)));

    connect(filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    connect(pluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));
    connect(mastersTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckState(QModelIndex)));

    connect(mCreateButton, SIGNAL(clicked()), this, SLOT(createButtonClicked()));

    connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void FileDialog::updateViews()
{
    // Ensure the columns are hidden because sort() re-enables them
    mastersTable->setColumnHidden(1, true);
    mastersTable->setColumnHidden(3, true);
    mastersTable->setColumnHidden(4, true);
    mastersTable->setColumnHidden(5, true);
    mastersTable->setColumnHidden(6, true);
    mastersTable->setColumnHidden(7, true);
    mastersTable->setColumnHidden(8, true);
    mastersTable->resizeColumnsToContents();

    pluginsTable->setColumnHidden(1, true);
    pluginsTable->setColumnHidden(3, true);
    pluginsTable->setColumnHidden(4, true);
    pluginsTable->setColumnHidden(5, true);
    pluginsTable->setColumnHidden(6, true);
    pluginsTable->setColumnHidden(7, true);
    pluginsTable->setColumnHidden(8, true);
    pluginsTable->resizeColumnsToContents();

}

void FileDialog::updateOpenButton(const QStringList &items)
{
    QPushButton *openButton = mButtonBox->button(QDialogButtonBox::Open);

    if (!openButton)
        return;

    openButton->setEnabled(!items.isEmpty());
}

void FileDialog::updateCreateButton(const QString &name)
{
    if (!mCreateButton->isVisible())
        return;

    mCreateButton->setEnabled(!name.isEmpty());
}

void FileDialog::filterChanged(const QString &filter)
{
    QRegExp filterRe(filter, Qt::CaseInsensitive, QRegExp::FixedString);
    mFilterProxyModel->setFilterRegExp(filterRe);
}

void FileDialog::addFiles(const QString &path)
{
    mDataFilesModel->addFiles(path);
    mDataFilesModel->sort(3);  // Sort by date accessed
}

void FileDialog::setEncoding(const QString &encoding)
{
    mDataFilesModel->setEncoding(encoding);
}

void FileDialog::setCheckState(QModelIndex index)
{
    if (!index.isValid())
        return;

    QObject *object = QObject::sender();

    // Not a signal-slot call
    if (!object)
        return;


    if (object->objectName() == QLatin1String("PluginsTable")) {
        QModelIndex sourceIndex = mPluginsProxyModel->mapToSource(
                    mFilterProxyModel->mapToSource(index));

        if (sourceIndex.isValid()) {
            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mDataFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                    : mDataFilesModel->setCheckState(sourceIndex, Qt::Checked);
        }
    }

    if (object->objectName() == QLatin1String("MastersTable")) {
        QModelIndex sourceIndex = mMastersProxyModel->mapToSource(index);

        if (sourceIndex.isValid()) {
            (mDataFilesModel->checkState(sourceIndex) == Qt::Checked)
                    ? mDataFilesModel->setCheckState(sourceIndex, Qt::Unchecked)
                    : mDataFilesModel->setCheckState(sourceIndex, Qt::Checked);
        }
    }

    return;
}

QStringList FileDialog::checkedItemsPaths()
{
    return mDataFilesModel->checkedItemsPaths();
}

QString FileDialog::fileName()
{
    return mNameLineEdit->text();
}

void FileDialog::openFile()
{
    setWindowTitle(tr("Open"));

    mNameLabel->hide();
    mNameLineEdit->hide();
    mCreateButton->hide();

    mButtonBox->removeButton(mCreateButton);
    mButtonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Open);
    QPushButton *openButton = mButtonBox->button(QDialogButtonBox::Open);
    openButton->setEnabled(false);

    show();
    raise();
    activateWindow();
}

void FileDialog::newFile()
{
    setWindowTitle(tr("New"));

    mNameLabel->show();
    mNameLineEdit->clear();
    mNameLineEdit->show();
    mCreateButton->show();

    mButtonBox->setStandardButtons(QDialogButtonBox::Cancel);
    mButtonBox->addButton(mCreateButton, QDialogButtonBox::ActionRole);

    show();
    raise();
    activateWindow();
}

void FileDialog::accept()
{
    emit openFiles();
}

void FileDialog::createButtonClicked()
{
    emit createNewFile();
}
