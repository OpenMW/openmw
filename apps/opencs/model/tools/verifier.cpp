
#include "verifier.hpp"

#include <QTimer>

#include "../doc/document.hpp"

void CSMTools::Verifier::run()
{
    mStep = 0;

    QTimer timer;

    timer.connect (&timer, SIGNAL (timeout()), this, SLOT (verify()));

    timer.start (0);

    exec();
}

void CSMTools::Verifier::abort()
{
    exit();
}

void CSMTools::Verifier::verify()
{
    ++mStep;
    emit progress (mStep, 1000, CSMDoc::Document::State_Verifying);

    if (mStep>=1000)
        exit();
}