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
#include <QGroupBox>
#include <QGridLayout>

#include <components/contentselector/model/esmfile.hpp>
#include <components/contentselector/view/lineedit.hpp>
#include "components/contentselector/view/contentselector.hpp"

#include <QDebug>

CSVDoc::FileDialog::FileDialog(QWidget *parent) :
    QDialog(parent),
    mOpenFileFlags (ContentSelectorView::Flag_Content | ContentSelectorView::Flag_LoadAddon),
    mNewFileFlags (ContentSelectorView::Flag_Content | ContentSelectorView::Flag_NewAddon)

{
    resize(400, 400);
}

void CSVDoc::FileDialog::addFiles(const QString &path)
{
    ContentSelectorView::ContentSelector::addFiles(path);
}

QString CSVDoc::FileDialog::filename()
{
    return ContentSelectorView::ContentSelector::instance().filename();
}

QStringList CSVDoc::FileDialog::selectedFilepaths()
{
    return ContentSelectorView::ContentSelector::instance().selectedFiles();
}

void CSVDoc::FileDialog::showDialog()
{
    show();
    raise();
    activateWindow();
}

void CSVDoc::FileDialog::openFile()
{
    setWindowTitle(tr("Open"));

    ContentSelectorView::ContentSelector::configure(this, mOpenFileFlags);

    connect (&ContentSelectorView::ContentSelector::instance(),
            SIGNAL (accepted()), this, SIGNAL (openFiles()));

    connect (&ContentSelectorView::ContentSelector::instance(),
            SIGNAL (rejected()), this, SLOT (slotRejected()));

    showDialog();
}

void CSVDoc::FileDialog::newFile()
{
    setWindowTitle(tr("New"));

    ContentSelectorView::ContentSelector::configure(this, mNewFileFlags);

    connect (&ContentSelectorView::ContentSelector::instance(),
            SIGNAL (accepted()), this, SIGNAL (createNewFile()));

    connect (&ContentSelectorView::ContentSelector::instance(),
            SIGNAL (rejected()), this, SLOT (slotRejected()));

    showDialog();
}

void CSVDoc::FileDialog::slotRejected()
{
    close();
}
