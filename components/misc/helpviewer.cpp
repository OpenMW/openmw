#include "helpviewer.hpp"

#include <QDesktopServices>
#include <QString>
#include <QUrl>

#include <components/version/version.hpp>

void Misc::HelpViewer::openHelp(const char* url)
{
    std::string_view docsUrl = Version::getDocumentationUrl();
    QString link = QString::fromUtf8(docsUrl.data(), docsUrl.size());
    link.append(url);
    QDesktopServices::openUrl(QUrl(link));
}
