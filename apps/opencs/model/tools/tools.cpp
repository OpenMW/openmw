
#include "tools.hpp"

#include <QThreadPool>

#include "verifier.hpp"

#include "../doc/state.hpp"

CSMTools::Operation *CSMTools::Tools::get (int type)
{
    switch (type)
    {
        case CSMDoc::State_Verifying: return mVerifier;
    }

    return 0;
}

const CSMTools::Operation *CSMTools::Tools::get (int type) const
{
    return const_cast<Tools *> (this)->get (type);
}

CSMTools::Verifier *CSMTools::Tools::getVerifier()
{
    if (!mVerifier)
    {
        mVerifier = new Verifier;

        connect (mVerifier, SIGNAL (progress (int, int, int)), this, SIGNAL (progress (int, int, int)));
        connect (mVerifier, SIGNAL (finished()), this, SLOT (verifierDone()));
    }

    return mVerifier;
}

CSMTools::Tools::Tools() : mVerifier (0)
{

}

CSMTools::Tools::~Tools()
{
    delete mVerifier;
}

void CSMTools::Tools::runVerifier()
{
    getVerifier()->start();
}

void CSMTools::Tools::abortOperation (int type)
{
    if (Operation *operation = get (type))
        operation->abort();
}

int CSMTools::Tools::getRunningOperations() const
{
    static const int sOperations[] =
    {
       CSMDoc::State_Verifying,
        -1
    };

    int result = 0;

    for (int i=0; sOperations[i]!=-1; ++i)
        if (const Operation *operation = get (sOperations[i]))
            if (operation->isRunning())
                result |= sOperations[i];

    return result;
}

void CSMTools::Tools::verifierDone()
{
    emit done (CSMDoc::State_Verifying);
}