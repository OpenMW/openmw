#include "application.hpp"

#include <QFile>
#include <QOperatingSystemVersion>
#include <QStyle>
#include <QStyleHints>

#include <components/debug/debuglog.hpp>
#include <components/misc/scalableicon.hpp>

namespace Platform
{
    Application::Application(int& argc, char* argv[])
        : QApplication(argc, argv)
    {
#if defined(WIN32) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        init();
    }

    static QString getStyleSheetPath()
    {
        QString qssPath(":/dark/dark_win10.qss");
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11)
            qssPath = ":/dark/dark_win11.qss";

        return qssPath;
    }

    void Application::init()
    {
        connect(this, &Application::darkModeChanged, this, &Application::updateStyle);

        const auto* hints = QGuiApplication::styleHints();
        const auto currentStyle = QApplication::style()->objectName();
        mInitialStyle = currentStyle.toStdString();
        mCurrentStyle = currentStyle.toStdString();
        if (hints->colorScheme() == Qt::ColorScheme::Dark)
        {
            mDarkMode = true;
            if (currentStyle == "windowsvista")
            {
                mCurrentStyle = "windows";
                setStyle("windows");

                QFile file(getStyleSheetPath());
                file.open(QIODevice::ReadOnly);
                setStyleSheet(file.readAll());
            }
        }
    }

    void Application::updateStyle(bool isDark)
    {
        if (mInitialStyle != "windowsvista")
            return;

        if (isDark)
        {
            mCurrentStyle = "windows";
            setStyle("windows");

            QFile file(getStyleSheetPath());
            file.open(QIODevice::ReadOnly);
            setStyleSheet(file.readAll());
        }
        else
        {
            mCurrentStyle = mInitialStyle;
            setStyleSheet("");
            setStyle(mInitialStyle.c_str());
        }
#endif
    }

    bool Application::notify(QObject* receiver, QEvent* event)
    {
        try
        {
            if (event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange)
            {
#if defined(WIN32) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
                const auto* hints = QGuiApplication::styleHints();
                const auto currentStyle = QApplication::style()->objectName();
                bool isDark = hints->colorScheme() == Qt::ColorScheme::Dark;
                if (isDark != mDarkMode)
                {
                    mDarkMode = isDark;

                    bool result = QApplication::notify(receiver, event);

                    emit darkModeChanged(isDark);

                    return result;
                }
#endif
                Misc::ScalableIcon::updateAllIcons();
            }

            return QApplication::notify(receiver, event);
        }
        catch (const std::exception& exception)
        {
            Log(Debug::Error) << "An exception has been caught: " << exception.what();
        }

        return false;
    }
}
