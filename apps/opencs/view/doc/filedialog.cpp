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

#include "components/fileorderlist/masterproxymodel.hpp"

FileDialog::FileDialog(QWidget *parent) :
    ContentSelector(parent)
{
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
    //mNameLineEdit = new LineEdit(this);
    //mNameLineEdit->setValidator(validator);

    nameLayout->addSpacerItem(spacer);
    nameLayout->addWidget(mNameLabel);
    //nameLayout->addWidget(mNameLineEdit);

    mButtonBox = new QDialogButtonBox(this);

    mCreateButton = new QPushButton(tr("Create"), this);
    mCreateButton->setEnabled(false);

    verticalLayout->addLayout(nameLayout);
    verticalLayout->addWidget(mButtonBox);

    resize(600, 400);

  //  connect(mDataFilesModel, SIGNAL(checkedItemsChanged(QStringList)), this, SLOT(updateOpenButton(QStringList)));
    //connect(mNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateCreateButton(QString)));

  //  connect(mCreateButton, SIGNAL(clicked()), this, SLOT(createButtonClicked()));

 //   connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
 //   connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
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

QString FileDialog::fileName()
{
    //return mNameLineEdit->text();
}

void FileDialog::openFile()
{
    setWindowTitle(tr("Open"));

    mNameLabel->hide();
    //mNameLineEdit->hide();
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
    //mNameLineEdit->clear();
    //mNameLineEdit->show();
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
