#include "operations.hpp"

#include <algorithm>

#include <QDockWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "operation.hpp"

namespace
{
    constexpr int operationLineHeight = 40;
}

CSVDoc::Operations::Operations()
{
    /// \todo make widget height fixed (exactly the height required to display all operations)

    setFeatures(QDockWidget::NoDockWidgetFeatures);

    QWidget* widgetContainer = new QWidget(this);
    mLayout = new QVBoxLayout;

    widgetContainer->setLayout(mLayout);
    setWidget(widgetContainer);
    setVisible(false);
    setFixedHeight(operationLineHeight);
    setTitleBarWidget(new QWidget(this));
}

void CSVDoc::Operations::setProgress(int current, int max, int type, int threads)
{
    for (std::vector<Operation*>::iterator iter(mOperations.begin()); iter != mOperations.end(); ++iter)
        if ((*iter)->getType() == type)
        {
            (*iter)->setProgress(current, max, threads);
            return;
        }

    Operation* operation = new Operation(type, this);
    connect(operation, qOverload<int>(&Operation::abortOperation), this, &Operations::abortOperation);

    mLayout->addLayout(operation->getLayout());
    mOperations.push_back(operation);
    operation->setProgress(current, max, threads);

    int newCount = static_cast<int>(mOperations.size());
    setFixedHeight(operationLineHeight * newCount);

    setVisible(true);
}

void CSVDoc::Operations::quitOperation(int type)
{
    for (std::vector<Operation*>::iterator iter(mOperations.begin()); iter != mOperations.end(); ++iter)
        if ((*iter)->getType() == type)
        {
            mLayout->removeItem((*iter)->getLayout());

            (*iter)->deleteLater();
            mOperations.erase(iter);

            int newCount = static_cast<int>(mOperations.size());
            if (newCount > 0)
                setFixedHeight(operationLineHeight * newCount);
            else
                setVisible(false);

            break;
        }
}
