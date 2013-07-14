
#include "aisequence.hpp"

#include "aipackage.hpp"

void MWMechanics::AiSequence::copy (const AiSequence& sequence)
{
    for (std::list<AiPackage *>::const_iterator iter (sequence.mPackages.begin());
        iter!=sequence.mPackages.end(); ++iter)
        mPackages.push_back ((*iter)->clone());
}

MWMechanics::AiSequence::AiSequence() : mDone (false) {}

MWMechanics::AiSequence::AiSequence (const AiSequence& sequence) : mDone (false)
{
    copy (sequence);
}

MWMechanics::AiSequence& MWMechanics::AiSequence::operator= (const AiSequence& sequence)
{
    if (this!=&sequence)
    {
        clear();
        copy (sequence);
    }
    
    return *this;
}

MWMechanics::AiSequence::~AiSequence()
{
    clear();
}

int MWMechanics::AiSequence::getTypeId() const
{
    if (mPackages.empty())
        return -1;
        
    return mPackages.front()->getTypeId();
}

bool MWMechanics::AiSequence::isPackageDone() const
{
    return mDone;
}

void MWMechanics::AiSequence::execute (const MWWorld::Ptr& actor)
{
    if (!mPackages.empty())
    {
        if (mPackages.front()->execute (actor))
        {
            mPackages.erase (mPackages.begin());
            mDone = true;
        }
        else
            mDone = false;    
    }
}

void MWMechanics::AiSequence::clear()
{
    for (std::list<AiPackage *>::const_iterator iter (mPackages.begin()); iter!=mPackages.end(); ++iter)
        delete *iter;

    mPackages.clear();
}

void MWMechanics::AiSequence::stack (const AiPackage& package)
{
    mPackages.push_front (package.clone());
}

void MWMechanics::AiSequence::queue (const AiPackage& package)
{
    mPackages.push_back (package.clone());
}
