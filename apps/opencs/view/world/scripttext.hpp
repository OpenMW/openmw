#ifndef CSV_WORLD_SCRIPTTEXT_H
#define CSV_WORLD_SCRIPTTEXT_H

#include <QString>

namespace CSVWorld
{
    inline void toEditorText(QString& text)
    {
        if (!text.contains(QLatin1Char('\r')))
            return;

        // Original scripts may have \r\n endings, while ScriptEdit stores \n.
        // So we should convert a ESM script to ScriptEdit format before working with its text.
        text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
        text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    }
}

#endif
