#include "filedialog.hpp"

#include <QDialogButtonBox>
#include <QPushButton>

#include <components/contentselector/model/contentmodel.hpp>
#include <components/contentselector/model/esmfile.hpp>
#include <components/contentselector/view/contentselector.hpp>
#include <components/files/qtconversion.hpp>

#include <string>

#include "adjusterwidget.hpp"
#include "filewidget.hpp"

#include "ui_filedialog.h"

CSVDoc::FileDialog::FileDialog(QWidget* parent)
    : QDialog(parent)
    , mSelector(nullptr)
    , ui(std::make_unique<Ui::FileDialog>())
    , mAction(ContentAction_Undefined)
    , mFileWidget(nullptr)
    , mAdjusterWidget(nullptr)
    , mDialogBuilt(false)
{
    ui->setupUi(this);
    resize(400, 400);

    setObjectName("FileDialog");
    mSelector = new ContentSelectorView::ContentSelector(ui->contentSelectorWidget, /*showOMWScripts=*/false);
    mAdjusterWidget = new AdjusterWidget(this);
}

CSVDoc::FileDialog::~FileDialog() = default;

void CSVDoc::FileDialog::addFiles(const std::vector<std::filesystem::path>& dataDirs)
{
    for (const auto& dir : dataDirs)
    {
        QString path = Files::pathToQString(dir);
        mSelector->addFiles(path);
    }
    mSelector->sortFiles();
}

void CSVDoc::FileDialog::setEncoding(const QString& encoding)
{
    mSelector->setEncoding(encoding);
}

void CSVDoc::FileDialog::clearFiles()
{
    mSelector->clearFiles();
}

QStringList CSVDoc::FileDialog::selectedFilePaths()
{
    QStringList filePaths;

    for (ContentSelectorModel::EsmFile* file : mSelector->selectedFiles())
        filePaths.append(file->filePath());

    return filePaths;
}

void CSVDoc::FileDialog::setLocalData(const std::filesystem::path& localData)
{
    mAdjusterWidget->setLocalData(localData);
}

void CSVDoc::FileDialog::showDialog(ContentAction action)
{
    mAction = action;

    ui->projectGroupBoxLayout->insertWidget(0, mAdjusterWidget);

    switch (mAction)
    {
        case ContentAction_New:
            buildNewFileView();
            break;

        case ContentAction_Edit:
            buildOpenFileView();
            break;

        default:
            break;
    }

    mAdjusterWidget->setFilenameCheck(mAction == ContentAction_New);

    if (!mDialogBuilt)
    {
        // connections common to both dialog view flavors
        connect(mSelector, &ContentSelectorView::ContentSelector::signalCurrentGamefileIndexChanged, this,
            qOverload<int>(&FileDialog::slotUpdateAcceptButton));

        connect(ui->projectButtonBox, &QDialogButtonBox::rejected, this, &FileDialog::slotRejected);
        mDialogBuilt = true;
    }

    show();
    raise();
    activateWindow();
}

void CSVDoc::FileDialog::buildNewFileView()
{
    setWindowTitle(tr("Create a new addon"));

    QPushButton* createButton = ui->projectButtonBox->button(QDialogButtonBox::Ok);
    createButton->setText("Create");
    createButton->setEnabled(false);

    if (!mFileWidget)
    {
        mFileWidget = new FileWidget(this);

        mFileWidget->setType(true);
        mFileWidget->extensionLabelIsVisible(true);

        connect(mFileWidget, &FileWidget::nameChanged, mAdjusterWidget, &AdjusterWidget::setName);

        connect(mFileWidget, &FileWidget::nameChanged, this,
            qOverload<const QString&, bool>(&FileDialog::slotUpdateAcceptButton));
    }

    ui->projectGroupBoxLayout->insertWidget(0, mFileWidget);

    connect(ui->projectButtonBox, &QDialogButtonBox::accepted, this, &FileDialog::slotNewFile);
}

void CSVDoc::FileDialog::buildOpenFileView()
{
    setWindowTitle(tr("Open"));
    ui->projectGroupBox->setTitle(QString(""));
    ui->projectButtonBox->button(QDialogButtonBox::Ok)->setText("Open");
    if (mSelector->isGamefileSelected())
        ui->projectButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    else
        ui->projectButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    if (!mDialogBuilt)
    {
        connect(mSelector, &ContentSelectorView::ContentSelector::signalAddonDataChanged, this,
            &FileDialog::slotAddonDataChanged);
    }
    connect(ui->projectButtonBox, &QDialogButtonBox::accepted, this, &FileDialog::slotOpenFile);
}

void CSVDoc::FileDialog::slotAddonDataChanged(const QModelIndex& topleft, const QModelIndex& bottomright)
{
    slotUpdateAcceptButton(0);
}

void CSVDoc::FileDialog::slotUpdateAcceptButton(int)
{
    QString name = "";

    if (mFileWidget && mAction == ContentAction_New)
        name = mFileWidget->getName();

    slotUpdateAcceptButton(name, true);
}

void CSVDoc::FileDialog::slotUpdateAcceptButton(const QString& name, bool)
{
    bool success = !mSelector->selectedFiles().empty();

    bool isNew = (mAction == ContentAction_New);

    if (isNew)
        success = !name.isEmpty();
    else if (success)
    {
        ContentSelectorModel::EsmFile* file = mSelector->selectedFiles().back();
        mAdjusterWidget->setName(file->filePath(), !file->isGameFile());
    }
    else
        mAdjusterWidget->setName("", true);

    ui->projectButtonBox->button(QDialogButtonBox::Ok)->setEnabled(success);
}

QString CSVDoc::FileDialog::filename() const
{
    if (mAction == ContentAction_New)
        return "";

    return mSelector->currentFile();
}

void CSVDoc::FileDialog::slotRejected()
{
    emit rejected();
    disconnect(ui->projectButtonBox, &QDialogButtonBox::accepted, this, &FileDialog::slotNewFile);
    disconnect(ui->projectButtonBox, &QDialogButtonBox::accepted, this, &FileDialog::slotOpenFile);
    if (mFileWidget)
    {
        delete mFileWidget;
        mFileWidget = nullptr;
    }
    close();
}

void CSVDoc::FileDialog::slotNewFile()
{
    emit signalCreateNewFile(mAdjusterWidget->getPath());
    if (mFileWidget)
    {
        delete mFileWidget;
        mFileWidget = nullptr;
    }
    disconnect(ui->projectButtonBox, &QDialogButtonBox::accepted, this, &FileDialog::slotNewFile);
    close();
}

void CSVDoc::FileDialog::slotOpenFile()
{
    ContentSelectorModel::EsmFile* file = mSelector->selectedFiles().back();

    mAdjusterWidget->setName(file->filePath(), !file->isGameFile());

    emit signalOpenFiles(mAdjusterWidget->getPath());
    disconnect(ui->projectButtonBox, &QDialogButtonBox::accepted, this, &FileDialog::slotOpenFile);
    close();
}
