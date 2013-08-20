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

#include <components/esxselector/model/datafilesmodel.hpp>
#include <components/esxselector/model/pluginsproxymodel.hpp>
#include <components/esxselector/model/esmfile.hpp>

#include <components/esxselector/view/lineedit.hpp>

#include "components/esxselector/model/masterproxymodel.hpp"

CSVDoc::FileDialog::FileDialog(QWidget *parent) :
    ContentSelector(parent)
{
    // Hide the profile elements
    profileGroupBox->hide();
    pluginView->showColumn(2);

    resize(400, 400);

    connect(mDataFilesModel, SIGNAL(checkedItemsChanged(QStringList)), this, SLOT(updateOpenButton(QStringList)));
    connect(projectNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateCreateButton(QString)));

    connect(projectCreateButton, SIGNAL(clicked()), this, SIGNAL(createNewFile()));

    connect(projectButtonBox, SIGNAL(accepted()), this, SIGNAL(openFiles()));
    connect(projectButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CSVDoc::FileDialog::updateOpenButton(const QStringList &items)
{
    QPushButton *openButton = projectButtonBox->button(QDialogButtonBox::Open);

    if (!openButton)
        return;

    openButton->setEnabled(!items.isEmpty());
}

void CSVDoc::FileDialog::updateCreateButton(const QString &name)
{
    if (!projectCreateButton->isVisible())
        return;

    projectCreateButton->setEnabled(!name.isEmpty());
}

QString CSVDoc::FileDialog::fileName()
{
    return projectNameLineEdit->text();
}

void CSVDoc::FileDialog::openFile()
{
    setWindowTitle(tr("Open"));

    projectNameLineEdit->hide();
    projectCreateButton->hide();
    projectGroupBox->setTitle(tr(""));
    projectButtonBox->button(QDialogButtonBox::Open)->setEnabled(false);

    show();
    raise();
    activateWindow();
}

void CSVDoc::FileDialog::newFile()
{
    setWindowTitle(tr("New"));

    projectButtonBox->setStandardButtons(QDialogButtonBox::Cancel);
    projectButtonBox->addButton(projectCreateButton, QDialogButtonBox::ActionRole);

    show();
    raise();
    activateWindow();
}
