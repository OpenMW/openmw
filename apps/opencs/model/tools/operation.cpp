
#include "operation.hpp"

#include <QTimer>

#include "../doc/state.hpp"

CSMTools::Operation::Operation (int type) : mType (type) {}

void CSMTools::Operation::run()
{
    mStep = 0;

    QTimer timer;

    timer.connect (&timer, SIGNAL (timeout()), this, SLOT (verify()));

    timer.start (0);

    exec();
}

void CSMTools::Operation::abort()
{
    exit();
}

void CSMTools::Operation::verify()
{
    ++mStep;
    emit progress (mStep, 1000, mType);

    if (mStep>=1000)
        exit();
}