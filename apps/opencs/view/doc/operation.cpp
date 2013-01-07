
#include "operation.hpp"

#include <sstream>

#include "../../model/doc/document.hpp"

void CSVDoc::Operation::updateLabel (int threads)
{
    if (threads==-1 || ((threads==0)!=mStalling))
    {
        std::string name ("unknown operation");

        switch (mType)
        {
            case CSMDoc::State_Saving: name = "saving"; break;
            case CSMDoc::State_Verifying: name = "verifying"; break;
        }

        std::ostringstream stream;

        if ((mStalling = (threads<=0)))
        {
            stream << name << " (waiting for a free worker thread)";
        }
        else
        {
            stream << name << " (%p%)";
        }

        setFormat (stream.str().c_str());
    }
}

CSVDoc::Operation::Operation (int type) : mType (type), mStalling (false)
{
    /// \todo Add a cancel button or a pop up menu with a cancel item

    updateLabel();

    /// \todo assign different progress bar colours to allow the user to distinguish easily between operation types
}

void CSVDoc::Operation::setProgress (int current, int max, int threads)
{
    updateLabel (threads);
    setRange (0, max);
    setValue (current);
}

int CSVDoc::Operation::getType() const
{
    return mType;
}