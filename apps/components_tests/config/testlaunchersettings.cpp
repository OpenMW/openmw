#include <components/config/launchersettings.hpp>

#include <QString>
#include <QTextStream>

#include <gtest/gtest.h>

namespace
{
    void read(Config::LauncherSettings& settings, QString text)
    {
        QTextStream stream(&text);
        settings.readFile(stream);
    }

    Config::LauncherSettings parse(const QString& text)
    {
        Config::LauncherSettings settings;
        read(settings, text);
        return settings;
    }

    QString write(const Config::LauncherSettings& settings)
    {
        QString out;
        QTextStream stream(&out);
        settings.writeFile(stream);
        stream.flush();
        return out;
    }

    constexpr char sConfig[] = R"([Profiles]
currentprofile=Default
Default/fallback-archive=Morrowind.bsa
Default/data=/games/morrowind/Data Files
Default/content=Morrowind.esm
Default/content=Tribunal.esm
)";

    TEST(LauncherSettingsTest, shouldKeepUnknownKeysAndSectionsOnWrite)
    {
        const Config::LauncherSettings settings = parse(R"([Settings]
language=en
future=1

[Profiles]
currentprofile=Default
Default/content=Morrowind.esm
Default/future=2

[General]
firstrun=false
future=3

[Importer]
importfontsetup=false
future=4

[Future]
thing=5
)");

        EXPECT_EQ(write(settings).toStdString(), R"(
[Settings]
language=en
future=1

[Profiles]
currentprofile=Default
Default/content=Morrowind.esm
Default/future=2

[General]
firstrun=false
MainWindow/width=0
MainWindow/posy=0
MainWindow/posx=0
MainWindow/height=0
future=3

[Importer]
importcontentsetup=true
importfontsetup=false
future=4

[Future]
thing=5
)");
    }

    TEST(LauncherSettingsTest, shouldClearUnknownKeys)
    {
        const QString text = QString::fromUtf8(sConfig) + "Default/future=yes\n";
        Config::LauncherSettings settings;
        read(settings, text);
        settings.clear();
        read(settings, text);

        EXPECT_EQ(write(settings).count("Default/future=yes\n"), 1);
    }

    TEST(LauncherSettingsTest, shouldNotKeepKnownKeyWithInvalidValue)
    {
        const QString written = write(parse("[General]\nfirstrun=maybe\n"));

        EXPECT_EQ(written.count("firstrun="), 1);
        EXPECT_TRUE(written.contains("firstrun=true\n"));
    }

    TEST(LauncherSettingsTest, shouldMatchProfileKeyOnExactSuffix)
    {
        const Config::LauncherSettings settings
            = parse(QString::fromUtf8(sConfig) + "Default/metadata=elsewhere\nDefault/mycontent=Other.esp\n");

        const QString written = write(settings);

        EXPECT_TRUE(written.contains("Default/metadata=elsewhere\n"));
        EXPECT_TRUE(written.contains("Default/mycontent=Other.esp\n"));
        EXPECT_EQ(settings.getDataDirectoryList("Default"), QStringList({ "/games/morrowind/Data Files" }));
        EXPECT_EQ(settings.getContentListFiles("Default"), QStringList({ "Morrowind.esm", "Tribunal.esm" }));
    }

    TEST(LauncherSettingsTest, shouldDropUnknownKeysOfRemovedContentList)
    {
        Config::LauncherSettings settings = parse(QString::fromUtf8(sConfig)
            + "Default/future=yes\nOther/content=Bloodmoon.esm\nOther/future=no\nDefault/Extra/future=maybe\n");

        settings.removeContentList("Default");
        const QString written = write(settings);

        EXPECT_FALSE(written.contains("Default/content="));
        EXPECT_FALSE(written.contains("Default/future="));
        EXPECT_TRUE(written.contains("Other/future=no\n"));
        EXPECT_TRUE(written.contains("Default/Extra/future=maybe\n"));
    }

    TEST(LauncherSettingsTest, shouldWriteSameOutputOnRewrite)
    {
        const QString once = write(parse(QString::fromUtf8(sConfig) + "Default/future=yes\n\n[Future]\nthing=2\n"));

        EXPECT_EQ(write(parse(once)), once);
    }

    TEST(LauncherSettingsTest, shouldKeepOrderOfRepeatedUnknownKeys)
    {
        const Config::LauncherSettings settings
            = parse(QString::fromUtf8(sConfig) + "Default/future=one\nDefault/future=two\nDefault/future=three\n");

        EXPECT_TRUE(write(settings).contains("Default/future=one\nDefault/future=two\nDefault/future=three\n"));
    }
}
