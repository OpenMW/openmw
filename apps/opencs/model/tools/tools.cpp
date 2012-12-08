
#include "tools.hpp"

#include <QThreadPool>

#include "verifier.hpp"

#include "../doc/document.hpp"

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

}

void CSMTools::Tools::verifierDone()
{
    emit done (CSMDoc::Document::State_Verifying);
}