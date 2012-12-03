
#include "editor.hpp"

#include <sstream>

#include <QtGui/QApplication>

#include "model/doc/document.hpp"
#include "model/world/data.hpp"

CS::Editor::Editor() : mViewManager (mDocumentManager), mNewDocumentIndex (0)
{
    connect (&mViewManager, SIGNAL (newDocumentRequest ()), this, SLOT (createDocument ()));
}

void CS::Editor::createDocument()
{
    std::ostringstream stream;

    stream << "NewDocument" << (++mNewDocumentIndex);

    CSMDoc::Document *document = mDocumentManager.addDocument (stream.str());

    static const char *sGlobals[] =
    {
            "Day", "DaysPassed", "GameHour", "Month", "PCRace", "PCVampire", "PCWerewolf", "PCYear", 0
    };

    for (int i=0; sGlobals[i]; ++i)
    {
        ESM::Global record;
        record.mId = sGlobals[i];
        record.mValue = i==0 ? 1 : 0;
        record.mType = ESM::VT_Float;
        document->getData().getGlobals().add (record);
    }

    document->getData().merge(); /// \todo remove once proper ESX loading is implemented

    mViewManager.addView (document);
}

int CS::Editor::run()
{
    /// \todo Instead of creating an empty document, open a small welcome dialogue window with buttons for new/load/recent projects
    createDocument();

    return QApplication::exec();
}