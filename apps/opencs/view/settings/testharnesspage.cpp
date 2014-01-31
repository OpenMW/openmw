#include <QTableView>
#include <QGridLayout>

#include "testharnesspage.hpp"
#include "../../model/settings/declarationmodel.hpp"
#include "../../model/settings/definitionmodel.hpp"

#include <QDebug>

CSVSettings::TestHarnessPage::TestHarnessPage( CSMSettings::SettingModel &model,
                            QWidget *parent)

    : Page ("Test Harness", model.declarationModel(),
            model.definitionModel(), true, parent)
{
    QTableView *sourceView = new QTableView(parent);

    sourceView->setSelectionBehavior(QAbstractItemView::SelectItems);
    sourceView->setModel(&model.definitionModel());

    pageFrame()->layout()->addWidget(sourceView);
}
