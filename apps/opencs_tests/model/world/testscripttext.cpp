#include "apps/opencs/view/world/scripttext.hpp"

#include <gtest/gtest.h>

#include <QString>

namespace CSVWorld
{
    namespace
    {
        TEST(CSVWorldScriptTextTest, returnsUnchangedTextWithoutDetachingWhenNoCarriageReturnIsPresent)
        {
            const QString source = QString::fromLatin1("begin test\nend test\n");
            QString text = source;
            toEditorText(text);

            EXPECT_EQ(text, source);
            EXPECT_EQ(text.constData(), source.constData());
        }

        TEST(CSVWorldScriptTextTest, convertsCrLfAndStandaloneCrToLf)
        {
            const QString source = QString::fromLatin1("begin test\r\nline\rline\nend test\r\n");
            QString text = source;
            toEditorText(text);

            EXPECT_EQ(text, QString::fromLatin1("begin test\nline\nline\nend test\n"));
            EXPECT_EQ(source, QString::fromLatin1("begin test\r\nline\rline\nend test\r\n"));
        }
    }
}
