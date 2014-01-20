#include <QTableView>
#include <QGridLayout>

#include "testharnesspage.hpp"
#include "../../model/settings/declarationmodel.hpp"
#include "../../model/settings/definitionmodel.hpp"

#include <QDebug>

CSVSettings::TestHarnessPage::TestHarnessPage(
                            CSMSettings::DeclarationModel &declarationModel,
                            CSMSettings::DefinitionModel &definitionModel,
                            QWidget *parent)

    : Page ("Test Harness", declarationModel, definitionModel, true, parent)
{
    QTableView *sourceView = new QTableView(parent);

    qDebug () << "source view model rowcount: " << definitionModel.rowCount();
    sourceView->setMinimumWidth(50);
    sourceView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sourceView->setSelectionBehavior(QAbstractItemView::SelectItems);
    sourceView->setModel(&definitionModel);

    pageFrame()->layout()->addWidget(sourceView);

    //item clicked connection to update the views for the newly selected setting
    //connect (sourceView, SIGNAL (), this SLOT ());
}
